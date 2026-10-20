#!/usr/bin/env python3
"""Run two OS guests on an isolated Ethernet link (no host IP configuration)."""
import argparse
from contextlib import ExitStack
import os
from pathlib import Path
import socket
import subprocess
import sys
import time

from run import ROOT, build_filesystem, tool
from graphics_check import QMP
from network_link import EthernetLink


def command(disk, cpus, display, audio, link, index, serial):
    return [tool(os.environ.get('QEMU_CMD', 'qemu-system-i386')),
            '-name', f'Doom player {index + 1}', '-machine', 'pc', '-accel', 'tcg', '-cpu', 'max',
            '-smp', str(cpus), '-m', '128M', '-no-reboot', '-snapshot',
            '-display', display, '-vga', 'std', '-monitor', 'none', '-serial', f'file:{serial}',
            '-drive', f'file={ROOT / "kernel/build/kernel.img"},index=0,media=disk,format=raw',
            '-drive', f'file={disk},index=1,media=disk,format=raw',
            '-device', 'isa-debug-exit,iobase=0xf4,iosize=0x04',
            '-netdev', f'socket,id=net0,fd={link}',
            '-device', f'rtl8139,netdev=net0,mac=52:54:00:12:34:{index + 1:02x}',
            '-audiodev', f'{audio},id=audio0', '-device', 'AC97,audiodev=audio0', '-qmp', 'stdio']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--program', choices=('net-smoke', 'doom'), default='doom')
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--faults', action='store_true', help='test handshake retries, lost tick bursts and duplicates')
    parser.add_argument('--cpus', type=int, default=4)
    parser.add_argument('--timeout', type=float, default=60)
    parser.add_argument('--display', default='none')
    parser.add_argument('--audio', default=None)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build/network')
    args = parser.parse_args()
    if not 1 <= args.cpus <= 16 or args.timeout <= 0:
        parser.error('CPUs must be 1..16 and timeout must be positive')
    if args.faults and (not args.check or args.program != 'doom'):
        parser.error('--faults requires a Doom --check run')
    build = args.build_dir.resolve()
    disks = [build_filesystem(args.program, build / role, args.check and args.program == 'doom',
                             network=f'{i+1} 0a00000{i+1} 0a000001') for i, role in enumerate(('host', 'join'))]
    audio = args.audio or ('none' if args.check else 'coreaudio' if sys.platform == 'darwin' else 'sdl')
    with ExitStack() as stack:
        pairs = [[stack.enter_context(s) for s in socket.socketpair()] for _ in range(2)]
        links = [pair[0] for pair in pairs]
        relay = EthernetLink([pair[1] for pair in pairs], build / 'network.pcap', args.faults, args.program == 'net-smoke')
        stack.callback(relay.close)
        processes, qmps, logs = [], [], []
        deadline = time.monotonic() + args.timeout
        try:
            for i, role in enumerate(('host', 'join')):
                directory = build / role
                log = directory / 'serial.log'
                log.write_text('')
                logs.append(log)
                errors = stack.enter_context((directory / 'qemu.log').open('w'))
                process = subprocess.Popen(command(disks[i], args.cpus, args.display, audio, links[i].fileno(), i, log),
                                           pass_fds=(links[i].fileno(),), stdin=subprocess.PIPE,
                                           stdout=subprocess.PIPE, stderr=errors, bufsize=0)
                processes.append(process)
                qmps.append(QMP(process, deadline))
            for link in links:
                link.close()
            if args.program == 'net-smoke':
                while time.monotonic() < deadline:
                    serial = [p.read_text(errors='replace') for p in logs]
                    if '*** net: PASS' in serial[1] and processes[1].poll() == 1:
                        if relay.error: raise RuntimeError(str(relay.error))
                        if relay.udp < 600 or relay.arp < 2: raise RuntimeError('Missing Ethernet traffic')
                        print(serial[1][serial[1].index('*** net: PASS'):].splitlines()[0])
                        print(f'Wire audit PASS: {relay.udp} UDP checksums, {relay.arp} ARP packets. Capture: {relay.capture}')
                        return 0
                    if any('Assertion' in s or 'halting' in s or 'FAIL' in s for s in serial) or any(p.poll() is not None for p in processes):
                        raise RuntimeError(f'Network guest failed; see {build}')
                    time.sleep(0.02)
                raise RuntimeError(f'Network test timed out; see {build}')
            if args.check:
                from network_check import check_doom_network
                check_doom_network(processes, qmps, logs, build, deadline, args.faults)
                if relay.error: raise RuntimeError(str(relay.error))
                if args.faults and (relay.dropped < 8 or relay.duplicated < 2):
                    raise RuntimeError('Fault injection did not exercise both directions')
                print(f'Wire audit PASS: {relay.udp} UDP packets; {relay.dropped} deliberately dropped, {relay.duplicated} duplicated.')
            else:
                print(f'Two-player cooperative E1M1. Logs: {build}', flush=True)
                while all(p.poll() is None for p in processes):
                    time.sleep(0.1)
            return 0
        finally:
            for p in processes:
                if p.poll() is None:
                    p.terminate()
                try:
                    p.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    p.kill()
                    p.wait()
                if p.stdin: p.stdin.close()
                if p.stdout: p.stdout.close()


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(f'error: {error}', file=sys.stderr)
        sys.exit(1)
    except KeyboardInterrupt:
        sys.exit(130)
