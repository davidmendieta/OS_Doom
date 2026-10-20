"""Boot the actual engine and compare its VGA title against IWAD TITLEPIC."""
import struct
import subprocess
import time

from graphics_check import QMP


def title_pixels(wad):
    data = wad.read_bytes()
    magic, count, directory = struct.unpack_from("<4sII", data)
    if magic != b"IWAD":
        raise RuntimeError("Expected an IWAD")
    lumps = {}
    for i in range(count):
        offset, size, name = struct.unpack_from("<II8s", data, directory + i * 16)
        lumps[name.rstrip(b"\0")] = data[offset:offset + size]
    patch, palette = lumps[b"TITLEPIC"], lumps[b"PLAYPAL"]
    width, height = struct.unpack_from("<HH", patch)
    if (width, height) != (320, 200):
        raise RuntimeError("Expected a 320x200 shareware title")
    pixels = bytearray(width * height)
    for x in range(width):
        offset = struct.unpack_from("<I", patch, 8 + x * 4)[0]
        while patch[offset] != 255:
            top, length = patch[offset:offset + 2]
            for y in range(length):
                pixels[(top + y) * width + x] = patch[offset + 3 + y]
            offset += length + 4
    def dac(component):
        value = component >> 2
        return value * 4 + (3 if value & 1 else 0)
    return [bytes(dac(c) for c in palette[p * 3:p * 3 + 3]) for p in pixels]


def verify_title(path, wad):
    expected = title_pixels(wad)
    with path.open("rb") as image:
        if image.readline().strip() != b"P6":
            raise RuntimeError("Expected PPM screenshot")
        width, height = map(int, image.readline().split())
        if image.readline().strip() != b"255":
            raise RuntimeError("Expected 8-bit RGB")
        actual = image.read()
    if width % 320 or height % 200 or not width or not height or len(actual) != width * height * 3:
        raise RuntimeError(f"Invalid VGA screenshot dimensions: {width}x{height}")
    mismatches = 0
    for y in range(height):
        for x in range(width):
            p = (y * width + x) * 3
            if actual[p:p + 3] != expected[(y * 200 // height) * 320 + x * 320 // width]:
                mismatches += 1
    if mismatches:
        raise RuntimeError(f"Title mismatch: {mismatches}/{width * height} displayed pixels differ from IWAD")
    print(f"Doom title test passed: all {width * height} displayed pixels match IWAD TITLEPIC/PLAYPAL.")


def check_doom(command, build, timeout, controls_check=False, music_check=False):
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
                while "*** doom: title frame submitted" not in serial.read_text():
                    output = serial.read_text()
                    if "FAIL" in output or "halting" in output or process.poll() is not None:
                        raise RuntimeError(f"Doom boot failed; see {serial}")
                    if time.monotonic() >= deadline:
                        raise RuntimeError(f"Doom title timed out; see {serial}")
                    time.sleep(0.01)
                qmp.command("stop")
                qmp.command("screendump", filename=str(build / "title.ppm"), format="ppm")
                qmp.command("screendump", filename=str(build / "title.png"), format="png")
                verify_title(build / "title.ppm", build / "root/doom/doom1.wad")
                qmp.command("cont")
                if music_check:
                    time.sleep(1.2) # capture title music before menu SFX begin
                if controls_check:
                    from input_check import check_controls
                    check_controls(qmp, process, serial, build, deadline)
                else:
                    while "*** doom: level frame submitted" not in serial.read_text():
                        output = serial.read_text()
                        if "FAIL" in output or "halting" in output or process.poll() is not None:
                            raise RuntimeError(f"Doom demo failed; see {serial}")
                        if time.monotonic() >= deadline:
                            raise RuntimeError(f"Doom demo timed out; see {serial}")
                        time.sleep(0.02)
                    # Let the title-to-level wipe finish and observe the demo moving.
                    time.sleep(2)
                    qmp.command("screendump", filename=str(build / "demo-first.ppm"), format="ppm")
                    time.sleep(1)
                    qmp.command("stop")
                    qmp.command("screendump", filename=str(build / "demo.ppm"), format="ppm")
                    qmp.command("screendump", filename=str(build / "demo.png"), format="png")
                    if (build / "demo-first.ppm").read_bytes() == (build / "demo.ppm").read_bytes():
                        raise RuntimeError("Doom demo display did not advance")
                    output = serial.read_text()
                    if "FAIL" in output or "halting" in output:
                        raise RuntimeError(f"Doom failed during demo playback; see {serial}")
                qmp.command("quit")
                if process.wait(timeout=5) != 0:
                    raise RuntimeError("QEMU did not quit cleanly")
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()
                # Full per-tic observations remain in serial.log for diagnosis.
                print("\n".join(line for line in serial.read_text().splitlines()
                                if not line.startswith("*** control:")))
    if not controls_check:
        print("Doom attract demo passed: reached a level and displayed changing frames.")
    print(f"Screenshot: {build / 'title.png'}")
    if controls_check:
        print(f"Controls screenshot: {build / 'controls.png'}; observations: {serial}")
