#!/usr/bin/env python3
# oracle_lv_<game>.txt for behave levels' parts "values" and "multi"
# (behave_levels.inc): every single class trained level by level - the
# experience each level wants, THAC0, saves, half attacks, hit points gained,
# spell slots, thief skills - from the original program's own tables read
# from the player's files, composed by the rules of the Project's
# behaviour_facts.md (classes) - never the engine's.
# Usage: lv_oracle.py [curse|darkness|treasures]
#   curse: every value (behaviour_facts.md's class rules);
#   darkness / treasures: the experience, the last level and the hit points
#   only (darkness_create_facts.md 43.1 and the class records - its THAC0,
#   saves, attacks, slots and thief skills' compositions are not in the facts
#   this kit has: no rows, not checked)
import sys
import gb

game = sys.argv[1] if len(sys.argv) > 1 else 'curse'
if game in ('darkness', 'treasures'):
    sys.argv[1:] = [game]
    import darkness as dk                           # (its oracles are written too: the same tables)
    w = gb.Writer('lv_' + game)
    w.c('behave levels oracle (%s: experience, last level, hit points) - tools/devkit/oracles/lv_oracle.py' % game)
    for k in (0, 2, 3, 4, 5, 6):
        w.row('class', k, 0, 40)                    # (training's limit: level 40 - 40.8)
        for L in range(1, 41):
            w.row('xp', k, 0, L, dk.E(k, L + 1) if L < 40 else -1)
            if L >= 2:
                for con in (18, 16, 12, 5):
                    lv = [0] * 7
                    lv[k] = L
                    g = []
                    for high in (True, False):
                        inc, cp = dk.hp_roll(lv, dk.masks[k], con, 1, high)
                        g.append((inc + cp) & 0xFF if (inc + cp) & 0xFF < 128 else ((inc + cp) & 0xFF) - 256)
                    w.row('hp', k, L, con, g)
        w.row('hd', k, dk.hpcount[k], dk.hpsides[k], dk.lasthd[k])
        w.row('past', k, 3 if k in (2, 3) else 2 if k in (0, 4, 6) else 1)
        for con in (18, 16, 12, 5):
            w.row('mfig', k, con, dk.fig(con, k))
    w.row('hpfam', 1)
    w.done()
    sys.exit(0)
if game != 'curse':
    sys.exit('lv_oracle.py: no facts for %s here' % game)

DSB = 0xABE0
THAC0, MAXHD, TBASE, TRACE, TDEX = 0x3E3A, 0x3EBB, 0x3EC0, 0x3F20, 0x3F33
RECORDS, SAVES, HPCOUNT, HPDICE = 0x429B, 0x45BE, 0x081A, 0x0822
ds = gb.DS(gb.image('curse'), DSB)
w = gb.Writer('lv_curse')
w.c('behave levels oracle (Curse) - tools/devkit/oracles/lv_oracle.py')
CLASSES = (0, 2, 3, 4, 5, 6)
FRP = (2, 3, 4)
FIXED = {0: 2, 2: 3, 3: 3, 4: 2, 5: 1, 6: 2}        # past the hit dice (facts: assigned, not added)


def xp(k, L):
    """the experience level L wants (L 2-12), -1: none"""
    return ds.i32(RECORDS + k * 99 + (L - 2) * 4) if 2 <= L <= 12 else -1


def con_fig(con, k):
    v = -2 if con == 3 else -1 if con <= 6 else 0 if con <= 14 else 1 if con == 15 else 2
    if k in FRP:
        v += 1 if con == 17 else 2 if con == 18 else 3 if con in (19, 20) else 0
    return v


def gains(k, L, con):
    """hit points a training to level L adds (dice highest, lowest) - one class"""
    stop = ds.u8(MAXHD + k)
    if L < stop:
        f = con_fig(con, k)
        return max(1, ds.u8(HPDICE + k) + f), max(1, 1 + f)
    return FIXED[k], FIXED[k]


def slots(k, L, stat):
    """27 slots: cleric 1-9, druid 1-9, magic-user 1-9 - the class records'
    gains from level 2 summed (a cleric / magic-user starts with a 1st), the
    cleric's Wisdom bonus where the level has a slot"""
    tot = [0] * 5
    if k in (0, 5):
        tot[0] = 1
    for lv in range(2, L + 1):
        tot = [a + b for a, b in zip(tot, ds.bytes(RECORDS + k * 99 + 44 + (lv - 2) * 5, 5))]
    out = [0] * 27
    if k == 0:
        bonus = {13: (1,), 14: (1, 1), 15: (1, 1, 2), 16: (1, 1, 2, 2), 17: (1, 1, 2, 2, 3), 18: (1, 1, 2, 2, 3, 4)}
        for lvl in bonus.get(stat, ()):
            if tot[lvl - 1] > 0:
                out[lvl - 1] += 1
        for i in range(5):
            out[i] += tot[i]
    elif k == 3:                                    # paladin: cleric spells
        for i in range(5):
            out[i] = tot[i]
    elif k == 4:                                    # ranger: druid 1-3, magic-user 1-2
        out[9:12] = tot[0:3]
        out[18:20] = tot[3:5]
    elif k == 5:
        out[18:23] = tot
    return out


for k in CLASSES:
    last = 1
    while xp(k, last + 1) > 0:
        last += 1
    w.row('class', k, 0, last)
    for L in range(1, last + 1):
        w.row('xp', k, 0, L, xp(k, L + 1) if L < last else -1)
        # (the best of every class's row at its level - the others' level 0 rows too)
        w.row('thac0', k, L, max(ds.u8(THAC0 + c * 13 + (L if c == k else 0)) for c in range(8)))
        w.row('save', k, L, list(ds.bytes(SAVES + k * 60 + L * 5, 5)))
        w.row('att', k, L, 3 if (k in (2, 3) and L >= 7) or (k == 4 and L >= 8) else 2)
        for stat in (18, 16, 13, 9):
            w.row('slots', k, 0, L, stat, slots(k, L, stat))
        if L >= 2:
            for con in (18, 16, 12, 5):
                w.row('hp', k, L, con, *gains(k, L, con))
    w.row('hd', k, ds.u8(HPCOUNT + k), ds.u8(HPDICE + k), ds.u8(MAXHD + k))
    w.row('past', k, FIXED[k])
    # (a multi-class's figure per class held: the fighters' extra goes by the
    # character's class id - never a fighter's in a multi-class id)
    for con in (18, 16, 12, 5):
        w.row('mfig', k, con, con_fig(con, -1))
# thief skills: the level's + the race's (a human's; 0 when its minus is more
# than the base) + Dexterity's for skills 1-5
for L in range(1, 13):
    for dex in (18, 14, 9):
        v = []
        for s in range(1, 9):
            b = ds.u8(TBASE + L * 8 + s)
            r = ds.s8(TRACE + 7 * 8 + s)
            x = 0 if -r > b else b + r
            if s <= 5:
                x += ds.s8(TDEX + dex * 5 + s)
            v.append(x)
        w.row('thief', L, dex, v)
w.row('hpfam', 0)
w.done()
