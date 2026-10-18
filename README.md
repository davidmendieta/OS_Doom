# OS-Doom

## Local development on `dev`

`main` is the original baseline; `dev` contains the consolidated feature branches.
The integrated kernel boots PrBoom in user mode, displays Doom's title screen,
and runs its built-in attract demo. Keyboard/mouse control, game audio, and
multiplayer integration are still unfinished.

On macOS, install the build and emulation tools once:

```sh
brew install i686-elf-gcc qemu e2fsprogs
```

Python 3 and Make are also required. On Linux, use the same `i686-elf-` cross
toolchain, or a GCC/binutils installation capable of producing 32-bit x86 ELF
executables, plus QEMU and e2fsprogs. `CROSS_COMPILE` can select another toolchain
prefix. The build automatically prefers `i686-elf-gcc` when available.

From the repository root:

```sh
make -j4 build                      # build kernel/build/kernel.img from source
make smoke                         # build and run the automated boot check
make smoke RUN_ARGS="--cpus 1"      # exercise single-CPU startup
make run                           # boot the smoke program with serial output
```

The smoke check compiles a fresh `/sbin/init`, creates an ext2 filesystem, and
requires successful entry into user mode, valid `argc`/`argv`, file contents,
EOF, close, and shutdown. A kernel panic or timeout fails the check. Generated
files and the serial log live in `build/smoke/`; existing testcase binaries and
disk images are preserved. Tested with GCC 16.2.0, QEMU 11.1.2, and e2fsprogs
1.47.4 on Apple Silicon.

To boot an existing filesystem image instead, for example:

```sh
make run RUN_ARGS="--disk t0.data"
# Add --display cocoa for a QEMU window on macOS.
```

The runner uses temporary disk snapshots, standard VGA, an RTL8139 NIC on an
isolated virtual network, and an AC'97 device with silent audio. It does not yet
configure a multiplayer connection. The smoke test verifies boot and filesystem
syscalls; the separate graphics test below verifies frame and palette output.
Input events, audio playback, and network traffic still need runtime validation.
Older `run_qemu_*` scripts and testcase harnesses retain their original classroom
assumptions; use the commands above for this development baseline.

The branch-by-branch source inventory is in [BRANCH_PROGRESS.md](BRANCH_PROGRESS.md).

### User-space graphics

```sh
make graphics-test                              # automated display verification
make graphics-demo RUN_ARGS="--display cocoa"   # visible demo on macOS
```

The demo presents a 320x200 frame with 256 palette indices through `/dev/vga`.
The automated test captures QEMU's display and checks every rendered pixel,
including all palette entries and a partial palette update. It also checks that
invalid palette ranges, unmapped buffers, and negative write lengths are rejected.
It then injects 160 mouse movement/button updates while the demo leaves input
unread and verifies that the guest stays running with the same displayed frame.
Full input queues drop incoming events; mouse records are queued or dropped as
complete three-word packets. Mouse event tracing is disabled by default.
QEMU doubles this mode to 640x400; artifacts are `build/graphics-smoke/frame.png`,
`frame.ppm`, and `serial.log`. The visible demo holds its frame until QEMU is closed
or interrupted with Ctrl-C in the launching terminal.
`graphics-test` closes QEMU automatically when verification finishes; use
`graphics-demo` to keep the window open.

The C interface is in `include/os/video.h`; link `userspace/video.S` with a user
program for the mode-init (103) and palette (105) syscall wrappers. Palette data
is an array of RGB byte triples, with components quantized to VGA's six-bit DAC.
After initialization, open `/dev/vga` and write 64,000 bytes of row-major palette
indices. Legacy partial writes update the beginning of the frame; writes larger
than a frame are capped at 64,000 bytes. Both frame and palette buffers must be
in the process's private mapped memory. The old mapping syscall remains 104.
This interface supplies the indexed-frame/palette path needed by PrBoom; it is
not a complete SDL implementation.

### Doom in QEMU

```sh
make doom-deps                                  # one-time download/build of Newlib + shareware IWAD
make runtime-test                               # libc, filesystem, clock, floating-point checks
make doom-test RUN_ARGS="--timeout 60"           # verify title pixels and moving attract demo
make doom RUN_ARGS="--display cocoa"            # visible Doom on macOS; stays open
```

