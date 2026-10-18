"""Exercise PS/2 via QEMU and assert effects in the actual Doom engine."""
import re
import subprocess
import time
from graphics_check import QMP


def check_controls(qmp, process, serial, build, deadline):
    def states():
        output = serial.read_text()
        if process.poll() is not None or "halting" in output or "FAIL" in output:
            raise RuntimeError(f"Input guest failed; see {serial}")
        return [dict((k, int(v)) for k, v in re.findall(r"(\w+)=(-?\d+)", line))
                for line in output.splitlines() if line.startswith("*** control:") and "buttons=" in line]

    def wait(predicate, description, after=-1):
        end = min(deadline, time.monotonic() + 6)
        while time.monotonic() < end:
            for state in states():
                if state["tic"] > after and predicate(state):
                    return state
            time.sleep(0.02)
        raise RuntimeError(f"Input check timed out: {description}; latest={states()[-1:]}")

    def latest():
        return states()[-1]

    def key(name, down):
        qmp.command("input-send-event", events=[{"type": "key", "data": {
            "down": down, "key": {"type": "qcode", "data": name}}}])

    def tap(name):
        key(name, True)
        time.sleep(0.10)
        key(name, False)
        time.sleep(0.12)

    def mouse(dx=0, dy=0, button=None, down=False):
        events = []
        if dx: events.append({"type": "rel", "data": {"axis": "x", "value": dx}})
        if dy: events.append({"type": "rel", "data": {"axis": "y", "value": dy}})
        if button: events.append({"type": "btn", "data": {"button": button, "down": down}})
        qmp.command("input-send-event", events=events)

    first = wait(lambda s: True, "first engine observation")
    tap("esc")
    wait(lambda s: s["menu"] != 0, "Escape opens menu", first["tic"])
    qmp.command("screendump", filename=str(build / "menu.png"), format="png")
    for _ in range(3): tap("ret") # New Game -> Episode 1 -> Hurt Me Plenty
    start = wait(lambda s: s["level"] and not s["demo"] and not s["menu"], "start an interactive game")
    time.sleep(1) # allow the screen wipe to finish
    start = latest()
    key("up", True)
    moving = wait(lambda s: s["forward"] > 0 and (s["x"], s["y"]) != (start["x"], start["y"]),
                  "Up moves the player", start["tic"])
    key("up", False)
    stopped = wait(lambda s: s["forward"] == 0, "Up release stops movement command", moving["tic"])
    key("left", True)
    turning = wait(lambda s: s["turn"] > 0 and s["angle"] != stopped["angle"], "Left turns", stopped["tic"])
    key("left", False)
    wait(lambda s: s["turn"] == 0, "Left release", turning["tic"])

    # Both physical Ctrl keys map to one Doom action; releasing one must not
    # release the other. Ammo proves an actual shot, not just an input marker.
    before = latest()
    key("ctrl", True)
    fired = wait(lambda s: s["ammo"] < before["ammo"], "Ctrl fires", before["tic"])
    key("ctrl_r", True)
    time.sleep(0.1)
    key("ctrl", False)
    still_firing = wait(lambda s: s["buttons"] & 1, "right Ctrl remains held", latest()["tic"])
    key("ctrl_r", False)
    wait(lambda s: not s["buttons"] & 1, "both Ctrl releases stop firing", still_firing["tic"])

    for dx, sign in ((12, -1), (-12, 1)):
        before = latest()
        mouse(dx=dx)
        wait(lambda s: s["turn"] * sign > 0, f"mouse X={dx} turns with correct sign", before["tic"])
    before = latest()
    mouse(button="left", down=True)
    fired = wait(lambda s: s["ammo"] < before["ammo"], "left mouse fires", before["tic"])
    mouse(button="left", down=False)
    wait(lambda s: not s["buttons"] & 1, "mouse release stops firing", fired["tic"])
    before = latest()
    tap("spc")
    wait(lambda s: s["buttons"] & 2, "Space activates use", before["tic"])
    before = latest()
    tap("pause")
    paused = wait(lambda s: s["paused"], "Pause key", before["tic"])
    tap("pause")
    wait(lambda s: not s["paused"], "Pause toggles off", paused["tic"])
    before = latest()
    tap("esc")
    menu = wait(lambda s: s["menu"], "in-game menu", before["tic"])
    tap("esc")
    wait(lambda s: not s["menu"], "resume from menu", menu["tic"])
    qmp.command("stop")
    qmp.command("screendump", filename=str(build / "controls.png"), format="png")
    print("Doom input passed: menus, new game, movement/release, turning, Ctrl aliases, mouse aim/fire, use, pause.")


def check_input_stream(command, build, timeout):
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
                def wait(marker):
                    while marker not in serial.read_text():
                        if process.poll() is not None or time.monotonic() >= deadline:
                            raise RuntimeError(f"Input stream check failed waiting for {marker}; see {serial}")
                        time.sleep(0.01)
                def key(name, down):
                    return {"type": "key", "data": {"down": down, "key": {"type": "qcode", "data": name}}}
                wait("*** input: flood ready")
                # Space hardware packets so this tests our guest queue, not
                # QEMU's small emulated keyboard FIFO dropping bytes.
                for _ in range(40):
                    qmp.command("input-send-event", events=[key("a", True), key("a", False)])
                    time.sleep(0.02)
                qmp.command("input-send-event", events=[key("up", True),
                    {"type": "btn", "data": {"button": "left", "down": True}},
                    {"type": "rel", "data": {"axis": "x", "value": 12}},
                    {"type": "rel", "data": {"axis": "y", "value": -7}}])
                wait("*** input: release ready")
                qmp.command("input-send-event", events=[key("up", False),
                    {"type": "btn", "data": {"button": "left", "down": False}}])
                time.sleep(0.1)
                qmp.command("input-send-event", events=[key("ret", True), key("ret", False)])
                wait("*** input: PASS")
                if process.wait(timeout=max(0.1, deadline - time.monotonic())) != 1:
                    raise RuntimeError("Input smoke did not exit through guest shutdown")
                output = serial.read_text()
                if "FAIL" in output or "halting" in output or "shutdown\n" not in output:
                    raise RuntimeError("Input smoke failed")
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()
                print(serial.read_text(), end="")
    print("Input stream passed: overflow recovery, release state, quick taps, signed mouse motion, syscall bounds.")
