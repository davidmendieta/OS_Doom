"""Verify the samples emitted by QEMU, independently of guest success messages."""
import array
import math
import struct
import sys


def read_capture(path):
    data = bytearray(path.read_bytes())
    expected = struct.pack("<4sIHHIIHH4s", b"fmt ", 16, 1, 2, 48000, 192000, 4, 16, b"data")
    if (data[:4] != b"RIFF" or data[8:12] != b"WAVE" or data[12:40] != expected
            or len(data) < 44 or (len(data) - 44) % 4):
        raise RuntimeError(f"Invalid 48 kHz stereo PCM capture: {path}")
    riff_size, pcm_size = struct.unpack_from("<I", data, 4)[0], struct.unpack_from("<I", data, 40)[0]
    # Some QEMU versions exit without finalizing the WAV backend's size fields,
    # including on QMP quit. Complete only that known header; never alter PCM.
    if riff_size == pcm_size == 0:
        struct.pack_into("<I", data, 4, len(data) - 8)
        struct.pack_into("<I", data, 40, len(data) - 44)
        path.write_bytes(data)
    elif (riff_size, pcm_size) != (len(data) - 8, len(data) - 44):
        raise RuntimeError(f"Truncated WAV capture: {path}")
    samples = array.array("h", data[44:])
    if sys.byteorder != "little":
        samples.byteswap()
    return samples


def verify_dma(path):
    samples = read_capture(path)
    frames = list(zip(samples[::2], samples[1::2]))
    runs = []
    for i, frame in enumerate(frames):
        if frame != (0, 0):
            if not runs or i > runs[-1][1] + 1:
                runs.append([i, i])
            else:
                runs[-1][1] = i
    if len(runs) != 2 or [b - a + 1 for a, b in runs] != [98304, 3073]:
        raise RuntimeError(f"PCM lost, repeated or interrupted; expected two tone runs, got {runs}")
    for (start, end), phase in zip(runs, (1, -1)):
        for i in range(end - start + 1):
            expected = (phase * (4000 if i % 100 < 50 else -4000),
                        phase * (2000 if i % 64 < 32 else -2000))
            if frames[start + i] != expected:
                raise RuntimeError(f"PCM mismatch at frame {start + i}: {frames[start + i]} != {expected}")
    if runs[1][0] - runs[0][1] < 4800 or len(frames) - runs[1][1] < 4800:
        raise RuntimeError("Missing silence after DMA drains")
    print(f"Audio DMA passed: all 101377 stereo frames match, ring wraps and silent underruns verified. WAV: {path}")


def correlation(reference, values):
    mean = sum(values) / len(values)
    centered = [v - mean for v in values]
    power = sum(v * v for v in centered)
    if not power:
        return 0
    return sum(a * b for a, b in zip(reference, centered)) / math.sqrt(power)


def normalized(values):
    mean = sum(values) / len(values)
    centered = [v - mean for v in values]
    norm = math.sqrt(sum(v * v for v in centered))
    return [v / norm for v in centered]


def verify_doom(path, wad, serial):
    samples = read_capture(path)
    left, right = samples[::2], samples[1::2]
    if not samples or max(map(abs, samples)) < 512:
        raise RuntimeError("Doom capture is silent or inaudibly quiet")
    # Menu clicks and the player's pistol are centered in the sound field.
    # Match their waveform to original IWAD PCM with an independent float
    # resampler, allowing a few frames of scheduling/alignment difference.
    data = wad.read_bytes()
    _, count, directory = struct.unpack_from("<4sII", data)
    lumps = {}
    for i in range(count):
        offset, size, name = struct.unpack_from("<II8s", data, directory + 16 * i)
        lumps[name.rstrip(b"\0")] = data[offset:offset + size]
    for name in ("swtchn", "pistol"):
        if f"*** sound: {name} " not in serial:
            raise RuntimeError(f"Engine did not start {name}")
        raw = lumps[("ds" + name).upper().encode()]
        fmt, rate, length = struct.unpack_from("<HHI", raw)
        if fmt != 3 or length > len(raw) - 8:
            raise RuntimeError(f"Invalid IWAD sound: {name}")
        def reference(frame):
            position = frame * rate / 48000
            index = int(position)
            fraction = position - index
            return raw[8 + index] * (1 - fraction) + raw[9 + index] * fraction - 128
        offsets = range(96, 480, 8)
        short = normalized([reference(i) for i in offsets])
        candidates = []
        for start in range(0, len(left) - 2048, 8):
            if correlation(short, [left[start + i] for i in offsets]) > 0.97:
                candidates.append(start)
        long_offsets = range(0, 2048, 8)
        full = normalized([reference(i) for i in long_offsets])
        best = 0
        for candidate in candidates:
            for start in range(max(0, candidate - 7), min(candidate + 8, len(left) - 2048)):
                score = min(correlation(full, [channel[start + i] for i in long_offsets])
                            for channel in (left, right))
                best = max(best, score)
        if best < 0.995:
            raise RuntimeError(f"Captured {name} does not match IWAD waveform (correlation {best:.5f})")
        print(f"Doom {name} sound matches IWAD in both output channels (correlation {best:.5f}).")
    print(f"Doom sound effects passed. WAV: {path}")
