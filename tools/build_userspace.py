#!/usr/bin/env python3
"""Cross-compile the Newlib runtime smoke test or native PrBoom port."""
import os
from pathlib import Path
import re
import subprocess
from concurrent.futures import ThreadPoolExecutor

ROOT = Path(__file__).resolve().parent.parent


def build_program(program, output):
    cc = os.environ.get("CROSS_COMPILE", "i686-elf-") + "gcc"
    sysroot = ROOT / "build/toolchain/i686-elf"
    if not (sysroot / "lib/libc.a").exists():
        raise RuntimeError("Newlib is missing: run make doom-deps first")
    build = ROOT / "build" / program / "objects"
    build.mkdir(parents=True, exist_ok=True)
    sources = ["userspace/start.S", "userspace/syscalls.S", "userspace/newlib.c", "userspace/video.S"]
    flags = ["-m32", "-std=gnu99", "-O2", "-g", "-fno-pie", "-fno-stack-protector",
             "-mno-sse", "-mfpmath=387", "-fcommon", "-I" + str(sysroot / "include"),
             "-I" + str(ROOT / "include")]
    if program == "doom":
        src = ROOT / "prboom-2.5.0/src"
        common = (src / "Makefile.am").read_text().split("COMMON_SRC =", 1)[1].split("NET_CLIENT_SRC", 1)[0]
        sources += ["prboom-2.5.0/src/" + s for s in re.findall(r"\b\w+\.c\b", common)]
        sources += ["prboom-2.5.0/src/" + s for s in
                    ["d_client.c", "w_memcache.c", "mmus2mid.c", "SDL/i_main.c", "SDL/i_system.c"]]
        sources += ["ports/prboom/" + s for s in ["platform.c", "i_video.c", "i_sound.c", "i_input.c"]]
        flags += ["-DHAVE_CONFIG_H", "-D_DEBUG", "-Wno-error=incompatible-pointer-types",
                  "-I" + str(ROOT / "ports/prboom"), "-I" + str(src)]
    else:
        sources += [f"testcases/{program}/init.c"]

    def compile_one(source):
        obj = build / (source.replace("/", "_") + ".o")
        extra = ["-Dmain=doom_main"] if source.endswith("SDL/i_main.c") else []
        subprocess.run([cc, *flags, *extra, "-c", str(ROOT / source), "-o", str(obj)], check=True)
        return str(obj)

    with ThreadPoolExecutor(max_workers=4) as pool:
        objects = list(pool.map(compile_one, sources))
    output.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([cc, "-m32", "-nostdlib", "-static", "-Wl,--build-id=none,-N,-e,start,-Ttext=0x80000000",
                    *objects, "-L" + str(sysroot / "lib"), "-Wl,--start-group", "-lc", "-lm", "-lgcc",
                    "-Wl,--end-group", "-o", str(output)], check=True)
