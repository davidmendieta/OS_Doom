# Branch progress inventory

Reviewed 2026-10-16 against the locally available Git refs. This is a source and history assessment, not a build or runtime certification. No remote fetch, branch checkout, merge, build, or QEMU run was performed. Committed binaries, `.result` files, and commit messages are historical evidence only; `.ok` files describe expected output, not a passing run.

All 25 available remote branch names are covered below. The three local branches (`main`, `mouse`, and `networking_refactor`) match their respective `origin/` tips. `origin/HEAD` is an alias for `origin/main`, not another development branch. The current checkout is clean at the start of this review, on `networking_refactor`; the earlier observation about a nested checkout concerned the previous workspace location.

Paths in each entry refer to that branch's tree and may not exist in the current checkout. Inspect them with `git show origin/<branch>:<path>`. Tip hashes make these notes reproducible even if branch pointers later move. Status labels describe implementation maturity, without assigning speculative completion percentages.

## Overall assessment

The project has a substantial 32-bit x86 kernel foundation and separate hardware/API demonstrations. A complete user-space Doom port and a reproducible integrated multiplayer build remain unfinished. PrBoom 2.5.0 in `doom_integration` is identical to the PrBoom tree in `main`; importing the game source did not complete its SDL, SDL_net, libc, timing, audio, or input dependencies.

The largest immediate integration problems are:

- `doom_integration` and `libc` contain committed merge-conflict markers in `kernel/sys.cc` and `kernel/sys.h`. The integration branch additionally duplicates `vga_syscall`, `stat`, and `fstat` definitions.
- In `doom_integration`, syscall numbers 200/201 are assigned to `stat`/`fstat`, but the dispatcher first routes the entire 200–249 range to socket syscalls, where 200/201 mean `socket`/`bind`.
- Several branches replace normal startup with a device demo or leave initialization in an infinite loop. Combining files does not produce a usable boot sequence automatically.
- Feature branches modify the same syscall, memory, filesystem, interrupt, PCI, and startup files. Their APIs must be reconciled explicitly.
- The test harness assumes a Linux-style 32-bit toolchain, GNU tools, ext2 image creation, and QEMU. A repeatable environment and fresh baseline runs are still required.

## Foundation and integration branches

### `main`

Tip: `350a8ec71` — 2025-04-21. **Status: kernel foundation; game source imported.**

- **Progress:** Boot and SMP initialization, threads/processes, paging, ELF loading, IDE/ext2 access, custom syscalls, and PCI enumeration. `kernel/kernel.cc` mounts the second disk, loads `/sbin/init`, constructs its arguments, and enters user mode. History includes an `open` null-pointer fix and the switch from Chocolate Doom to PrBoom.
- **Evidence:** `kernel/init.cc`, `kernel/kernel.cc`, `kernel/sys.cc`, `kernel/vmm.cc`, `kernel/ext2.cc`, `kernel/pci.cc`, and `prboom-2.5.0/`. Most subsystem test directories contain only README placeholders.
- **Remaining:** Feature integration, a game executable/root filesystem, platform dependencies, and a verified build/boot. The top-level harness defaults to `all_tests`, which is not a complete bundled test suite on this branch.
- **Use:** Reference baseline for the original kernel and a candidate for establishing the first reproducible boot.

### `template`

Tip: `e8ca61c1f` — 2025-04-16. **Status: historical starting point.**

- **Progress:** Initial kernel/project organization and team directory structure.
- **Evidence:** An ancestor of `main`; it has no commits absent from `main`. Inspect its `kernel/` and original README for the starting architecture.
- **Remaining:** Later core fixes and all subsequent feature work. This is not an alternate integration candidate.
- **Use:** Historical comparison only.

### `writing-to-disk2`

Tip: `81f38daf9` — 2025-04-19. **Status: historical snapshot; no independent disk feature at this tip.**

- **Progress:** Points to the PCI merge already in `main`'s history.
- **Evidence:** `git rev-list origin/main..origin/writing-to-disk2` is empty. Its name should not be interpreted as a newer version of `writing-to-disk`.
- **Remaining:** The actual filesystem mutation work is on `writing-to-disk` and related input/storage integration branches.
- **Use:** Preserve as history; no unique commits to integrate into `main`.