For Linux, select a QEMU display backend installed on your system, such as
`--display gtk`. Close QEMU or press Ctrl-C in the launching terminal to stop.
`doom-test` closes QEMU automatically. You can add `--cpus 1` to `RUN_ARGS` to
exercise single-CPU boot; four CPUs is the default.

`doom-deps` downloads [Newlib 4.5.0.20241231](https://sourceware.org/pub/newlib/)
and the [Doom 1.9 shareware IWAD](https://github.com/Doom-Utils/shareware-collection/tree/master/Doom%201.9),
verifies pinned SHA-256 checksums, and builds Newlib with `i686-elf-gcc`.
Everything downloaded or compiled stays under ignored `build/`; it does not
replace the old `mlibc` or `doom-sysroot` work. Internet access, `curl`, and `tar`
are needed for this setup step. The IWAD is id Software game data, separate
from PrBoom's GPL source; it is not added to git.

`ports/prboom/` supplies the 320x200 indexed-video backend, timer adapter, and
explicitly silent audio backend. It builds the existing PrBoom 2.5.0 engine
against Newlib, without requiring SDL on the guest. The tiny local `SDL.h` only
provides the two timer functions used by upstream `SDL/i_system.c`.
`userspace/newlib.c` connects allocation, buffered file I/O, metadata, seeking,
and uptime to kernel syscalls. New calls 110–114 are defined in
`include/os/runtime.h`. Programs use x87 floating point, saved/restored across
thread switches, and a single ELF load segment for the current loader.

The runtime test creates and reads a 640 KiB file backwards, crossing both the
disk cache capacity and ext2's direct-block boundary. It also checks calloc,
realloc, EOF, relative/end seeking, formatting, libm, and a live x87 value across
scheduling. The filesystem path required larger kernel stacks and a repaired
bounded disk cache; `flush()` now writes dirty sectors without destroying it.

The Doom check independently decodes `TITLEPIC` and `PLAYPAL` from the IWAD and
compares every QEMU display pixel, then waits for a level and checks that demo
frames change. Artifacts are `build/doom/title.png`, `demo.png`, and `serial.log`.
Each run builds a fresh ext2 image and QEMU uses a snapshot, so configuration,
cache, and savegame writes do not persist between runs.

This milestone runs the title/attract loop. Input events are not yet connected
to PrBoom, audio is disabled with `-nosound`, and PrBoom networking is not built.
The runtime has a 48 MiB heap limit and exposes uptime rather than calendar time;
it is not a general-purpose POSIX environment. Next: wire PS/2 events into
`I_StartTic` so menus and gameplay are controllable.

The instructions below describe the original team workflow.

## Logistics
- Each group should have their own branch onto which they write their feature
  ```bash
  $ git checkout -b <feature_name>
  $ git push --set-upstream origin <feature_name>
  ```
- I don't recommend using `$ git add .` since it is easy to push temporary files / insignificant changes which might lead to merge conflicts in the future. Therefore, try to make sure the files you `$ git add <file1> <file2>...` are the actual ones you are working on.
- I know that sometimes it can be hard not to, but please try to keep all your changes in files/directories that are proper to your group. That is, if you need to add a feature to kernelMain, write all of it in another file and simply call one function inside kernelMain. This will help prevent merge conflicts. If you do find yourself needing to change files that are shared with other groups, please let the class know by writing so on the discord group chat.
- In short, try to put everything you add inside your group's directory (both in kernel and userside, and for your testcases).
- Once you have finished a feature that you would like to add to main, you should:
  ```bash
  $ git pull
  $ git merge origin/main
  ```
- Fix all the merge conflicts to make sure your branch is compatible with main. Once this is done, make sure to `$ git push` your changes.
- You will then go to this github page and create a pull request, and write a description of what you have done. We will then review your changes, write comments if things need to be changed, or approve it and merge it to main if it is good to go.

## Presentation
- Each group will individually present their project using their testcases (or anything else, really)
- We will have one demonstration by playing Doom on 2 computers running QEMU
