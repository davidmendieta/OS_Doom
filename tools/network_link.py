"""QEMU framed-Ethernet relay: wire capture, checksum audit and repeatable loss."""
from collections import Counter
import select
import struct
import threading
import time


def checksum(data):
    if len(data) % 2:
        data += b'\0'
    value = sum(struct.unpack('!' + 'H' * (len(data) // 2), data))
    while value >> 16:
        value = (value & 0xffff) + (value >> 16)
    return ~value & 0xffff


class EthernetLink:
    def __init__(self, sockets, capture, faults=False, malformed=False):
        self.sockets = sockets
        self.capture = capture
        self.faults = faults
        self.malformed = malformed
        self.injected = set()
        self.counts = Counter()
        self.dropped = self.duplicated = self.udp = self.arp = 0
        self.error = None
        self.stop = threading.Event()
        self.thread = threading.Thread(target=self.run, daemon=True)
        self.thread.start()

    def inspect(self, frame, direction):
        if len(frame) < 14:
            raise RuntimeError('Truncated Ethernet frame')
        if frame[12:14] == b'\x08\x06':
            if len(frame) < 42 or frame[14:20] != b'\0\1\x08\0\6\4':
                raise RuntimeError('Malformed ARP on the wire')
            self.arp += 1
        if frame[12:14] != b'\x08\0':
            return None
        ip = frame[14:]
        ihl, total = (ip[0] & 15) * 4, int.from_bytes(ip[2:4], 'big')
        if ip[0] >> 4 != 4 or ihl < 20 or total > len(ip) or checksum(ip[:ihl]):
            raise RuntimeError('Invalid IPv4 length/checksum on the wire')
        if ip[9] != 17:
            return None
        udp = ip[ihl:total]
        if len(udp) < 8 or int.from_bytes(udp[4:6], 'big') != len(udp):
            raise RuntimeError('Invalid UDP length on the wire')
        pseudo = ip[12:20] + bytes((0, 17)) + udp[4:6]
        if udp[6:8] == b'\0\0' or checksum(pseudo + udp):
            raise RuntimeError('Missing/invalid UDP checksum on the wire')
        self.udp += 1
        if len(udp) >= 16 and (int.from_bytes(udp[:2], 'big') == 5030 or int.from_bytes(udp[2:4], 'big') == 5030):
            kind = udp[9]
            self.counts[direction, kind] += 1
            return kind, self.counts[direction, kind]
        return None

    def run(self):
        incoming, outgoing = [bytearray(), bytearray()], [bytearray(), bytearray()]
        for sock in self.sockets:
            sock.setblocking(False)
        try:
            with self.capture.open('wb') as pcap:
                pcap.write(struct.pack('<IHHIIII', 0xa1b2c3d4, 2, 4, 0, 0, 65535, 1))
                while not self.stop.is_set():
                    readable, writable, _ = select.select(self.sockets, [s for i, s in enumerate(self.sockets) if outgoing[i]], [], 0.05)
                    for sock in writable:
                        i = self.sockets.index(sock)
                        n = sock.send(outgoing[i])
                        del outgoing[i][:n]
                    for sock in readable:
                        i = self.sockets.index(sock)
                        data = sock.recv(65536)
                        if not data:
                            return
                        incoming[i].extend(data)
                        while len(incoming[i]) >= 4:
                            size = int.from_bytes(incoming[i][:4], 'big')
                            if not 14 <= size <= 65535:
                                raise RuntimeError(f'Invalid QEMU Ethernet frame length {size}')
                            if len(incoming[i]) < 4 + size:
                                break
                            frame = bytes(incoming[i][4:4 + size])
                            framed = bytes(incoming[i][:4 + size])
                            del incoming[i][:4 + size]
                            stamp = time.time()
                            pcap.write(struct.pack('<IIII', int(stamp), int(stamp % 1 * 1e6), size, size) + frame)
                            packet = self.inspect(frame, i)
                            if self.malformed and i not in self.injected and frame[12:14] == b'\x08\0':
                                self.injected.add(i)
                                damaged = []
                                bad = bytearray(frame); bad[24] ^= 1; damaged.append(bad)  # IPv4 checksum
                                bad = bytearray(frame); bad[40] ^= 1; damaged.append(bad)  # UDP checksum
                                bad = bytearray(frame); bad[38:40] = b'\xff\xff'; damaged.append(bad)  # UDP length
                                bad = bytearray(frame); bad[20] |= 0x20  # fragments unsupported
                                bad[24:26] = b'\0\0'
                                bad[24:26] = checksum(bytes(bad[14:34])).to_bytes(2, 'big')
                                damaged.append(bad)
                                damaged.append(frame[:41])  # truncated UDP header
                                for bad in damaged:
                                    outgoing[1-i].extend(len(bad).to_bytes(4, 'big') + bad)
                            if self.faults and packet:
                                kind, count = packet
                                # Lose the first setup/start response and bursts longer
                                # than PrBoom's one-tic redundancy, in both directions.
                                if (kind in (1, 2) and count == 1) or (kind in (3, 4) and 5 <= count <= 7):
                                    self.dropped += 1
                                    continue
                                if kind in (3, 4) and count == 12:
                                    self.duplicated += 1
                                    outgoing[1-i].extend(framed)
                            outgoing[1-i].extend(framed)
        except Exception as error:
            if not self.stop.is_set():
                self.error = error

    def close(self):
        self.stop.set()
        self.thread.join(timeout=1)
        if self.error:
            raise RuntimeError(f'Ethernet relay: {self.error}')