### `doom_integration`

Tip: `69b4f9f62` — 2025-04-25. **Status: partial integration with source-level build blockers.**

- **Progress:** Combines networking, VGA, early PS/2 work, and an early mlibc port. `kernelMain` calls `networkingMain`, `vga_init`, and `PS2::init` before loading init. Contains the full history of `main`.
- **Evidence:** `kernel/kernel.cc`, `kernel/sys.cc`, `kernel/sys.h`, `mlibc/sysdeps/doom-new-os/`, plus networking, graphics, and peripheral test trees.
- **Remaining:** Resolve committed conflict markers, duplicate definitions, and the syscall collision described above. Complete the game platform layer and build/package PrBoom. Later work from numerous feature branches is absent; the PrBoom tree itself is unchanged from `main`.
- **Use:** Integration reference and source of prior merge decisions. It is not a known-good runnable baseline.

## Hardware, graphics, and input branches

### `PCI`

Tip: `c94f4a645` — 2025-04-23. **Status: substantial infrastructure; one later verification fix.**

- **Progress:** PCI enumeration, vendor/device lookup, configuration access, and BAR parsing are largely already in `main`. The unique later commit changes verification to iterate the expected-device list instead of looking up devices just discovered.
- **Evidence:** `kernel/pci.cc`, especially `verifyDevices`; the branch's unique patch relative to its common ancestor with `main` changes this file only.
- **Remaining:** Verify the expected-device policy against the chosen QEMU hardware. Missing expected devices cause a panic, so copying this fix without checking the list can prevent boot.
- **Use:** Review the focused verification patch; retain the shared PCI implementation used by NIC/audio drivers.

### `graphics`

Tip: `582c4a872` — 2025-04-28. **Status: VGA demo implementation; incomplete game presentation API.**

- **Progress:** 320×200, 256-color VGA setup, pixel/image drawing helpers, color-bar/image demonstrations, and a Sonic animation/banner. A VGA syscall and graphical test configuration exist.
- **Evidence:** `kernel/vga.cc`, `kernel/vga.h`, `kernel/graphics/framebuffer.cc`, `testcases/graphics/vga_test.dir/`, and `testcases/graphics/runningTests.md`. The guide describes a visual smoke test; it is not a fresh verification.
- **Remaining:** `vga_display_file` has no active implementation. The demo contains an infinite animation loop. A general user-space frame submission/palette interface and Doom rendering integration remain to be established.
- **Use:** Low-level VGA and visual test reference. Compare with `graphics-dev`, which takes a different API direction.

### `graphics-dev`

Tip: `f09168f53` — 2025-04-24. **Status: partial SDL-style graphics adapter.**

- **Progress:** VGA mapping, surface allocation, palette programming, and an `SDL_Flip`-style copy path; includes `graphics_test.dir` and its expected output.
- **Evidence:** `kernel/graphics/sdl.cc`, `kernel/graphics/sdl.h`, `kernel/graphics/vga.cc`, and `graphics_test.dir/sbin/`.
- **Remaining:** `SDL_Flip` advances the destination row but repeatedly copies from the same source address, so it repeats the first row. Surface creation is named `SDL_Set_Video_Mode`, and comments explicitly identify a missing syscall boundary. This is not a complete SDL implementation.
- **Use:** Recover the surface/palette work, repair it, and reconcile its API with the actual PrBoom dependency. Its tip is not contained in `doom_integration`.

### `keyboard`

Tip: `80ecb29a0` — 2025-04-21. **Status: early keyboard event translation.**

- **Progress:** PS/2 keyboard handling and scan-code translation into SDL-style key events, with a ring-buffer interface and peripheral testcase.
- **Evidence:** `kernel/keyboard.cc`, `kernel/keyboard.h`, `kernel/sdl_keycodes.h`, `kernel/peripherals/RingBuffer.h`, and `testcases/peripherals/t0.ok`.
- **Remaining:** Validate key-up/down behavior and the event representation needed by PrBoom; combine with later input-device work. A commit message claiming working conversion does not establish runtime coverage.
- **Use:** Translation reference. Compare source with `ps2`; do not assume the newest PS/2 branch contains every commit from this branch.

