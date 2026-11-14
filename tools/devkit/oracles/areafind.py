#!/usr/bin/env python3
# areafind.py [game ...]  (darkness treasures knights; none: all three)
# Rules round 23 - the computer's area spells' centre in Pools of Darkness,
# Treasures of the Savage Frontier and Death Knights of Krynn: each game's
# area finder (Darkness GAME.OVR 0x22976, Treasures 0x2AC37, Death Knights
# 0x23207) RUN in Unicorn (emu16.py) on random
# fields: fighters of both sides at random squares, a caster, a spell of
# the finder's own sets (34, 91, 101) or another, the radius (its row's aim
# - 8, poked into the data segment's row), the side asked (the row's byte),
# the reach (its range helper's answer) and, past 18, the caster's target.
# Its helpers answered here: the use test (the near call 0x249a / 0xA74B):
# 1 on the side asked, -10 on the other (what the engine's ai_use gives for
# these states - the check asks it too); the random 25% never; the sight
# test: no walls, the engine's path length (2 a straight step, 3 a
# diagonal) within 2 x reach + 1. Needs unicorn (emu16.py says how).
# Output /tmp/claude-0/bt/oracle_areafind_<game>.txt, a line each:
#   area <spell> <radius> <own> <reach> <caster> <target|-1> <n> <side x y> x n -> <x> <y> | none
import os, random, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gb
import emu16
from unicorn.x86_const import UC_X86_REG_DX, UC_X86_REG_EFLAGS, UC_X86_REG_SP

GAMES = {
    'darkness': dict(prog=('darkness', 'GAME.EXE'), ds=0x9E70, fn=0x22976, seg=0x1E642, use=0x2249A,
                     cur=0x9CEE, head=0x9CF2, block=0x87F8, rows=0x6519,
                     xy=(0x135, 0x66, 0x6B), side=(0xEC, 0xB6), rng=(0xBB, 0x2F), alloc=(0x825, 0x23F),
                     fill=(0x670, 0x46E), inset=(0x825, 0xD24), rnd=(0xEC, 0x25), path=(0x130, 0x39),
                     free=(0x825, 0x254), check=None, fillb=6, pathx=0, rec=(0x1EE, 0x1EF, 0x1E3, 0x1E7, 9),
                     flag=0x21),
    'treasures': dict(prog=('treasures', 'GAME.EXE'), ds=0xC3E0, fn=0x2AC37, seg=0x26642, use=0x2A74B,
                      cur=0x9308, head=0x930C, block=0x7E10, rows=0x5789,
                      xy=(0x150, 0x66, 0x6B), side=(0x105, 0xB6), rng=(0xD4, 0x2F), alloc=(0xA77, 0x23F),
                      fill=(0x86B, 0x74A), inset=(0xA77, 0xD77), rnd=(0x105, 0x25), path=(0x14B, 0x39),
                      free=(0xA77, 0x254), check=(0xA77, 0x4DF), fillb=6, pathx=0, rec=(0x1EE, 0x1EF, 0x1E3, 0x1E7, 9),
                      flag=0x21),
    # (Death Knights GAME.OVR 0x23207: a half of 9, no sets; its records'
    # active / side / next / combat block, its target at +0xA; the sight
    # test takes the map's far pointer on top)
    'knights': dict(prog=('knights', 'START.EXE'), ds=0xBCE0, fn=0x23207, seg=0x1ED0F, use=0x22CE4,
                    cur=0xA2C2, head=0xA2C6, block=0x8C2D, rows=0x6A7F,
                    xy=(0x102, 0x66, 0x6B), side=(0xCF, 0xBB), rng=(0xA3, 0x2F), alloc=(0xA07, 0x23F),
                    fill=(0xA07, 0x1AD4), inset=None, rnd=(0xCF, 0x25), path=(0xFD, 0x39),
                    free=(0xA07, 0x254), check=None, fillb=8, pathx=2, rec=(0xC8, 0xC9, 0xB9, 0xBD, 0xA),
                    flag=0xE6),
}
REC0, CMB0, BUF, BLK = 0x8000, 0x9000, 0x9800, 0x9A00


def path_len(x0, y0, x, y):
    # (the engine's path_xy, no walls: 2 a straight step, 3 a diagonal)
    ddx, ddy = abs(x - x0), abs(y - y0)
    sx, sy = (1 if x > x0 else -1), (1 if y > y0 else -1)
    err, n = ddx - ddy, 0
    while x0 != x or y0 != y:
        e2, mx, my = 2 * err, False, False
        if e2 > -ddy:
            err -= ddy
            x0 += sx
            mx = True
        if e2 < ddx:
            err += ddx
            y0 += sy
            my = True
        n += 3 if mx and my else 2
    return n


