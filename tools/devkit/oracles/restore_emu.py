#!/usr/bin/env python3
# restore_emu.py <game> [in] [out]: Restoration as each ORIGINAL does it - the
# spell's own routine (the one the Darkness family's temples call too) RUN in
# Unicorn (emu_far: the program image and the overlays it enters, so the
# experience tables, the race limits and the Constitution figures are the
# original's code) on every case sweep's `behave levels` part `restore` wrote
# (RSTMAKE: a line a case - id, two unused numbers, the drained record in the
# original's own layout as hex). The routine's target is a far pointer in the
# data segment (the spell's "on whom"); it is pointed at the case's record.
# Writes "res id 0 hex" (the record after) or "fail id why"; DRAINCHK-style
# comparison by RSTCHK (behave_levels.inc).
# Not run: the words, the class values (each record field they set is left
# out of the comparison), "able to train" and the stack check.
# Facts (GAME.OVR file offsets; img = the unpacked program's):
#   PoolRad   0x29C5B - target DS 0x6C95, race limits img 0x3AF7 (run)
#   Curse     0x31D1D - target DS 0x744F, race limits 0x3A7DD (run)
#   Gateway   0x38C29 - target DS 0x875A, race limits 0x4233E (run)
#   Darkness  0x2EBEC - target DS 0xAD3B, experience 0x39349, Constitution
#             0x37C27 (run)
#   Treasures 0x38136 - target DS 0xA428, experience 0x4398F, Constitution
#             0x41E34 (run)
#   Knights   0x2BF32 - target DS 0xC31D, the experience row 0x359C3 and
#             Constitution 0x35D99 (run), its tables at DS 0x7638
# Needs unicorn (/tmp/claude-0/uvenv/bin/python).
import os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gb, emu_far

GAMES = {
    'poolrad': dict(exe='START.EXE', ds=0xC7C0, fn=0x29C5B, target=0x6C95, ovr_py=(0x2AA87,), img_py=(0xBA8B, 0x228C)),
    'curse': dict(exe='START.EXE', ds=None, fn=0x31D1D, target=0x744F, ovr_py=(0x3B026, 0x38E2D), img_py=(0x9A32,)),
    'gateway': dict(exe='GAME.EXE', ds=None, fn=0x38C29, target=0x875A, ovr_py=(0x42C47, 0x40725), img_py=(0x3F39,),
                    stack=0x390F),
    'darkness': dict(exe='GAME.EXE', ds=0x9E70, fn=0x2EBEC, target=0xAD3B, ovr_py=(0x3836D, 0x37798), img_py=()),
    'treasures': dict(exe='GAME.EXE', ds=0xC3E0, fn=0x38136, target=0xA428, ovr_py=(0x4294B, 0x41986), img_py=(),
                      stack=0xAC4F),
    'knights': dict(exe='START.EXE', ds=0xBCE0, fn=0x2BF32, target=0xC31D, ovr_py=(0x365FF, 0x35A5E), img_py=()),
}


def data_segment(img):
    """The Turbo Pascal System unit's "mov dx,SEG; mov ds,dx" (Curse / Gateway:
    the DS base not in the table above)."""
    best = None
    p = 0
    while True:
        p = img.find(b'\x8e\xda', p + 1)
        if p < 0:
            break
        if img[p - 3] == 0xBA:
            best = struct.unpack_from('<H', img, p - 2)[0] * 16
    return best


def run(game, fin, fout):
    g = GAMES[game]
    img = gb.image(game, g['exe'])
    ovr = gb.raw(game, 'GAME.OVR')
    ds = g['ds'] or data_segment(img)
    m = emu_far.Machine(img, ovr, ds)
    for f in g['ovr_py']:
        m.py_overlay(f, lambda m, **k: 0)
    for f in g['img_py']:
        m.py_image(f, lambda m, **k: 0)
    if 'stack' in g:
        m.py_image(g['stack'], lambda m, **k: 0, argbytes=0)
    m.mu.mem_write(ds + g['target'], struct.pack('<HH', 0, emu_far.REC))
    out = []
    for line in open(fin):
        p = line.split()
        if not p or p[0] != 'case':
            continue
        cid, rec = int(p[1]), bytes.fromhex(p[4])
        m.record(0, rec)
        try:
            m.call_ovr(g['fn'], [])
        except Exception as e:      # (a run the emulator can't finish: said, not taken as a result)
            out.append('fail %d %s' % (cid, str(e).replace(' ', '_')[:80]))
            continue
        out.append('res %d 0 %s' % (cid, m.read_record(0, len(rec)).hex()))
    open(fout, 'w').write('\n'.join(out) + '\n')
    print('%s: %d cases, %d failed (DS 0x%X)' % (game, len(out), sum(1 for l in out if l.startswith('fail')), ds))


if __name__ == '__main__':
    game = sys.argv[1]
    d = gb.OUT
    run(game, sys.argv[2] if len(sys.argv) > 2 else os.path.join(d, 'restore_in_%s.txt' % game),
        sys.argv[3] if len(sys.argv) > 3 else os.path.join(d, 'restore_out_%s.txt' % game))