### `mouseSDL`

Tip: `80ecb29a0` — 2025-04-21. **Status: exact duplicate tip of `keyboard`.**

- **Progress:** Identical tree and history to `keyboard` at this snapshot, including its keyboard/event conversion work.
- **Evidence:** Both refs resolve to the same commit. There is no separate mouse-SDL implementation to infer from this branch name.
- **Remaining:** Same integration and validation gaps as `keyboard`.
- **Use:** No additional merge is needed beyond the identical commit. Inspect `mouse` and `ps2` for distinct mouse work.

### `mouse`

Tip: `3df38ce48` — 2025-04-21. **Status: standalone PS/2 mouse driver/demo.**

- **Progress:** Mouse initialization, three-byte packet assembly, movement/buttons, position clamping, sensitivity, IRQ handling, and a continuous diagnostic test.
- **Evidence:** `kernel/mouse.cc`, `kernel/mouse.h`, `kernel/ps2.cc`, assembly/interrupt changes, and root `t0.dir`/`t0.ok`.
- **Remaining:** Reconcile controller ownership and interrupt handling with the later shared PS/2 implementation. Its 640×480 coordinate assumptions and absolute position model need adaptation to the game's input interface.
- **Use:** Driver behavior/reference; do not install competing PS/2 handlers from multiple branches.

### `ps2andwrite`

Tip: `507f81ca9` — 2025-04-27. **Status: intermediate input/storage integration.**

- **Progress:** Combines PS/2 input and writable filesystem changes, device-node work, and an initial polling implementation. Includes disk and peripheral tests.
- **Evidence:** `kernel/ps2.cc`, `kernel/ext2.h`, `kernel/poll.h`, `kernel/sys.cc`, `testcases/disk/`, and `testcases/peripherals/`.
- **Remaining:** Later input access, polling wiring, and demonstrations are on `ps2`. Merely defining a polling helper does not establish a usable user-space syscall path at this intermediate tip.
- **Use:** Already an ancestor of `ps2`; review the latter first to avoid integrating this history twice.

### `ps2`

Tip: `66c8d193e` — 2025-04-28. **Status: most developed combined input branch inspected.**

- **Progress:** Shared PS/2 controller handling, keyboard/mouse input, character/modifier handling, `/dev/input/[device]` opening, device-node polling, and inherited disk-write work. Includes hangman and polling demonstrations.
- **Evidence:** `kernel/ps2.cc`, `kernel/poll.h`, `kernel/sys.cc`, `kernel/ext2.h`, `kernel/peripherals/README.md`, and `testcases/input-interface/`.
- **Remaining:** Validate buffer/concurrency behavior and expose the events Doom expects. The README requires `DEMO=true` for hangman and `DEMO=false` for polling; the final polling demonstration intentionally never terminates. Polling and sockets must be reconciled across branches.
- **Use:** Leading input integration candidate, subject to fresh tests. Contains `ps2andwrite`, but not the latest `writing-to-disk` tip by ancestry.

### `audio`

Tip: `b1f6ab600` — 2025-04-27. **Status: AC'97 playback demo; game audio integration absent.**

- **Progress:** AC'97 setup, DMA buffer management/refill code, generated test tones, PCM/WAV loading code, and amplitude visualization.
- **Evidence:** `kernel/ac97.cc`, `kernel/ac97.h`, `kernel/kernel.cc`, `kernel/audio/`, and `testcases/audio/t1.ok`. History mentions static/beeps and explicitly calls an earlier WAV loader untested.
- **Remaining:** The branch's `kernelMain` generates a tone and enters `poll_ac97_loop`; it does not follow the normal init/game boot path. Validate DMA layout, refill timing, and actual playback, then expose a user-space audio API and connect Doom's mixer.
- **Use:** Recover the driver and focused demonstrations. Avoid adopting its replacement `kernelMain` as the integrated startup.

## Networking branches

### `networking`

Tip: `8eb578cc0` — 2025-04-22. **Status: early packet/ARP prototype, superseded by later networking history.**

