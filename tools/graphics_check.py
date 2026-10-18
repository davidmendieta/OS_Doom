"""Check user-mode video output using QEMU's rendered display, not guest claims.

QMP screendump documentation:
https://www.qemu.org/docs/master/interop/qemu-qmp-ref.html#command-screendump
"""
import json
import os
import select
import subprocess
import time

READY = "*** graphics: READY frame and palette"


class QMP:
    def __init__(self, process, deadline):
        self.process = process
        self.deadline = deadline
        self.pending = b""
        if "QMP" not in self.receive():
            raise RuntimeError("Missing QMP greeting")
        self.command("qmp_capabilities")

    def receive(self):
        while b"\n" not in self.pending:
            remaining = self.deadline - time.monotonic()
            if remaining <= 0 or not select.select([self.process.stdout], [], [], remaining)[0]:
                raise RuntimeError("Timed out waiting for QMP")
            data = os.read(self.process.stdout.fileno(), 65536)
            if not data:
                raise RuntimeError("QEMU closed QMP unexpectedly")
            self.pending += data
        line, self.pending = self.pending.split(b"\n", 1)
        return json.loads(line)

    def command(self, name, **arguments):
        try:
            self.process.stdin.write((json.dumps({"execute": name, "arguments": arguments}) + "\n").encode())
            self.process.stdin.flush()
        except BrokenPipeError as error:
            raise RuntimeError(f"QEMU exited during {name}; check the serial log for a guest panic") from error
        while True:
            message = self.receive()
            if "error" in message:
                raise RuntimeError(f"QMP {name}: {message['error']}")
            if "return" in message:
                return message["return"]


def verify_frame(path):
    with path.open("rb") as image:
        if image.readline().strip() != b"P6":
            raise RuntimeError("Expected a binary PPM screenshot")
        width, height = map(int, image.readline().split())
        if image.readline().strip() != b"255":
            raise RuntimeError("Expected 8-bit screenshot components")
        pixels = image.read()
    if width < 320 or height < 200 or width % 320 or height % 200:
        raise RuntimeError(f"Unexpected VGA display dimensions: {width}x{height}")
    if len(pixels) != width * height * 3:
        raise RuntimeError("Incomplete screenshot")

    def expand(value):
        # QEMU's VGA c6_to_8 replicates the low DAC bit into the low two
        # display bits: https://github.com/qemu/qemu/blob/master/hw/display/vga_int.h
        value >>= 2
        return value * 4 + (3 if value & 1 else 0)

    # Check every displayed pixel, including doubled scanlines/pixels.
    for screen_y in range(height):
        y = screen_y * 200 // height
        for screen_x in range(width):
            x = screen_x * 320 // width
            index = (y * 16 // 200) * 16 + x // 20
            if x == 0 or x == y * 3 // 2:
                index = 255
            if index == 255:
                expected = (255, 255, 255)
            else:
                expected = (expand((index & 15) * 16), expand((index >> 4) * 16), 255 if index & 1 else 0)
            offset = (screen_y * width + screen_x) * 3
            actual = tuple(pixels[offset:offset + 3])
            if actual != expected:
                raise RuntimeError(f"Pixel ({screen_x},{screen_y}): expected {expected}, got {actual}")
    return width, height


def check_graphics(command, build, timeout, input_stress=False):
    serial = build / "serial.log"
    serial.write_text("")
    command = list(command)
    command[command.index("-serial") + 1] = f"file:{serial}"
    command += ["-qmp", "stdio"]
    deadline = time.monotonic() + timeout
    with (build / "qemu.log").open("w") as errors:
        with subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=errors, bufsize=0) as process:
            try:
                qmp = QMP(process, deadline)
                while True:
                    output = serial.read_text()
                    if "FAIL" in output or "halting" in output or process.poll() is not None:
                        raise RuntimeError(f"Graphics guest failed; see {serial}")
                    if READY in output:
                        break
                    if time.monotonic() >= deadline:
                        raise RuntimeError(f"Graphics boot timed out; see {serial}")
                    time.sleep(0.02)
                if input_stress:
                    # More than 85 three-word mouse packets overflow the old
                    # 256-word queue. Space events across PS/2 sampling ticks.
                    for i in range(160):
                        qmp.command("human-monitor-command", **{"command-line": "mouse_move 4 -3"})
                        qmp.command("human-monitor-command", **{"command-line": f"mouse_button {i % 2}"})
                        time.sleep(0.02)
                    qmp.command("human-monitor-command", **{"command-line": "mouse_button 0"})
                    time.sleep(0.1)
                    if process.poll() is not None or "halting" in serial.read_text():
                        raise RuntimeError("Guest halted during mouse input stress")
                qmp.command("stop")
                qmp.command("screendump", filename=str(build / "frame.ppm"), format="ppm")
                qmp.command("screendump", filename=str(build / "frame.png"), format="png")
                qmp.command("quit")
                if process.wait(timeout=max(0.1, deadline - time.monotonic())) != 0:
                    raise RuntimeError("QEMU did not quit cleanly")
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()
                print(serial.read_text(), end="")
    width, height = verify_frame(build / "frame.ppm")
    print(f"Graphics test passed: {width}x{height} display, every pixel and all 256 palette entries checked.")
    print(f"Screenshot: {build / 'frame.png'}")
    if input_stress:
        print("Mouse input stress passed: 160 movement/button updates with no input consumer.")
