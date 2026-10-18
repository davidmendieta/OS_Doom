#!/usr/bin/env python3
"""Build a fresh program filesystem and boot the integrated kernel in QEMU."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

from graphics_check import check_graphics
from build_userspace import build_program
from doom_check import check_doom
from input_check import check_input_stream

ROOT = Path(__file__).resolve().parent.parent
PASS = "*** smoke: PASS user mode, arguments, ext2, syscalls"


def tool(name):
    found = shutil.which(name)
    if found:
        return found
    if name == "mkfs.ext2":
        for prefix in ("/opt/homebrew", "/usr/local"):
            candidate = Path(prefix) / "opt/e2fsprogs/sbin/mkfs.ext2"
            if candidate.is_file():
                return str(candidate)
    raise RuntimeError(f"Missing {name}; see README.md for build prerequisites")


def build_filesystem(program, build, controls_check=False):
    prefix = os.environ.get("CROSS_COMPILE")
    if prefix is None:
        prefix = "i686-elf-" if shutil.which("i686-elf-gcc") else ""
    cc = tool(prefix + "gcc")
    mkfs = tool("mkfs.ext2")
    rootfs = build / "root"
    (rootfs / "sbin").mkdir(parents=True, exist_ok=True)
    (rootfs / "etc").mkdir(exist_ok=True)
    (rootfs / "etc/boot-message").write_text("Hello from ext2!\n")
    if controls_check:
        (rootfs / "etc/input-test").write_text("enable engine observations\n")
    else:
        (rootfs / "etc/input-test").unlink(missing_ok=True)
    if program in ("doom", "runtime-smoke", "input-smoke"):
        build_program(program, rootfs / "sbin/init")
    else:
        subprocess.run([
            cc, "-m32", "-std=c99", "-ffreestanding", "-fno-pie", "-fno-stack-protector",
            "-nostdlib", "-static", "-O2", "-g", "-Wall", "-Werror",
            "-Wl,--build-id=none,-N,-e,start,-Ttext=0x80000000",
            "-I", str(ROOT / "include"),
            str(ROOT / "testcases/smoke/start.S"), str(ROOT / f"testcases/{program}/init.c"),
            str(ROOT / "userspace/video.S"),
            "-o", str(rootfs / "sbin/init"),
        ], check=True)
    if program == "doom":
        (rootfs / "doom").mkdir(exist_ok=True)
        (rootfs / ".prboom").mkdir(exist_ok=True)
        shutil.copyfile(ROOT / "build/deps/doom1.wad", rootfs / "doom/doom1.wad")
        shutil.copyfile(ROOT / "prboom-2.5.0/data/prboom.wad", rootfs / "doom/prboom.wad")
    disk = build / "rootfs.img"
    # Create a new image every time; never reuse committed testcase results.
    with disk.open("wb") as image:
        image.truncate((32 if program == "doom" else 10) * 1024 * 1024)
    subprocess.run([
        mkfs, "-F", "-q", "-b", "4096", "-i", "4096", "-I", "128",
        "-O", "none", "-d", str(rootfs), str(disk),
    ], check=True)
    return disk


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--program", choices=("smoke", "graphics-smoke", "runtime-smoke", "input-smoke", "doom"), default="smoke")
    parser.add_argument("--check", action="store_true", help="run the program's automated boot/display checks")
    parser.add_argument("--disk", type=Path, help="boot an existing ext2 image instead of the smoke program")
    parser.add_argument("--display", default="none", help="QEMU display backend (none, cocoa, gtk, etc.)")
    parser.add_argument("--cpus", type=int, default=4)
    parser.add_argument("--timeout", type=float, default=30)
    parser.add_argument("--input-stress", action="store_true", help="exercise mouse input while the graphics demo holds its frame")
    parser.add_argument("--controls-check", action="store_true", help="test Doom menus and player controls through QEMU input")
    args = parser.parse_args()
    if args.controls_check and (args.program != "doom" or not args.check):
        parser.error("--controls-check requires --program doom --check")
    build = ROOT / "build" / args.program
    if args.input_stress and (not args.check or args.program != "graphics-smoke"):
        parser.error("--input-stress requires --program graphics-smoke --check")
    if args.check and args.disk:
        parser.error("--check requires the freshly built smoke filesystem")
    if not 1 <= args.cpus <= 16 or args.timeout <= 0:
        parser.error("--cpus must be 1..16 and --timeout must be positive")

    qemu = tool(os.environ.get("QEMU_CMD", "qemu-system-i386"))
    kernel = ROOT / "kernel/build/kernel.img"
    if not kernel.is_file():
        raise RuntimeError("Missing kernel/build/kernel.img; run make build first")
    disk = args.disk.resolve() if args.disk else build_filesystem(args.program, build, args.controls_check)
    if not disk.is_file():
        raise RuntimeError(f"Missing filesystem image: {disk}")
    command = [
        qemu, "-machine", "pc", "-accel", "tcg", "-cpu", "max",
        "-smp", str(args.cpus), "-m", "128M", "-no-reboot", "-snapshot",
        "-display", args.display, "-vga", "std", "-monitor", "none", "-serial", "stdio",
        "-drive", f"file={kernel},index=0,media=disk,format=raw",
        "-drive", f"file={disk},index=1,media=disk,format=raw",
        "-device", "isa-debug-exit,iobase=0xf4,iosize=0x04",
        "-netdev", "hubport,id=net0,hubid=0",
        "-device", "rtl8139,netdev=net0,mac=52:54:00:12:34:56",
        "-audiodev", "none,id=audio0", "-device", "AC97,audiodev=audio0",
    ]
    if not args.check:
        result = subprocess.run(command)
        return 0 if result.returncode == 1 else result.returncode

    build.mkdir(parents=True, exist_ok=True)
    if args.program == "graphics-smoke":
        check_graphics(command, build, args.timeout, args.input_stress)
        return 0
    if args.program == "doom":
        check_doom(command, build, args.timeout, args.controls_check)
        return 0
    if args.program == "input-smoke":
        check_input_stream(command, build, args.timeout)
        return 0
    log = build / "serial.log"
    with log.open("w") as output:
        try:
            result = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, timeout=args.timeout)
        except subprocess.TimeoutExpired:
            print(log.read_text(), end="")
            raise RuntimeError(f"Boot timed out after {args.timeout}s; log: {log}")
    serial = log.read_text()
    print(serial, end="")
    # isa-debug-exit returns (value << 1) | 1. A panic uses it too, so
    # the exit status alone can never establish success.
    marker = "*** runtime: PASS allocation, files, seeking, clock, x87, libc" if args.program == "runtime-smoke" else PASS
    if result.returncode != 1 or marker not in serial or "shutdown\n" not in serial or "FAIL" in serial or "halting" in serial:
        raise RuntimeError(f"Smoke test failed (QEMU exit {result.returncode}); log: {log}")
    print(f"Boot smoke test passed on {args.cpus} CPU(s). Log: {log}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(f"error: {error}", file=sys.stderr)
        sys.exit(1)
    except KeyboardInterrupt:
        sys.exit(130)