- **Progress:** RTL8139 driver, Ethernet/IPv4/UDP parser work, ARP code, and interrupt plumbing.
- **Evidence:** `kernel/networking/drivers/rtl8139.cc`, `kernel/networking/protocols/parser.cc`, `kernel/networking/Arp/`, and `kernel/networking/networking.cc`.
- **Remaining:** Startup sends a fixed broadcast test frame and spins forever. The networking init testcase is largely the original filesystem/process test, not an end-to-end protocol test. Byte-order and protocol correctness need validation.
- **Use:** Historical driver/protocol foundation. Its tip is already contained in `networking_refactor`, `networking_benchmarks`, and `doom_integration`.

### `networking_sockets`

Tip: `172142258` — 2025-04-21. **Status: early UDP socket implementation, superseded by later networking history.**

- **Progress:** UDP socket objects, port table, per-process socket bookkeeping, socket syscall dispatch, and packet encode/decode integration.
- **Evidence:** `kernel/networking/socket/`, `kernel/processes.h`, `kernel/sys.cc`, and `kernel/networking/protocols/parser.cc`.
- **Remaining:** `networkingMain` still sends a fixed test frame and spins forever, preventing normal user startup through that path. Early socket code needs the later corrections and real user-space tests.
- **Use:** History/reference; its tip is already contained in `networking_refactor`, `networking_benchmarks`, and `doom_integration`.

### `networkHostDemo`

Tip: `5ce9e0c32` — 2025-04-24. **Status: early two-endpoint UDP/SDL_net demonstration.**

- **Progress:** Network-interface abstraction, hard-coded host identity, UDP socket shim, and image/file payload receive/reply demonstrations.
- **Evidence:** `kernel/networking/networking.cc`, `testcases/networking/networking.dir/sbin/init.c`, and `testcases/networking/networkingclient.dir/sbin/init.c`.
- **Remaining:** Check endpoint addresses/ports and the selected testcase together; the differently named demos do not make a self-configuring host/client setup. No Doom protocol/gameplay integration is established.
- **Use:** Reference demo, compared with the later `networkingHostTestcase` branch rather than treated as a separate protocol stack.

### `networkclient`

Tip: `37ca6fbe5` — 2025-04-24. **Status: early client-side UDP exchange demonstration.**

- **Progress:** Client identity and SDL_net-style send/receive/reply sequence for file payloads, targeting the host's port 5001.
- **Evidence:** `kernel/networking/networking.cc` and `testcases/networking/networking.dir/sbin/init.c`.
- **Remaining:** Hard-coded configuration, blocking demo flow, and payload-buffer sizing need attention. The second send copies using the first file's length, and the initial buffer is reused for a second file without resizing. Later client work is on `networking_refactor`.
- **Use:** Historical peer for the early host demo; recover test intent rather than treating it as a separate integration requirement.

### `networkingHostTestcase`

Tip: `9cba3de39` — 2025-04-28. **Status: later host-side UDP exchange test.**

- **Progress:** Dedicated host testcase opens port 5001, receives a payload, replies to its source address/port with a file, receives another payload, then shuts down. Includes later socket changes.
- **Evidence:** `testcases/networking/networkingHost.dir/sbin/init.c`, `networkingHost.ok`, `kernel/networking/socket/`, and `kernel/networking/networking.cc`.
- **Remaining:** Fresh paired execution with the client, explicit configuration/timeouts, payload assertions, and socket behavior under repeated traffic. A completed exchange is a useful milestone but is not equivalent to a Doom multiplayer session.
- **Use:** Leading host testcase to pair with the later client branch; compare shared networking files before merging.

### `networking_refactor`

Tip: `97d5f6bfd` — 2026-01-27. **Status: substantial UDP/client implementation; DHCP unfinished.**

