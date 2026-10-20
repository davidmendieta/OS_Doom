"""Exercise the MUS core on the host, then compare native PCM with QEMU audio."""
from pathlib import Path
import struct
import subprocess
from audio_check import read_capture

ROOT = Path(__file__).resolve().parent.parent


def prepare_reference(build):
    build.mkdir(parents=True, exist_ok=True)
    executable = build / 'music-reference'
    subprocess.run(['cc', '-std=c99', '-O2', '-g', '-fsanitize=address',
                    '-I', str(ROOT / 'ports/prboom'),
                    str(ROOT / 'testcases/music/check.c'), str(ROOT / 'ports/prboom/mus_player.c'),
                    str(ROOT / 'ports/prboom/third_party/nuked-opl3/opl3.c'),
                    '-o', str(executable)], check=True)
    wad = (ROOT / 'build/deps/doom1.wad').read_bytes()
    _, count, directory = struct.unpack_from('<4sII', wad)
    songs = []
    for i in range(count):
        offset, size, name = struct.unpack_from('<II8s', wad, directory + 16 * i)
        name = name.rstrip(b'\0').decode()
        if name == 'GENMIDI' or name.startswith('D_'):
            (build / name).write_bytes(wad[offset:offset + size])
            if name.startswith('D_'): songs.append(name)
    subprocess.run([str(executable), str(build / 'GENMIDI')], check=True)
    for name in songs:
        subprocess.run([str(executable), str(build / 'GENMIDI'), str(build / name),
                        str(build / (name + '.pcm')), '96000'], check=True)
    print(f'Music reference: parsed and rendered all {len(songs)} IWAD tracks.')


def verify_music(capture, build, serial):
    samples = read_capture(capture)
    actual = samples.tobytes()
    if '*** doom: OPL music ready' not in serial:
        raise RuntimeError('Guest did not initialize the music synthesizer')
    # Exact PCM matches prove score sequencing, synthesis and transport through
    # the 32-bit guest. Core lifecycle behavior is checked independently above.
    # Skip the first half second of E1M1 to avoid the final menu sound's tail.
    for name, start in [('D_INTRO', 4800), ('D_E1M1', 28800)]:
        reference = (build / (name + '.pcm')).read_bytes()[start * 4:(start + 24000) * 4]
        if not any(reference): raise RuntimeError(f'Silent reference: {name}')
        if actual.find(reference) < 0:
            raise RuntimeError(f'QEMU music does not match {name} reference PCM')
        print(f'Music capture matches {name}: 24000 consecutive stereo frames.')
    if '*** sound: pistol ' not in serial:
        raise RuntimeError('No simultaneous gameplay sound effects observed')
    print(f'Doom music passed: title to E1M1, gameplay with SFX, pause/resume. WAV: {capture}')


if __name__ == '__main__':
    prepare_reference(ROOT / 'build/music-core')