def run_game(game, out):
    g = GAMES[game]
    code = open(gb.find(game, 'GAME.OVR'), 'rb').read()
    img = gb.image(*g['prog'])
    r = emu16.Routine(code, g['seg'], img, g['ds'], pascal=True)
    mu = r.mu
    st = {}

    def rec_of(seg):
        return (seg - REC0) // 0x40

    def h_xy(which):
        def h(e):
            w = e.args(2)
            k = rec_of(w[1])
            return st['f'][k][1 + which] & 0xFF
        return h

    def h_side(e):
        return 1 - st['f'][st['caster']][0]

    def h_alloc(e):
        mu.reg_write(UC_X86_REG_DX, BUF)
        return 0

    def h_fill(e):
        w = e.args(3)                           # (far ptr, size): pushed ptr then size
        mu.mem_write(BUF * 16, b'\0' * 0x559)
        return 0

    def h_inset(e):
        w = e.args(3)                           # (spell, far set): on top the set's offset, seg, then the spell
        off, seg, sp = w[0], w[1], w[2] & 0xFF
        b = mu.mem_read(seg * 16 + off + (sp >> 3), 1)[0]
        member = (b >> (sp & 7)) & 1
        fl = mu.reg_read(UC_X86_REG_EFLAGS)
        mu.reg_write(UC_X86_REG_EFLAGS, (fl & ~0x40) | (0 if member else 0x40))
        return 0

    def h_path(e):
        w = e.args(8 + g['pathx'])[g['pathx']:]     # tx, ty, &cx far, &cy far, &range far - on top &range
        roff, rseg, yoff, yseg, xoff, xseg, ty, tx = w
        rng = struct.unpack('<H', mu.mem_read(rseg * 16 + roff, 2))[0]
        cx = mu.mem_read(xseg * 16 + xoff, 1)[0]
        cy = mu.mem_read(yseg * 16 + yoff, 1)[0]
        tx, ty = struct.unpack('<h', struct.pack('<H', tx))[0], struct.unpack('<h', struct.pack('<H', ty))[0]
        return 1 if path_len(cx, cy, tx, ty) <= 2 * rng + 1 else 0

    r.helper(g['xy'][0], g['xy'][1], h_xy(0), 4)
    r.helper(g['xy'][0], g['xy'][2], h_xy(1), 4)
    r.helper(*g['side'], h_side, 4)
    r.helper(*g['rng'], lambda e: st['reach'], 2)
    r.helper(*g['alloc'], h_alloc, 2)
    r.helper(*g['fill'], h_fill, g['fillb'])
    if g['inset']:
        r.helper(*g['inset'], h_inset, 6)
    r.helper(*g['rnd'], lambda e: 0, 2)
    r.helper(*g['path'], h_path, 16 + 2 * g['pathx'])
    r.helper(*g['free'], lambda e: 0, 6)
    if g['check']:
        r.helper(*g['check'], lambda e: 0, 0)
    # the use test: push cs + a near call - a far Pascal call (target far, spell, side), retf 8
    lin_use = emu16.CS * 16 + g['use'] - g['seg']
    mu.mem_write(lin_use, b'\xcd\x30\xca\x08\x00')

    def h_use(e):
        sp = mu.reg_read(UC_X86_REG_SP)
        side, spell, off, seg = struct.unpack('<HHHH', mu.mem_read(emu16.SS * 16 + sp + 4, 8))
        k = rec_of(seg)
        return 1 if st['f'][k][0] == (side & 0xFF) else 0xFFF6
    r.handlers[lin_use] = h_use

    rnd = random.Random(23)
    lines = []
    spells = [34, 91, 101, 0x2D, 0x45] if g['inset'] else [0x22, 0x2D, 0x45, 0x50]
    for case in range(3000):
        n = rnd.randint(2, 12)
        f = []
        used = set()
        cx0, cy0 = rnd.randint(4, 45), rnd.randint(3, 20)
        for k in range(n):
            while True:
                x = max(0, min(49, cx0 + rnd.randint(-12, 12)))
                y = max(0, min(23, cy0 + rnd.randint(-8, 8)))
                if (x, y) not in used:
                    break
            used.add((x, y))
            f.append((rnd.randint(0, 1), x, y))
        caster = rnd.randrange(n)
        spell = rnd.choice(spells)
        rad = rnd.randint(1, 3)
        own = rnd.random() < 0.25
        reach = rnd.choice([1, 2, 3, 4, 6, 8, 10, 12, 16, 20])
        target = rnd.randrange(n) if rnd.random() < 0.6 else -1
        st.update(f=f, caster=caster, reach=reach)
        # the row: aim 8 + radius, the side byte (0: its own)
        row = g['ds'] + g['rows'] + spell * 16
        r.poke_ds(g['rows'] + spell * 16 + 7, bytes([8 + rad]))
        r.poke_ds(g['rows'] + spell * 16 + 15, bytes([0 if own else 1]))
        # records: active, next, side, combat block (+9: its target)
        ra, rs, rn, rc, rt = g['rec']
        for k, (sd, x, y) in enumerate(f):
            rec = bytearray(0x400)
            rec[ra] = 1
            rec[rs] = sd
            nxt = (0, REC0 + (k + 1) * 0x40) if k + 1 < n else (0, 0)
            rec[rn:rn + 4] = struct.pack('<HH', *nxt)
            rec[rc:rc + 4] = struct.pack('<HH', 0, CMB0 + k * 0x10)
            mu.mem_write((REC0 + k * 0x40) * 16, bytes(rec))
            cmb = bytearray(0x100)
            if k == caster and target >= 0:
                cmb[rt:rt + 4] = struct.pack('<HH', 0, REC0 + target * 0x40)
            mu.mem_write((CMB0 + k * 0x10) * 16, bytes(cmb))
        r.poke_ds(g['cur'], struct.pack('<HH', 0, REC0 + caster * 0x40))
        r.poke_ds(g['head'], struct.pack('<HH', 0, REC0))
        blk = bytearray(0x200)
        blk[g['flag']] = 1                      # (indoors: the fireballs' radius as it is)
        mu.mem_write(BLK * 16, bytes(blk))
        r.poke_ds(g['block'], struct.pack('<HH', 0, BLK))
        # (the result: a far pointer, then x, y - Pascal: spell pushed first)
        outp = 0xF000
        mu.mem_write(emu16.SS * 16 + outp, b'\0' * 8)
        ax = r.call(g['fn'], [spell, emu16.SS, outp])
        res = mu.mem_read(emu16.SS * 16 + outp, 6)
        got = '%d %d' % (res[4], res[5]) if ax & 0xFF else 'none'
        lines.append('area %d %d %d %d %d %d %d %s -> %s' % (
            spell, rad, 1 if own else 0, reach, caster, target, n, ' '.join('%d %d %d' % t for t in f), got))
    with open(out, 'w') as fo:
        fo.write('# areafind oracle (%s) - tools/devkit/oracles/areafind.py\n' % game)
        fo.write('\n'.join(lines) + '\n')
    print('%s: %d lines -> %s (%d with a centre)' % (game, len(lines), out, sum('none' not in l for l in lines)))