- **Progress:** RTL8139 → network interface → ARP/IPv4/UDP → port table/socket queues → custom syscalls → SDL_net-style shim. Dedicated client exchange and DHCP testcase trees exist.
- **Evidence:** `kernel/networking/`, `testcases/networking/networkingClient.dir/sbin/`, and `testcases/networking/DHCP.dir/sbin/`. A committed client `.result` says `pass`; it is not a fresh run or proof of full coverage.
- **Remaining:** Receive UDP checksum verification is commented out in `protocols/parser.cc`. DHCP encoding places its header directly after Ethernet without the IPv4/UDP wrapping that its Ethernet type implies. The SDL_net shim only covers part of PrBoom's API. Address configuration, lengths/ownership, nonblocking behavior, and sustained bidirectional traffic need validation.
- **Use:** Main UDP recovery candidate, paired with host-side tests. The January 2026 tip only edits a TODO comment/newline; it adds no protocol functionality beyond the April 2025 work.

### `networking_benchmarks`

Tip: `3f5c6513a` — 2025-04-28. **Status: experimental benchmark/TCP work with a startup definition gap.**

- **Progress:** UDP throughput experiment code (payload sizes and PIT-jiffy timing), additional parser/receive work, TCP header/state definitions, and SYN/SYN-ACK/ACK packet construction helpers.
- **Evidence:** `kernel/networking/networking.cc`, `kernel/networking/protocols/tcp.cc`, `tcp.h`, and `parser.cc`.
- **Remaining:** Both alternative `networkingMain` implementations and the network-interface global definition are inside block comments, while `kernelMain` still calls the function. This prevents a normal link unless repaired. TCP send helpers pass null MAC addresses to the encoder; the code does not establish a complete reliable TCP transport. Benchmark code is not a set of validated measurements.
- **Use:** Mine targeted improvements and experiments after comparing against the UDP branch. PrBoom's existing network adapter uses UDP, so completing TCP is not a prerequisite for the first multiplayer milestone.

### `network_graphic`

Tip: `2a7dc3efa` — 2025-04-24. **Status: early networking/graphics merge experiment.**

- **Progress:** Merges a graphics-dev snapshot into the networking stack, including VGA/SDL-style helpers and socket-name fixes.
- **Evidence:** `kernel/graphics/`, `kernel/networking/`, `kernel/kernel.cc`, and `run_qemu_vga.sh`.
- **Remaining:** `networkingMain` sends ARP then loops forever; `kernelMain` calls it before mounting the filesystem/loading init. Thus the combined tree does not itself demonstrate the normal graphics/game startup. Its graphics snapshot also predates the current `graphics-dev` tip.
- **Use:** Prior merge reference, not a finished combined subsystem.

## Runtime, storage, memory, and shell branches

### `libc`

Tip: `a64b5fde1` — 2025-04-28. **Status: incomplete mlibc port with committed build blockers.**

- **Progress:** Vendored mlibc, `doom-new-os` sysdeps, Meson changes, sysroot artifacts, a small test program, and kernel stat/fstat work. Two commits postdate the version merged into `doom_integration`.
- **Evidence:** `mlibc/sysdeps/doom-new-os/internal-sysdeps.cpp`, `syscalls-new.cpp`, `meson.build`, `kernel/sys.cc`, `kernel/sys.h`, and `test.c`.
- **Remaining:** Resolve conflict markers in kernel syscall files. Internal sysdeps directly reference kernel allocation/debug/VMM services, TLS setup returns success without installing TLS, and futex functions are stubs. New syscall wrappers do not establish a consistent user/kernel ABI; `sys_ioctl` is empty. The generic Linux-style syscall source present in the tree is not proof that it is built or compatible.
- **Use:** Porting material requiring architectural repair and a user-space smoke test. Neither a sysroot directory nor a library build establishes that a program runs on this OS.

### `writing-to-disk`

Tip: `6dd845270` — 2025-04-28. **Status: substantial filesystem mutation work with focused tests.**

- **Progress:** IDE/block/cache write changes, ext2 mutation, file creation via open flags, `writev`, `mkdir`, and `ftruncate`.
- **Evidence:** `kernel/ide.cc`, `kernel/block_io.cc`, `kernel/buffer_cache.h`, `kernel/ext2.cc`, and `kernel/sys.cc`. Tests: `016` for writev; `018` for truncation; `037` for indirect-block writes; `043` for file creation/write/read; `044` for directory and file creation.
- **Remaining:** Fresh tests of allocation, truncation, and persistence across reboot, plus integration with VM caching and input-device nodes. Test `016` itself needs repair: it reads 43 bytes into a 42-byte array and then appends a terminator; it also uses unsigned values for negative-error checks.
- **Use:** Storage candidate. Compare against the storage code already carried by `ps2`; the latest branch tip is not fully contained there.

