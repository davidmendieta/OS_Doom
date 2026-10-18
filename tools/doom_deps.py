#!/usr/bin/env python3
"""Fetch pinned build dependencies and install Newlib inside ignored build/."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
DEPS = ROOT / "build/deps"
VERSION = "4.5.0.20241231"
NEWLIB_SHA = "33f12605e0054965996c25c1382b3e463b0af91799001f5bb8c0630f2ec8c852"
WAD_SHA = "1d7d43be501e67d927e415e0b8f3e29c3bf33075e859721816f652a526cac771"


def fetch(name, url, digest):
    path = DEPS / name
    if not path.exists():
        temp = path.with_suffix(path.suffix + ".download")
        subprocess.run(["curl", "-fL", "--retry", "2", "--max-time", "120", url, "-o", str(temp)], check=True)
        if hashlib.sha256(temp.read_bytes()).hexdigest() != digest:
            raise RuntimeError(f"Checksum mismatch: {temp}")
        temp.replace(path)
    if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise RuntimeError(f"Checksum mismatch: {path}; remove it and retry")
    return path


def main():
    DEPS.mkdir(parents=True, exist_ok=True)
    archive = fetch("newlib.tar.gz", f"https://sourceware.org/pub/newlib/newlib-{VERSION}.tar.gz", NEWLIB_SHA)
    fetch("doom1.wad", "https://raw.githubusercontent.com/Doom-Utils/shareware-collection/master/Doom%201.9/doom1.wad", WAD_SHA)
    install = ROOT / "build/toolchain"
    if (install / "i686-elf/lib/libc.a").exists():
        print("Newlib is installed; dependency checksums passed.")
        return
    source = DEPS / f"newlib-{VERSION}"
    if not source.exists():
        subprocess.run(["tar", "-xzf", str(archive), "-C", str(DEPS)], check=True)
    build = ROOT / "build/newlib-build"
    build.mkdir(exist_ok=True)
    commands = [
        [str(source / "configure"), "--target=i686-elf", "--disable-multilib",
         "--disable-newlib-supplied-syscalls", "--disable-libgloss", "--disable-nls",
         "--prefix=" + str(install),
         "CFLAGS_FOR_TARGET=-O2 -g -std=gnu11 -mno-sse -fno-pie -fno-stack-protector"],
        ["make", "-j" + str(min(os.cpu_count() or 2, 4)), "all-target-newlib"],
        ["make", "install-target-newlib"],
    ]
    for name, command in zip(("configure", "build", "install"), commands):
        log = build / (name + ".log")
        print(f"Newlib {name}; log: {log}", flush=True)
        with log.open("w") as output:
            subprocess.run(command, cwd=build, stdout=output, stderr=subprocess.STDOUT, check=True)
    print("Newlib installed locally. Shareware IWAD checksum passed.")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(f"error: {error}", file=sys.stderr)
        sys.exit(1)