def run_queen(out):
    """The Dark Queen's finder (DQK.EXE 0x2960A, w0's segment 0x21E4 at file
    0x24D60, DS:0 at 0x63A20; Borland C: the caller pops): the same draw, its
    globals - the caster [0xAD67], the fighters' list [0xC6E5] (next at +0),
    the grid [0xC57C] (words), the game block [0xB20F] (+0x22 the
    fireballs' flag, +0x3C under water) - and its records' active byte
    0x18B, side word 0x66, combat block +0x40 (its target at +0xB)."""
    emu16.CS = 0xA000                       # (its helpers' segments 0x1D99 / 0x1093 sit where emu16 puts code)
    code = open(gb.find('queen', 'DQK.EXE'), 'rb').read()
    r = emu16.Routine(code, 0x24D60, code, 0x63A20, pascal=False)
    mu = r.mu
    st = {}
    GRID = 0x9C00

    def rec_of(seg):
        return (seg - REC0) // 0x40

    def h_xy(which):
        def h(e):
            w = e.args(2)                   # (C: the record far - offset on top)
            return st['f'][rec_of(w[1])][1 + which] & 0xFF
        return h

    def h_path(e):
        tx, ty, xoff, xseg, yoff, yseg, roff, rseg = e.args(8)
        rng = mu.mem_read(rseg * 16 + roff, 1)[0]
        cx = mu.mem_read(xseg * 16 + xoff, 1)[0]
        cy = mu.mem_read(yseg * 16 + yoff, 1)[0]
        sx = lambda v: v - 256 if v & 0x80 else v
        return 1 if path_len(cx, cy, sx(tx & 0xFF), sx(ty & 0xFF)) <= 2 * rng + 1 else 0

    def h_fill(e):
        mu.mem_write(GRID * 16, b'\0' * 0xAB2)
        return 0

    r.helper(0x2740, 0xAEA, h_xy(0))
    r.helper(0x2740, 0xB19, h_xy(1))
    r.helper(0x20CB, 0x8FD, lambda e: 1 - st['f'][st['caster']][0])
    r.helper(0x1D99, 0x10C1, lambda e: st['reach'])
    r.helper(0x1093, 0x15, h_fill)
    r.helper(0x77E, 0x2164, lambda e: 0)
    r.helper(0x1D99, 0x10D5, h_path)
    lin_use = emu16.CS * 16 + 0x2902C - 0x24D60
    mu.mem_write(lin_use, b'\xcd\x30\xcb')     # (push cs + a near call: a far C call, retf)

    def h_use(e):
        off, seg, spell, side = e.args(4)
        return 1 if st['f'][rec_of(seg)][0] == (side & 0xFF) else 0xFFD8     # (-40: the Queen's wrong side)
    r.handlers[lin_use] = h_use
    rnd = random.Random(23)
    lines = []
    for case in range(3000):
        n = rnd.randint(2, 12)
        f, used = [], set()
        cx0, cy0 = rnd.randint(4, 45), rnd.randint(3, 20)
        for k in range(n):
            while True:
                x = max(0, min(49, cx0 + rnd.randint(-12, 12)))
                y = max(0, min(23, cy0 + rnd.randint(-8, 8)))
                if (x, y) not in used:
                    break
            used.add((x, y))
            f.append((rnd.randint(0, 1), x, y))
        caster = rnd.randrange(n)
        spell = rnd.choice([0x22, 0x65, 0x2D, 0x45])
        rad = rnd.randint(1, 3)
        own = rnd.random() < 0.25
        reach = rnd.choice([1, 2, 3, 4, 6, 8, 10, 12, 16, 20])
        target = rnd.randrange(n) if rnd.random() < 0.6 else -1
        st.update(f=f, caster=caster, reach=reach)
        r.poke_ds(0x27E9 + spell * 22 + 7, bytes([8 + rad]))
        r.poke_ds(0x27E9 + spell * 22 + 0x13, struct.pack('<H', 0 if own else 1))
        for k, (sd, x, y) in enumerate(f):
            rec = bytearray(0x400)
            rec[0x18B] = 1
            rec[0x66:0x68] = struct.pack('<H', sd)
            nxt = (0, REC0 + (k + 1) * 0x40) if k + 1 < n else (0, 0)
            rec[0:4] = struct.pack('<HH', *nxt)
            rec[0x40:0x44] = struct.pack('<HH', 0, CMB0 + k * 0x10)
            mu.mem_write((REC0 + k * 0x40) * 16, bytes(rec))
            cmb = bytearray(0x100)
            if k == caster and target >= 0:
                cmb[0xB:0xF] = struct.pack('<HH', 0, REC0 + target * 0x40)
            mu.mem_write((CMB0 + k * 0x10) * 16, bytes(cmb))
        r.poke_ds(0xAD67, struct.pack('<HH', 0, REC0 + caster * 0x40))
        r.poke_ds(0xC6E5, struct.pack('<HH', 0, REC0))
        r.poke_ds(0xC57C, struct.pack('<HH', 0, GRID))
        blk = bytearray(0x100)
        blk[0x22] = 1
        mu.mem_write(BLK * 16, bytes(blk))
        r.poke_ds(0xB20F, struct.pack('<HH', 0, BLK))
        outp = 0xF000
        mu.mem_write(emu16.SS * 16 + outp, b'\0' * 8)
        # (C: spell, then the result's far pointer - pushed last first)
        ax = r.call(0x2960A, [emu16.SS, outp, spell])
        res = mu.mem_read(emu16.SS * 16 + outp, 8)
        got = '%d %d' % (res[4], res[6]) if ax & 0xFF else 'none'
        lines.append('area %d %d %d %d %d %d %d %s -> %s' % (
            spell, rad, 1 if own else 0, reach, caster, target, n, ' '.join('%d %d %d' % t for t in f), got))
    with open(out, 'w') as fo:
        fo.write('# areafind oracle (queen) - tools/devkit/oracles/areafind.py\n')
        fo.write('\n'.join(lines) + '\n')
    print('queen: %d lines -> %s (%d with a centre)' % (len(lines), out, sum('none' not in l for l in lines)))


if __name__ == '__main__':
    os.makedirs(gb.OUT, exist_ok=True)
    for game in sys.argv[1:] or list(GAMES) + ['queen']:
        if game == 'queen':
            run_queen(os.path.join(gb.OUT, 'oracle_areafind_queen.txt'))
        else:
            run_game(game, os.path.join(gb.OUT, 'oracle_areafind_%s.txt' % game))
