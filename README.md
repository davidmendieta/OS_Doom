# OS-Doom

## Local development on `dev`

`main` is the original baseline; `dev` contains the consolidated feature branches.
The integrated kernel boots PrBoom in user mode, displays Doom's title screen,
and supports menus, single-player keyboard/mouse controls, and stereo sound
effects, AdLib-style music, and two-player cooperative multiplayer over native UDP.

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
isolated virtual network, and an AC'97 device. Interactive Doom uses host audio;
checks run silently or capture a WAV file. The multiplayer commands below connect
two guests. The smoke test verifies boot and filesystem
syscalls; the separate graphics test below verifies frame and palette output.
Input, audio, and multiplayer have separate QEMU checks below.
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
Legacy input queues drop incoming events; mouse records are queued or dropped as
complete three-word packets. The native input API also preserves current held
keys/buttons so Doom can recover after its event queue fills.
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
make input-test                                 # input bounds, overflow, releases, quick taps
make doom-input-test                            # drive menus and gameplay via QEMU input
make audio-test                                 # verify PCM DMA, stereo, drain and restart
make doom-audio-test                            # match captured menu/pistol audio to IWAD
make music-test                                 # MUS parser, synthesizer and playback lifecycle
make doom-music-test                            # verify title/level music through QEMU
make doom RUN_ARGS="--display cocoa"            # visible Doom on macOS; stays open
```

For Linux, select a QEMU display backend installed on your system, such as
`--display gtk`; the default audio backend is SDL (CoreAudio on macOS).
Use `--audio pa` for PulseAudio or `--audio none` for a silent run.
Add `--no-music` to keep sound effects while disabling music.
Close QEMU or press Ctrl-C in the launching terminal to stop.
The automated tests close QEMU automatically. You can add `--cpus 1` to `RUN_ARGS` to
exercise single-CPU boot; four CPUs is the default. Use `--build-dir build/another-run`
to keep files and logs separate when running multiple instances.

Click the QEMU window to focus/capture input. Press Escape, then choose
**New Game → Knee-Deep in the Dead → difficulty** using the arrows and Enter.
The defaults are classic Doom controls:

| Action | Control |
| --- | --- |
| Move forward / backward | Up / Down arrows |
| Turn | Left / Right arrows, or move mouse sideways |
| Strafe left / right | Comma / Period, or hold Alt while turning |
| Run | Hold Shift |
| Fire | Ctrl or left mouse button |
| Use doors / switches | Space |
| Select weapon | Number keys |
| Map | Tab |
| Menu / return to game | Escape |
| Pause / unpause | Pause key, if your keyboard has one |

On Cocoa, release the captured pointer with **Control+Option+G** (QEMU's
[documented window shortcut in its Cocoa backend](https://github.com/qemu/qemu/blob/master/ui/cocoa.m)).
Mouse vertical movement retains PrBoom's classic forward/backward behavior.
Key bindings can be changed in PrBoom's setup menus; settings are temporary
because the runner uses disk snapshots.

`doom-deps` downloads [Newlib 4.5.0.20241231](https://sourceware.org/pub/newlib/)
and the [Doom 1.9 shareware IWAD](https://github.com/Doom-Utils/shareware-collection/tree/master/Doom%201.9),
verifies pinned SHA-256 checksums, and builds Newlib with `i686-elf-gcc`.
Everything downloaded or compiled stays under ignored `build/`; it does not
replace the old `mlibc` or `doom-sysroot` work. Internet access, `curl`, and `tar`
are needed for this setup step. The IWAD is id Software game data, separate
from PrBoom's GPL source; it is not added to git.

`ports/prboom/` supplies the 320x200 indexed-video backend, native input, timer adapter,
and a native mixer for sound effects and OPL music. It builds the existing PrBoom 2.5.0 engine
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

The input syscall (115), defined in `include/os/input.h`, returns up to 32 ordered
key/button transitions, accumulated signed mouse movement, and current held
state. It is nonblocking and intended for one foreground reader. On overflow,
Doom reconciles held state so movement/fire releases are not lost. PS/2 uses
untranslated set 2, E0/F0/E1 keyboard prefixes, and three-byte mouse packets;
interrupt handlers only decode/enqueue and never touch the filesystem.

`input-test` deliberately fills the queue, then checks held keys/buttons,
release recovery, a complete fast tap, signed motion, and invalid user buffers.
`doom-input-test` starts a real game through its menus and checks player movement,
turning, ammunition use, dual-Ctrl handling, mouse aiming/firing, use, and pause.
It enables engine observations only in the test filesystem; normal launches
do not stream input/game state to serial. Screenshots are `build/doom/menu.png`
and `controls.png`, with observations in `build/doom/serial.log`. Controls have
been tested with both one and four QEMU CPUs.

### Networked Doom

Run two guests on this Mac, with separate windows and a private Ethernet link:

```sh
make doom-network
# Linux: make doom-network RUN_ARGS="--display gtk"
# Optional: RUN_ARGS="--audio none" for silent play
```

Both guests boot the OS and PrBoom, then automatically start cooperative E1M1
on Hurt Me Plenty. The first guest also runs the game server; Python only
connects the emulated NICs. Click a window to control that player. Music and
sound effects remain enabled in both guests. Closing either window stops the
pair launcher. Logs and screenshots from checks live in the selected build directory.

For separate terminals or machines, start the host first:

```sh
# Terminal/machine 1. Use --net-address 0.0.0.0 to accept a second machine.
make doom RUN_ARGS="--display cocoa --net-role host"