### `vm`

Tip: `41138390d` — 2025-04-28. **Status: expanded mapping/cache prototype with concrete correctness blockers.**

- **Progress:** mmap/munmap and ELF/thread mapping changes, shared file-page caching, and clock-style eviction work. Includes `vm_tests/page_cache_tests`, `precise_addr_mmap`, and `t2` testcase trees.
- **Evidence:** `kernel/vmm.cc`, `kernel/vme.cc`, `kernel/page_cache.cc`, `kernel/page_cache.h`, and `vm_tests/`. History describes the cache as supporting shared mappings only.
- **Remaining:** `Cache::Cache` allocates another `Cache` within its own constructor, causing unbounded recursive construction if called. `global_cache` starts null and the inspected kernel has no external initialization call. Cache writeback functions contain placeholders rather than filesystem writes. The address-based cache key/access-bit scheme also needs a correctness review before use across processes/files.
- **Use:** Recover mapping changes and tests independently from the unfinished cache; do not make this whole branch a prerequisite for the first boot/game milestone.

### `shell`

Tip: `eac0b159d` — 2025-04-28. **Status: substantial shell prototype with tests; device/storage integration pending.**

- **Progress:** Tokenization, command-tree execution, built-ins, environment-variable helpers, fork/exec, file redirection, comments, and interactive input changes. Includes `cat`/`grep` sources and four shell testcase fixtures.
- **Evidence:** `shell/src/shell.c`, `tokenizer.c`, `env_variables.c`, `shell/src/bin/`, `shell/tests/t0`–`t3`, and kernel directory-listing changes.
- **Remaining:** Reconcile its input and filesystem assumptions with `ps2` and writable ext2. Redirection closes a standard descriptor then opens a file, relying on descriptor allocation order rather than a dup2-style API. `shell/todo.md` overlaps implemented features and should not be treated as an accurate completion checklist.
- **Use:** Useful for launching/diagnosing applications after the syscall/device layer is stable; not required to prove the first Doom frame.

## Integration relationships and next milestones

Verified ancestry relationships:

- `template` and `writing-to-disk2` are ancestors of `main`; `main` is an ancestor of `doom_integration`.
- `keyboard` and `mouseSDL` are the same commit.
- `ps2andwrite` is an ancestor of `ps2`.
- `networking` and `networking_sockets` are ancestors of `networking_refactor`, `networking_benchmarks`, and `doom_integration`.
- Other branches can share or copy substantial implementation without containing each other's tips. Ancestry alone does not establish which source version is best.

Commits reachable from selected branch tips but absent from `doom_integration`:

| Branch | Commits absent |
| --- | ---: |
| PCI | 1 |
| graphics | 1 |
| graphics-dev | 20 |
| libc | 2 |
| ps2 | 47 |
| audio | 16 |
| networking_refactor | 9 |
| networkingHostTestcase | 9 |
| networking_benchmarks | 7 |
| writing-to-disk | 23 |
| vm | 36 |
| shell | 54 |

These are history counts from `git rev-list --count origin/doom_integration..origin/<branch>`, including inherited/merge history; they are not counts of independent features or patches that should all be merged.

Recommended milestones:

1. Establish a reproducible 32-bit build, ext2 image, and QEMU boot into a minimal user program. Choose a baseline based on fresh results, accounting for the integration branch's committed blockers.
2. Define one syscall ABI and user-space runtime path. Reconcile open/stat/mmap/device/socket APIs and verify a program that allocates memory, reads a file, and uses timing.
3. Bring up a frame/palette test and input-event test, then build the chosen Doom engine and reach single-player gameplay.
4. Run the host/client UDP exchange with explicit endpoint configuration, then validate PrBoom's required network API and a two-instance game session.
5. Integrate sustained audio playback and complete storage/memory regression coverage as those subsystems are adopted.
