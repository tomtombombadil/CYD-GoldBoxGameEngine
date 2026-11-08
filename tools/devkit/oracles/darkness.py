#!/usr/bin/env python3
# The oracles of behave darkness / behave treasures (behave_darkness.inc,
# _fights.inc, _left.inc): oracle_<game>.txt, oracle_<game>_fights.txt,
# oracle_<game>_left.txt. Usage: darkness.py [darkness|treasures]
#
# Tables from the player's GAME.EXE (unpacked; data segment Darkness image
# 0x9E70, Treasures 0xC3E0) at the profile's addresses, ITEM0.DAX block 1
# (the kit's items), GAME.OVR's code (the spell handlers' words); the rules
# worked out here from the Project's facts (darkness_create_facts.md 40-49,
# darkness_fight_rules_facts.md R1-R16, darkness_left_facts.md L1-L9;
# Treasures: treasures_port_facts.md - Darkness's rules at Treasures'
# places: conhp DS 0x6423, turn 0x0B33, meteor 0x0D9D, kit 0x0AE7) - never
# from the engine's code. Section numbers below are the facts'.
import re, struct, subprocess, sys
import gb

GAME = sys.argv[1] if len(sys.argv) > 1 else 'darkness'
if GAME == 'darkness':
    BASE = 0x9E70
    T = dict(spells=0x6519, thac0=0x6D18, flags=0x6DB2, masks=0x6DB9, lasthd=0x6DC9, hpcount=0x6DD0, hpsides=0x6DD7,
             limits=0x6EF7, raceclasses=0x6F57, ages=0x6FAB, brackets=0x7053, classmin=0x708F, align=0x70F5,
             records=0x71AE, xpstep=0x6EDB, conhp=0x719C, turn=0x09FB, meteor_x=0x0C66, meteor_y=0x0C6A, kit=0x09B7,
             reserve=0x08D8)
else:
    BASE = 0xC3E0
    T = dict(spells=0x5789, thac0=0x5F98, flags=0x6032, masks=0x603A, lasthd=0x604C, hpcount=0x6054, hpsides=0x605C,
             limits=0x617E, raceclasses=0x61DE, ages=0x6232, brackets=0x62DA, classmin=0x6316, align=0x637C,
             records=0x6436, xpstep=0x6162, conhp=0x6423, turn=0x0B33, meteor_x=0x0D9E, meteor_y=0x0DA2, kit=0x0AE7,
             reserve=None)
ds = gb.DS(gb.image(GAME, 'GAME.EXE'), BASE)
OVR = gb.find(GAME, 'GAME.OVR')
ovr = open(OVR, 'rb').read()

masks = list(ds.bytes(T['masks'], 7))
lasthd = list(ds.bytes(T['lasthd'], 7))
hpcount = list(ds.bytes(T['hpcount'], 7))
hpsides = list(ds.bytes(T['hpsides'], 7))
conhp = ds.sbytes(T['conhp'], 26)
FRP = (2, 3, 4)                                     # fighter, paladin, ranger


def E(cls, L):
    """The experience level L starts at (ovr 0x39349)"""
    if L >= 41:
        return 0x7FFFFFFF
    r = T['records'] + cls * 0x12D
    if L <= 11:
        return ds.i32(r + 4 * L)
    return ds.i32(r + 44) + (L - 11) * ds.i32(T['xpstep'] + 4 * cls)


def fig(con, k):
    """43.1's Constitution figure (ovr 0x26ECC; Modify's least 0x1847F)"""
    v = conhp[con]
    if k in FRP:
        v += 1 if con == 17 else 2 if con == 18 else 3 if con in (19, 20) else 4 if 21 <= con <= 23 else 5 if con >= 24 else 0
    return v


def fig_most(con, k):
    """43.4's figure (EC:115 = ovr 0x37C27); 20+ left unset - not used"""
    assert 3 <= con <= 19
    if con == 3: return -2
    if con <= 6: return -1
    if con <= 14: return 0
    if con == 15: return 1
    if con == 16: return 2
    return con - 14 if k in FRP else 2