# Terminal/machine 2. Replace 127.0.0.1 with machine 1's LAN address if needed.
make doom RUN_ARGS="--display cocoa --net-role join --net-address 127.0.0.1"
```

The default host tunnel listens only on localhost. `--net-port 12345` selects
the TCP port used by QEMU to carry Ethernet frames; both launches must use the
same port. The guests themselves exchange UDP on port 5030, using static
addresses `10.0.0.1` and `10.0.0.2` with distinct MAC addresses. No guest DHCP,
host bridge, TAP device, or external Doom server is required. Host and join
images use separate `build/doom-host/` and `build/doom-join/` directories.
The separate-machine transport is available through QEMU; automated tests use
a local Ethernet relay between two real guests.

```sh
make network-test                     # UDP/ARP, wire checksums, malformed packets
make doom-network-test                # actual synchronized game, movement/fire, clean quit
make doom-network-test RUN_ARGS="--faults --cpus 1 --build-dir build/doom-network-loss"
```

The UDP test exchanges 300 datagrams (0, 1, 1471 and 1472 payload bytes), exercises
NIC DMA wraparound, duplicate binds, invalid buffers, and bounded receive queues.
The relay independently checks every outgoing IPv4/UDP checksum and injects
bad checksums, invalid lengths, truncated headers, and fragmented packets.
The multiplayer test drives both players and compares positions, angles,
health, ammunition, and commands at matching game ticks in both simulations.
`--faults` drops setup/start messages and bursts of game packets, duplicates
packets, then disconnects a guest to check timeout recovery. Normal game exit
ends the network session immediately; an unresponsive peer times out after
15 seconds, allowing the remaining player to continue locally. Starting a new
network game requires restarting both guests. Connection setup waits up to
120 seconds. Packet captures are saved as `network.pcap` (transmitted frames
before deliberate corruption/loss), alongside per-guest serial logs and screenshots.

The native API in `include/os/net.h` provides configuration, UDP open/send/receive/
close, and counters through syscalls 220–225. IP addresses and ports are numeric
host-order values; `10.0.0.1` is `0x0a000001`. Send returns the payload length,
`-2` for ARP/TX backpressure (retry), or `-1` for invalid arguments. Receive
returns 1 for a datagram, including an empty one, 0 for no packet, or -1 for an
invalid argument. Eight ports with 32 datagrams each serve the single foreground
application. RTL8139 uses persistent DMA buffers and polling outside interrupts.
PrBoom's existing tick protocol handles retransmission, with a cooperative
relay in the host guest and validated packet layouts in `ports/prboom/i_network.c`.

Current scope is two players, the same IWAD/engine build, and a single Ethernet
link with static addresses. Routing, IP fragmentation, DHCP, DNS, TCP sockets,
late joining, and server migration are not implemented. Historical network
sources remain for reference, but the old socket syscall range 200–219 now
returns -1; the integrated runner uses the native UDP API.

### Sound effects

Doom decodes its IWAD's unsigned 8-bit DMX sound effects, resamples them to
48 kHz, and mixes up to 32 channels with volume, pitch, and stereo positioning.
The engine's idle, input and rendering paths feed a short PCM queue. AC'97 uses
a ring of eight-byte descriptors and physical DMA buffers; silence follows an
underrun, and playback restarts when another effect arrives. Menu clicks,
weapons, doors and monster sounds use this path. Music feeds the same PCM queue.

The C interface in `include/os/audio.h` provides open/write/queued/close syscalls
116–119 through `userspace/syscalls.S`. It accepts signed 16-bit interleaved
stereo at 48 kHz; counts are stereo frames. Writes accept 1–1024 frames and
return zero when the DMA ring is full. Open/close discard queued audio. This
interface currently serves one foreground producer, without audio interrupts.

`audio-test` checks syscall bounds, backpressure, multiple DMA ring wraps,
a one-frame write, silence after draining, restart, and close/reopen. It compares
all 101,377 captured stereo frames with the expected PCM, including independent
left/right signals. `doom-audio-test` drives the real menus and gameplay, then
compares recorded menu and pistol waveforms against the IWAD in both channels.
That check disables music to isolate the sound-effect signal.
Its isolated filesystem, logs, screenshots and `audio.wav` live in
`build/doom-audio/`; the driver capture is `build/audio-smoke/audio.wav`.

To record a play session instead of sending audio to speakers:

```sh
make doom RUN_ARGS="--display cocoa --audio-capture build/doom/session.wav"
```

The runner completes WAV size headers after QEMU exits when the backend leaves
them unset. WAV capture is output-only; QEMU may log unavailable `ac97.pi` and
`ac97.mc` recording voices. These messages do not indicate a playback failure.

### Music

Music is enabled by default. `ports/prboom/mus_player.c` reads bounded MUS event
streams at 140 ticks per second and programs eighteen OPL voices using the IWAD's
`GENMIDI` instruments. It supports melodic and percussion instruments, double
voices, pitch bends, channel volume/expression, panning, sustain, track changes,
looping, and the engine's music-volume control. Playback timing follows generated
PCM frames, so scheduling does not accumulate timer drift within a track.

The synthesizer is [Nuked OPL3](https://github.com/nukeykt/Nuked-OPL3), vendored at
a pinned revision with its LGPL license under `ports/prboom/third_party/`.
Instrument frequency/volume tables come from Chocolate Doom under GPL-2.0-or-later;
source attribution is in `opl_tables.h`. The build needs no soundfont download or
host MIDI service. This produces AdLib-style FM music; external MIDI/MP3/OGG
replacements are not supported and the engine falls back to its MUS lump.

PrBoom's `mus_pause_opt` setting is honored: 0 stops/restarts the track, 1 freezes
and resumes its position, and 2 (the upstream default) continues music while the
game is paused. Music and effects have independent volume controls.

`music-test` requires a host C compiler (`cc`) with AddressSanitizer. It checks
truncated/malformed scores, timing, exact loops, pause/resume, mute, stop, stereo
pan, sustain, percussion, pitch bends and repeated-note attacks, and renders all
13 tracks in the shareware IWAD. `doom-music-test` additionally boots the game,
compares 24,000 consecutive captured stereo frames from each of the title and
E1M1 tracks against the native reference renderer, and exercises menus, gameplay,
firing and pause with music enabled. It passes with one and four QEMU CPUs.
Artifacts are in `build/doom-music/`, including `audio.wav`, screenshots and logs;
standalone reference renders are in `build/music-core/`.

PrBoom networking is not built. The runtime has a 48 MiB heap limit and exposes
uptime rather than calendar time; it is not a general-purpose POSIX environment.
Multiplayer is the next integration milestone.

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
