#!/usr/bin/env python3
# drain_emu.py <game> [in] [out]: the level drain as each ORIGINAL does it -
# its own drain routine RUN in Unicorn (emu_far: the program image and the
# overlays it enters, so the hit point routine, the experience tables and the
# class values are the original's code too) on every case sweep's `behave
# levels` part `drain` wrote (DRAINMAKE: a line a case - id, levels drained,
# dice high / low, the record in the original's own layout as hex). Writes a
# line a case: id, the status the routine put the character in (0 none, 4
# collapses, 8 dies / dead forever), the record after. The engine's own drain
# is then compared case by case by DRAINCHK (behave_levels.inc).
# Not run: the screen (the words, the game's delay) and Random - its value
# the highest face (n - 1) or the lowest (0) by the case's dice, as the
# engine's die hook. Needs unicorn (/tmp/claude-0/uvenv/bin/python).
# Facts (file offsets of GAME.OVR, image offsets of the unpacked program):
#   PoolRad   0x10D7D - its words through 0x32123 and START 0x228C, the
#             fall (status) the near routine 0xECB3
#   Darkness  0x1CF3B - words 0x359A6, delay 0x550A, Random 0x97E7, fall 0x31810
#   Treasures 0x22C8E - words 0x3F993, delay 0x72FF, Random 0xBD5A, fall 0x3B119
#   Knights   0x1D089 - words 0x33BB8, delay 0x5996, Random 0xB65C, fall 0x2FD8C
# (all: the record far pointer, then the levels byte, retf 6)
import os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gb, emu_far

GAMES = {
    'poolrad': dict(exe='START.EXE', ds=0xC7C0, fn=0x10D7D, ovr_py=(0x32123,), img_py=(0x228C,), near_fall=0xECB3,
                    fall_rec=(0x10C, 0x10D, 0x11B), rnd=None),
    'darkness': dict(exe='GAME.EXE', ds=0x9E70, fn=0x1CF3B, ovr_py=(0x359A6,), img_py=(0x550A,), fall=0x31810,
                     rnd=0x97E7),
    'treasures': dict(exe='GAME.EXE', ds=0xC3E0, fn=0x22C8E, ovr_py=(0x3F993,), img_py=(0x72FF,), fall=0x3B119,
                      rnd=0xBD5A),
    'knights': dict(exe='START.EXE', ds=0xBCE0, fn=0x1D089, ovr_py=(0x33BB8,), img_py=(0x5996,), fall=0x2FD8C,
                    rnd=0xB65C),
}


def run(game, fin, fout):
    g = GAMES[game]
    img = gb.image(game, g['exe'])
    ovr = gb.raw(game, 'GAME.OVR')
    m = emu_far.Machine(img, ovr, g['ds'])
    st = {}
    mode = {'hi': 1}
    for f in g['ovr_py']:
        m.py_overlay(f, lambda m, **k: 0)
    for f in g['img_py']:
        m.py_image(f, lambda m, **k: 0)

    def fall(m, near=False):
        a = m.args(5, near)
        st['status'] = a[2] & 0xFF
        if 'fall_rec' in g:     # (PoolRad's: its record's own - unless dead, gone or stoned already, the status
            lin = a[4] * 16 + a[3]   # put in, the byte after it 0 and the hit points 0, as its 0xECB3 does)
            stat, nxt, hp = g['fall_rec']
            if m.mu.mem_read(lin + stat, 1)[0] not in (6, 7, 8):
                m.mu.mem_write(lin + stat, bytes([a[2] & 0xFF, 0]))
                m.mu.mem_write(lin + hp, b'\0')
        return 0
    if 'fall' in g:
        m.py_overlay(g['fall'], fall)
    if 'near_fall' in g:
        m.py_near_routine(g['near_fall'], fall)
    if g['rnd'] is not None:
        m.py_image(g['rnd'], lambda m, **k: (m.args(1)[0] - 1) if mode['hi'] else 0)
    out = []
    for line in open(fin):
        p = line.split()
        if not p or p[0] != 'case':
            continue
        cid, n, hi, rec = int(p[1]), int(p[2]), int(p[3]), bytes.fromhex(p[4])
        mode['hi'] = hi
        st.clear()
        m.record(0, rec)
        try:
            m.call_ovr(g['fn'], [emu_far.REC, 0, n])
        except Exception as e:      # (a run the emulator can't finish: said, not taken as a result)
            out.append('fail %d %s' % (cid, str(e).replace(' ', '_')[:80]))
            continue
        after = m.read_record(0, len(rec))
        out.append('res %d %d %s' % (cid, st.get('status', 0), after.hex()))
    open(fout, 'w').write('\n'.join(out) + '\n')
    print('%s: %d cases, %d failed' % (game, len(out), sum(1 for l in out if l.startswith('fail'))))


if __name__ == '__main__':
    game = sys.argv[1]
    d = gb.OUT
    run(game, sys.argv[2] if len(sys.argv) > 2 else os.path.join(d, 'drain_in_%s.txt' % game),
        sys.argv[3] if len(sys.argv) > 3 else os.path.join(d, 'drain_out_%s.txt' % game))