def least(lv, con):
    """43.3 Modify's least (ovr 0x184FE)"""
    base = figs = n = 0
    for k in range(7):
        L = lv[k]
        if L <= 0:
            continue
        n += 1
        f = fig(con, k)
        if L < lasthd[k]:
            base += L + (1 if k == 4 else 0)
            figs += f * L
        else:
            x = L - lasthd[k] + 1
            base += {0: 2 * x + 9, 2: 3 * x + 9, 3: 3 * x + 9, 4: 2 * x + 11, 5: x + 11, 6: 2 * x + 10}.get(k, 0)
            figs += f * (lasthd[k] - 1)
    v = base + figs if figs > 0 else base
    q = abs(v) // n * (1 if v >= 0 else -1)          # (idiv: towards 0)
    return q & 0xFF


def most(lv, con, former=None, r139=0, human=False):
    """43.4 Modify's most (ovr 0x27206)"""
    former = former or [0] * 7
    s = n = 0
    for k in range(7):
        if not (lv[k] > r139 or former[k] > 0):
            continue
        f = fig_most(con, k)
        if former[k] > 0 and lv[k] == 0:
            L = former[k]
        else:
            n += 1
            L = lv[k] - r139 if human and any(former) else lv[k]
        last, sides = lasthd[k], hpsides[k]
        rg = 1 if k == 4 else 0
        if L < last:
            v = (L + rg) * (sides + f)
        else:
            v = ((sides + f) * (last - 1 + rg)) & 0xFF
            v += {0: 2, 4: 2, 6: 2, 2: 3, 3: 3, 5: 1}.get(k, 0) * (L - last + 1)
        s += v
    return s // n if n else 0


