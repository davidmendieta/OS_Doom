"""Observe actual game state in both VMs while driving independent inputs."""
import re
import time

STATE = re.compile(r'\*\*\* netstate: tic=(\d+) player=(\d+) x=(-?\d+) y=(-?\d+) angle=(\d+) health=(-?\d+) ammo=(\d+) forward=(-?\d+) buttons=(\d+)')


def states(log):
    return {(int(m[0]), int(m[1])): tuple(map(int, m[2:])) for m in STATE.findall(log.read_text(errors='replace'))}


def key(qmp, code, down):
    qmp.command('input-send-event', events=[{'type': 'key', 'data': {'down': down, 'key': {'type': 'qcode', 'data': code}}}])


def check_doom_network(processes, qmps, logs, build, deadline, abrupt_disconnect=False):
    def wait_for(predicate, message):
        while time.monotonic() < deadline:
            outputs = [log.read_text(errors='replace') for log in logs]
            if any(p.poll() is not None for p in processes) or any('halting' in s or 'I_Error' in s or 'consistency failure' in s for s in outputs):
                raise RuntimeError(f'Doom guest failed; see {build}')
            snapshots = [states(log) for log in logs]
            if predicate(snapshots):
                return snapshots
            time.sleep(0.02)
        raise RuntimeError(f'{message}; see {build}')

    snapshots = wait_for(lambda s: all(len(v) >= 20 for v in s), 'Multiplayer game did not start')
    # Two separately controlled players must move and fire, and both simulations
    # must agree on both players at the same game tic (not the same wall time).
    begin = max(t for s in snapshots for t, p in s)
    key(qmps[0], 'up', True)
    key(qmps[1], 'down', True)
    key(qmps[0], 'ctrl', True)
    key(qmps[1], 'ctrl', True)
    snapshots = wait_for(lambda s: all(max(t for t, p in v) >= begin + 60 for v in s), 'Game stalled during movement')
    for q in qmps:
        key(q, 'up', False)
        key(q, 'down', False)
        key(q, 'ctrl', False)
    snapshots = wait_for(lambda s: all(max(t for t, p in v) >= begin + 100 for v in s), 'Game stalled after input')
    common = sorted(set(snapshots[0]) & set(snapshots[1]))
    if len(common) < 80:
        raise RuntimeError(f'Only {len(common)} shared player/tic observations')
    for k in common:
        if snapshots[0][k] != snapshots[1][k]:
            raise RuntimeError(f'Game diverged at tic/player {k}: {snapshots[0][k]} vs {snapshots[1][k]}')
    for player in (0, 1):
        observed = [snapshots[0][k] for k in common if k[1] == player]
        if len({v[:2] for v in observed}) < 5 or not any(v[4] < 50 for v in observed) or not any(v[5] != 0 for v in observed):
            raise RuntimeError(f'Player {player + 1} did not move and fire in both simulations')
    for i, q in enumerate(qmps):
        q.command('screendump', filename=str(build / ('host' if i == 0 else 'join') / 'frame.png'), format='png')
    print(f'Doom network PASS: {len(common)} matching player/tic states; both players moved and fired. Screenshots/logs: {build}')
    before = max(t for t, p in states(logs[0]))
    if abrupt_disconnect:
        qmps[1].command('quit')
    else:
        for code in ('f10', 'y'):
            key(qmps[1], code, True)
            time.sleep(0.15)
            key(qmps[1], code, False)
            time.sleep(0.15)
    while time.monotonic() < deadline:
        output = logs[0].read_text(errors='replace')
        if processes[0].poll() is not None or 'halting' in output:
            raise RuntimeError('Host failed after peer disconnected')
        if processes[1].poll() is not None and 'ending network session' in output:
            if max(t for t, p in states(logs[0])) > before + 20:
                print('Disconnect PASS: host resumes local play after ' + ('peer timeout.' if abrupt_disconnect else 'peer quits.'))
                return
        time.sleep(0.05)
    raise RuntimeError('Host did not recover after peer disconnected')