def con_recalc(lv, old, r139, human, con, rolled):
    """43.2 the Constitution recalculation (ovr 0x3307E, helper 0x32F8C)"""
    bonus = held = 0

    def part(L, k):
        if lasthd[k] <= L:
            L = lasthd[k] - 1
        if k == 4:
            L += 1
        if k in FRP:
            if 15 <= con <= 19: return L * (con - 14)
            if con == 20: return L * 5
            if 21 <= con <= 23: return L * 6
            if con in (24, 25): return L * 7
            return 0
        return 2 * L if con > 15 else L if con == 15 else 0
    for k in range(7):
        if old[k] > 0:
            bonus = (bonus + part(old[k], k)) & 0xFF
        if lv[k] > 0:
            held += 1
            L = min(lv[k], lasthd[k])
            if human:
                if L > r139:
                    bonus = (bonus + part(L - r139, k)) & 0xFF
            else:
                bonus = (bonus + part(L, k)) & 0xFF
    return (rolled + bonus // held) & 0xFF


def hp_roll(lv, mask, con, counted, high):
    """43.1 / L2: the hit point routine's roll -> (rolled, figure)"""
    rolled = cs = 0
    for k in range(7):
        if lv[k] <= 0 or not (masks[k] & mask):
            continue
        if lv[k] < lasthd[k]:
            n = hpcount[k] if lv[k] == 1 else 1
            rolled += n * hpsides[k] if high else n
            cs += fig(con, k) * n
        else:
            rolled = 3 if k in (2, 3) else 2 if k in (4, 0, 6) else 1
    return max(1, (rolled & 0xFF) // counted), (cs & 0xFF) // counted


def drain(n, high, con, hp, hpmax, rolled, exp, bexp, bhp, lv, bl):
    """L1 / L2 the level drain (ovr 0x1CF3B), record 0x139 = 0"""
    lv, bl = list(lv), list(bl)
    for _ in range(n):
        counted = sum(1 for k in range(7) if lv[k] > 0 or bl[k] > 0)
        c = 0
        for k in range(1, 7):
            if lv[k] > lv[c] or (lv[k] == lv[c] and E(c, lv[c]) > E(k, lv[k])):
                c = k
        L = lv[c]
        bl[c] = max(bl[c], L)
        bexp = max(bexp, exp)
        bhp = max(bhp, hpmax)
        lo, hi = (0, 0) if L <= 1 else (0, E(c, 2)) if L == 2 else (E(c, L - 1), E(c, L))
        exp = lo + (hi - lo) // 2
        if L > 0:
            lv[c] -= 1
        if lv[c] >= 0:
            if counted == 0:
                hp = rolled = hpmax = 0
                continue
            rv, fg = hp_roll(lv, masks[c], con, counted, high)
            lost = (hpmax - hp) & 0xFF
            rolled = rolled - rv if rolled > rv else 0
            take = (rv + fg) & 0xFF               # (a byte: ovr 0x27149)
            mo = most(lv, con)
            if hpmax - take > mo:
                take = (hpmax - mo) & 0xFF
            hpmax = hpmax - take if take <= hpmax else 0
            hp = hpmax - lost if lost <= hpmax else 0
    res = 2 if not any(lv) or hpmax == 0 else 1 if hp == 0 else 0
    return lv, bl, exp, bexp, bhp, hpmax, hp, rolled, res


def restore(con, hp, hpmax, bhp, rolled, exp, bexp, lv, bl):
    """L6 / R11 Restoration (ovr 0x2EBEC)"""
    lv = list(lv)
    held = sum(1 for k in range(7) if bl[k] > 0)
    below = [k for k in range(7) if lv[k] < bl[k]]
    lost = sum(bl[k] - lv[k] for k in below)
    if not below:
        return lv, hp, hpmax, rolled, exp
    c = below[0]
    for k in below[1:]:
        if E(k, lv[k] + 1) <= E(c, lv[c] + 1):
            c = k
    if hpmax < bhp:
        per = (bhp - hpmax) // lost
        f = fig_most(con, c)
        cp = int(f / held) if lv[c] + 1 < lasthd[c] else 0
        hp = (hp + per) & 0xFF
        rolled = (rolled + per - cp) & 0xFF
        hpmax = (hpmax + per) & 0xFF
    lv[c] += 1
    if lost == 1:
        exp = max(exp, bexp)
    else:
        exp = E(c, lv[c])
    return lv, hp, hpmax, rolled, exp


MULTI = {0: 0x01, 2: 0x04, 3: 0x08, 4: 0x10, 5: 0x20, 6: 0x40, 8: 0x05, 9: 0x25, 10: 0x11, 11: 0x21, 12: 0x41,
         13: 0x24, 14: 0x44, 15: 0x64, 16: 0x60}

# ------------------------------------------------------------ oracle_<game>
w = gb.Writer(GAME)
w.c('behave %s oracle (creation, kit, Modify, conhp, Human Change) - tools/devkit/oracles/darkness.py' % GAME)
for k in range(7):
    w.row('xp', k, [E(k, L + 1) for L in range(1, 41)])
w.row('conhp', conhp)
w.row('classmask', masks)
w.row('lasthd', lasthd)
w.row('hpcount', hpcount)
w.row('hpsides', hpsides)
for k, key in ((5, 'muslots'), (4, 'rgslots')):
    r = T['records'] + k * 0x12D + 0x26
    for L in range(0, 41):
        w.row(key, L, list(ds.bytes(r + min(L, 29) * 9 + 1, 9)))
for n in range(1, 128):
    w.row('spell', n, list(ds.bytes(T['spells'] + 1 + 16 * n, 15)))
# 41. the magic-user's book: every class-2 spell of levels 1-3; the lists for
# 4th-7th level slots (the book bytes GAME.OVR 0x162CE sets - facts 41,
# Treasures' 0x1BA8C the same); a 9th level slot: every class-2 spell
w.row('mubook_all', 3)
w.row('mubook', 4, 81, 82, 83, 84, 85, 87, 88)
w.row('mubook', 5, 91, 92, 118, 119)
w.row('mubook', 6, 111, 112, 113, 114)
w.row('mubook', 7, 115, 117)
w.row('mubook_every', 9)
for dr in range(6):
    cnt = ds.u8(T['raceclasses'] + 14 * dr)
    w.row('raceclasses', dr, list(ds.bytes(T['raceclasses'] + 14 * dr + 1, cnt)))
    w.row('racelim', dr, list(ds.bytes(T['limits'] + 16 * dr, 16)))
    w.row('agebr', dr, [ds.u16(T['brackets'] + 10 * dr + 2 * b) for b in range(5)])
    for e in range(7):
        a = T['ages'] + 28 * dr + 4 * e
        w.row('age', dr, e, ds.u16(a), ds.u8(a + 2), ds.u8(a + 3))
for cls in range(17):
    cnt = ds.u8(T['align'] + 10 * cls)
    w.row('classalign', cls, list(ds.bytes(T['align'] + 10 * cls + 1, cnt)))
    w.row('classmin', cls, list(ds.bytes(T['classmin'] + 6 * cls, 6)))
w.row('age_plus', 10)
# 40.6 AD&D's race adjustments (Game Fixes): elf Dex +1 Con -1, dwarf Con +1
# Cha -1, halfling Str -1 Dex +1 (Darkness race numbers 0 elf .. 5 human)
ADJ = {0: (0, 0, 0, 1, -1, 0), 2: (0, 0, 0, 0, 1, -1), 4: (-1, 0, 0, 1, 0, 0)}
for dr in range(6):
    w.row('raceadj', dr, ADJ.get(dr, (0,) * 6))
# 40.3 the races' effects in the original's order
for dr, fx in ((0, (0x5F,)), (1, (0x12,)), (2, (0x2F, 0x1A, 0x61)), (3, (0x30, 0x07, 0x61)), (4, (0x61,)), (5, ())):
    w.row('racefx', dr, fx)
w.row('start_xp', 1500001, 750000, 500000)          # 40.4
w.row('money_pp', 200)                              # 40.7
w.row('icon_head', 5, 9, 0, 7, 3)                   # 40.11: men / women size 2, men / women small, halfling
w.row('icon_body', 0x17, 1, 0x18, 0x1D, 5)          # cleric, ranger, paladin / fighter, magic-user, thief
w.row('racesize', 2, 2, 1, 1, 1, 2)
items = gb.dax_block(gb.find(GAME, 'ITEM0.DAX'), 1)
for kind in range(1, 5):                            # 42. the kit
    ids = list(ds.bytes(T['kit'] + 6 * kind + 1, 6))
    w.row('kit', kind, ids)
    for j, b in enumerate(ids, 1):
        if b and (b - 1) * 17 + 17 <= len(items):
            w.row('kititem', kind, j, list(items[(b - 1) * 17:(b - 1) * 17 + 17]))
w.row('mu_change', 11, 18, 12, 21)                  # 45.
# 43.3 / 43.4 Modify's bounds: levels 7, Con, least, most
LV = (1, 2, 4, 7, 9, 10, 11, 12, 13, 15, 20, 30, 40)
for cls, bits in sorted(MULTI.items()):
    held = [k for k in range(7) if bits >> k & 1]
    for i, L in enumerate(LV):
        for con in (3, 5, 9, 15, 16, 17, 18, 19):
            for shift in range(len(held)):
                lv = [0] * 7
                for j, k in enumerate(held):
                    lv[k] = LV[(i + j * shift) % len(LV)]
                w.row('bounds', lv, con, least(lv, con), most(lv, con))
                if len(held) == 1:
                    break
# 43.2 the Constitution recalculation cases: levels, former levels, 0x139, human, Con, rolled -> HP max
import random
rnd = random.Random(43)
for i in range(600):
    cls, bits = rnd.choice(sorted(MULTI.items()))
    lv, old = [0] * 7, [0] * 7
    human = rnd.random() < 0.5
    for k in range(7):
        if bits >> k & 1:
            lv[k] = rnd.choice(LV)
    r139 = 0
    if human and len([k for k in range(7) if bits >> k & 1]) == 1 and rnd.random() < 0.6:
        o = rnd.choice([k for k in (0, 2, 5, 6) if not bits >> k & 1])
        old[o] = rnd.choice((2, 5, 9, 12, 15))
        r139 = old[o]
        lv[[k for k in range(7) if bits >> k & 1][0]] = rnd.choice((1, 3, r139, r139 + 1, r139 + 4, 30))
    con = rnd.choice((3, 9, 14, 15, 16, 17, 18, 19, 20, 22, 24, 25))
    rolled = rnd.randrange(0, 200)
    w.row('conhpcase', lv, old, r139, int(human), con, rolled, con_recalc(lv, old, r139, human, con, rolled))
w.done()

# ------------------------------------------------------------ _fights
f = gb.Writer(GAME + '_fights')
f.c('behave %s fights oracle - tools/devkit/oracles/darkness.py' % GAME)
for t in range(1, 14):                              # R13 the turn table
    f.row('turn', t, ds.sbytes(T['turn'] + 11 * t, 11))
for n in range(1, 128):                             # R1 the spell rows
    f.row('sprow', n, list(ds.bytes(T['spells'] + 1 + 16 * n, 15)))
f.row('bb', 8, 8, 3)                                # R2: 8d8, 3 rounds a level
f.row('pwk_fix', 10, 60, 10, 120)                   # L3 AD&D's
f.row('meteor', 4, 10, 4)                           # R8: four meteors of 10d4
f.row('meteor_dx', ds.sbytes(T['meteor_x'], 4))
f.row('meteor_dy', ds.sbytes(T['meteor_y'], 4))
if T['reserve'] is not None:                        # R10 the reserve's MON blocks by frame style
    f.row('reserve', list(ds.bytes(T['reserve'], 5)))
f.row('dance', 4)                                   # R6: AC 4 worse
f.row('feared', -2, -2)                             # R14 0x78
f.row('kalistes', 1, 10, 2, 4, 1, 8, 1, 8, 1, 4, 1, 6, 4)   # R14 0x74
f.row('vorpal', 17)
f.row('dfear', 1, -4)                               # (save kind, bonus)
f.row('pgaze', 1, 3, 10)
f.row('gaze_fly', 18, 17, 45, 50)                   # L7
f.row('beholder', 2, 3)
f.done()

# ------------------------------------------------------------ _left
lf = gb.Writer(GAME + '_left')
lf.c('behave %s drain / camp oracle - tools/devkit/oracles/darkness.py' % GAME)
rnd = random.Random(1)
case = 0
for i in range(500):
    cls, bits = rnd.choice(sorted(MULTI.items()))
    lv = [0] * 7
    for k in range(7):
        if bits >> k & 1:
            lv[k] = rnd.choice((1, 2, 3, 5, 8, 10, 11, 12, 13, 18, 25))
    n = rnd.choice((1, 2))
    if sum(lv) < n + (0 if rnd.random() < 0.1 else 1):
        continue
    con = rnd.choice((3, 5, 10, 15, 16, 17, 18, 19))
    hpmax = rnd.randrange(1, 255)
    hp = rnd.randrange(0, hpmax + 1)
    rolled = rnd.randrange(0, 255)
    exp = rnd.randrange(0, 4000000)
    bexp = rnd.choice((0, exp + 1000, exp // 2))
    bhp = rnd.choice((0, min(255, hpmax + 10), hpmax // 2))
    bl = [max(0, lv[k] + rnd.choice((0, 0, 1, -1))) if lv[k] else 0 for k in range(7)]
    high = rnd.randrange(2)
    out = drain(n, high, con, hp, hpmax, rolled, exp, bexp, bhp, lv, bl)
    case += 1
    lf.row('drain', case, n, high, con, hp, hpmax, rolled, exp, bexp, bhp, lv, bl, out[0], out[1], out[2:])
for i in range(300):
    cls, bits = rnd.choice(sorted(MULTI.items()))
    bl, lv = [0] * 7, [0] * 7
    for k in range(7):
        if bits >> k & 1:
            bl[k] = rnd.choice((2, 3, 5, 9, 11, 12, 14, 20))
            lv[k] = max(1, bl[k] - rnd.choice((0, 1, 1, 2, 3)))
    con = rnd.choice((3, 5, 10, 15, 16, 17, 18, 19))
    hpmax = rnd.randrange(1, 200)
    hp = rnd.randrange(0, hpmax + 1)
    bhp = rnd.choice((hpmax, min(255, hpmax + rnd.randrange(1, 50)), max(0, hpmax - 5)))
    rolled = rnd.randrange(0, 200)
    exp = rnd.randrange(0, 3000000)
    bexp = rnd.choice((exp, exp + 5000, exp // 2))
    out = restore(con, hp, hpmax, bhp, rolled, exp, bexp, lv, bl)
    case += 1
    lf.row('restore', case, con, hp, hpmax, bhp, rolled, exp, bexp, lv, bl, out[0], out[1:])
# L6 the camp: every spell's row (spell, class, level, targets, effect, when, time, time a level)
for n in range(1, 128):
    r = ds.bytes(T['spells'] + 1 + 16 * n, 15)
    lf.row('camprow', n, r[0], r[1], r[7], r[10], r[11], r[4], r[5])
# L6 the handlers' words: the spell handler table's far pointers (written by a
# run of "mov ax, off; mov dx, seg; mov [table + 4 n], ax; mov [.. + 2], dx"
# in GAME.OVR - Darkness's at 0x30B4E), each resolved through its overlay
# stub; the handler and the routines it calls in its own overlay read for the
# Pascal strings they load ("mov di, s; push cs / ds; push di") and join
# (the runtime's copy 0x825:0xA6F then add 0x825:0xAFC: "is " + a name)
IMG = gb.image(GAME, 'GAME.EXE')


def stub(seg, off):
    s = seg * 16
    if IMG[s:s + 2] != b'\xcd\x3f' or IMG[s + off:s + off + 2] != b'\xcd\x3f':
        return None, None
    base = struct.unpack_from('<I', IMG, s + 4)[0]
    return base + struct.unpack_from('<H', IMG, s + off + 2)[0], base


def pstr(buf, at):
    if at + 1 >= len(buf):
        return None
    n = buf[at]
    t = buf[at + 1:at + 1 + n]
    return t.decode('latin-1') if n and all(32 <= c < 127 for c in t) else None


# the runtime's string copy / add (Darkness 0x825:0xA6F / 0xAFC, Treasures
# 0xA77:0xAC2 / 0xB4F); Treasures' effect names: BORD.DAX block 9 through the
# pointer [DS 0x9E14] (effect n at 38 x (n - 1))
RT = ('0x825:0xa6f', '0x825:0xafc') if GAME == 'darkness' else ('0xa77:0xac2', '0xa77:0xb4f')
NAMES = None if GAME == 'darkness' else gb.dax_block(gb.find(GAME, 'BORD.DAX'), 9)


def routine_words(at, base, out):
    """The words a handler loads and joins, up to its first ret"""
    lines = [ln[28:].strip() for ln in subprocess.run(['ndisasm', '-b16', '-o', str(at - base), '-e', str(at), OVR],
                                                      capture_output=True, text=True).stdout.splitlines()[:700]]
    last = cur = None
    for i, t in enumerate(lines):
        nx = lines[i + 1] if i + 1 < len(lines) else ''
        m = re.match(r'mov di,(0x[0-9a-f]+)$', t)
        if m:
            a = int(m.group(1), 16)
            s = pstr(ovr, base + a) if nx == 'push cs' else pstr(IMG, BASE + a) if nx == 'push ds' else None
            if s is not None:
                last = s
                out.add(s)
        m = re.match(r'les di,\[0x9e14\]$', t)
        if m and NAMES is not None:
            m2 = re.match(r'add di,(0x[0-9a-f]+)$', nx)
            off = int(m2.group(1), 16) if m2 else 0
            if off % 38 == 0:
                last = pstr(NAMES, off)
                if last:
                    out.add(last)
        if t == 'call ' + RT[0]:
            cur = last
        elif t == 'call ' + RT[1] and cur is not None and last is not None:
            cur = cur + last
            out.add(cur)
        if t.startswith('ret'):
            break


fills = [(m.start(), struct.unpack('<HHH', m.group(1) + m.group(2) + m.group(3)))
         for m in re.finditer(rb'\xb8(..)\xba(..)\xa3(..)\x89\x16..', ovr, re.S)]
best = []
for i in range(len(fills)):
    j = i
    while j + 1 < len(fills) and fills[j + 1][0] == fills[j][0] + 13 and fills[j + 1][1][2] == fills[j][1][2] + 4:
        j += 1
    if j - i + 1 > 100 and len(fills[i:j + 1]) > len(best):
        best = fills[i:j + 1]
table = best[0][1][2] - 4                           # (its first entry: spell 1)
lf.c('the spell handler table at DS 0x%X (filled at GAME.OVR 0x%X)' % (table, best[0][0]))
for _, (off, seg, addr) in best:
    s = (addr - table) // 4
    a, b = stub(seg, off)
    if not a:
        continue
    ws = set()
    routine_words(a, b, ws)
    for t in sorted(ws):
        lf.row('campword', s, len(t), [ord(c) for c in t])
lf.row('campfx', 0x2B, 0x24, 0x14, 0x37)           # Remove Curse's 0x2B, the curse, Resist Fire's, poison
for s, nd, sd, plus, mult in ((0x39, 5, 4, 0, 1), (0x3D, 5, 4, 0, 1), (0x3B, 1, 4, 4, 10), (0x1A, 0, 0, 3780, 1),
                              (0x7A, 0, 0, 1440, 1), (0x79, 1, 4, 1, 1)):
    lf.row('camptimed', s, nd, sd, plus, mult)      # the time routine (ovr 0x2AB51)
lf.done()
