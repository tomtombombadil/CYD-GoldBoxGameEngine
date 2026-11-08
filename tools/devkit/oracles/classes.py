#!/usr/bin/env python3
# oracle_classes.txt for behave classes and behave saves (behave_classes.inc,
# behave_saves.inc through behave_oracle.inc; Curse). From the player's
# START.EXE data segment (image 0xABE0) at classes::Layout's addresses
# (profile.cpp): THAC0 0x3E3A [class][0-12], max hit dice 0x3EBB, thief
# skills 0x3EC0 / 0x3F20 / 0x3F33, stat limits 0x3F88, race classes 0x3FFA,
# ages 0x404E, age brackets 0x4124, class minimums 0x4174, alignments
# 0x41DA, class records 0x429B (experience, slots gained), saves 0x45BE,
# hit dice 0x081A (count) / 0x0822 (sides).
# From the rules (PHB / DMG): Constitution's hit points, the hit points a
# level past the hit dice, Wisdom's bonus spells, Strength / Dexterity,
# carrying. From the facts (coab's listing of the original's code, as
# recorded in the Project's facts docs): the age effects by bracket, the
# race / class effects given at creation, starting experience, a new
# magic-user's spells and silent training's, Human Change's.
import gb

DSB = 0xABE0
THAC0, MAXHD, TBASE, TRACE, TDEX = 0x3E3A, 0x3EBB, 0x3EC0, 0x3F20, 0x3F33
LIMITS, RCLASSES, AGES, BRACKETS, CMIN, CALIGN, RECORDS, SAVES = 0x3F88, 0x3FFA, 0x404E, 0x4124, 0x4174, 0x41DA, 0x429B, 0x45BE
HPCOUNT, HPDICE = 0x081A, 0x0822

ds = gb.DS(gb.image('curse'), DSB)
w = gb.Writer('classes')
w.c('behave classes / saves oracle (Curse) - tools/devkit/oracles/classes.py')

for cls in range(8):
    w.row('xp', cls, [ds.i32(RECORDS + cls * 99 + i * 4) for i in range(11)])
    w.row('thac0', cls, list(ds.bytes(THAC0 + cls * 13, 13)))
    for lv in range(1, 13):
        w.row('save', cls, lv, list(ds.bytes(SAVES + cls * 60 + lv * 5, 5)))

# Constitution's hit points a die (PHB): "con hp fighters' extra" from that Con up
for con, hp, extra in ((3, -2, 0), (4, -1, 0), (7, 0, 0), (15, 1, 0), (16, 2, 0), (17, 2, 1), (18, 2, 2), (19, 2, 3)):
    w.row('con_hp', con, hp, extra)
# Hit dice: sides and dice at level 1 (the program's), the level they stop at
# (the program's), the fixed gain a level past them (PHB)
FIXED = [2, 2, 3, 3, 2, 1, 2, 0]
for k in range(8):
    w.row('hp', k, ds.u8(HPDICE + k), ds.u8(HPCOUNT + k), ds.u8(MAXHD + k), FIXED[k])

for lv in range(1, 13):
    w.row('thief_base', lv, list(ds.bytes(TBASE + lv * 8 + 1, 8)))
for race in range(1, 8):
    w.row('thief_race', race, ds.sbytes(TRACE + race * 8 + 1, 8))
for dex in range(9, 19):
    w.row('thief_dex', dex, ds.sbytes(TDEX + dex * 5 + 1, 5))

# Spell slots in all by level: the class records' gains summed (a cleric /
# magic-user starts with one 1st-level slot - the PHB's)
for key, cls, first in (('cleric_slots', 0, 1), ('paladin_slots', 3, 0), ('ranger_slots', 4, 0), ('mu_slots', 5, 1)):
    tot = [first, 0, 0, 0, 0]
    for lv in range(1, 13):
        if lv >= 2:
            tot = [a + b for a, b in zip(tot, ds.bytes(RECORDS + cls * 99 + 44 + (lv - 2) * 5, 5))]
        w.row(key, lv, tot)
# Wisdom's bonus cleric spells (PHB), cumulative, by spell level 1-5
WIS = {12: [0, 0, 0, 0, 0], 13: [1, 0, 0, 0, 0], 14: [2, 0, 0, 0, 0], 15: [2, 1, 0, 0, 0], 16: [2, 2, 0, 0, 0],
       17: [2, 2, 1, 0, 0], 18: [2, 2, 1, 1, 0]}
for wis, b in WIS.items():
    w.row('wis_bonus', wis, b)

for race in range(1, 8):
    at = RCLASSES + race * 14
    w.row('race_classes', race, list(ds.bytes(at + 1, ds.u8(at))))
    w.row('race_lim', race, list(ds.bytes(LIMITS + race * 16, 16)))
    for e in range(7):
        a = AGES + race * 28 + e * 4
        w.row('age', race, e, ds.u16(a), ds.u8(a + 2), ds.u8(a + 3))
    w.row('age_bracket', race, [ds.u16(BRACKETS + race * 10 + b * 2) for b in range(5)])
for cls in range(17):
    w.row('class_min', cls, list(ds.bytes(CMIN + cls * 6, 6)))
    at = CALIGN + cls * 10
    w.row('class_align', cls, list(ds.bytes(at + 1, ds.u8(at))))
    w.row('start_xp', cls, 25000 if cls < 8 else 8333 if cls in (9, 15, 16) else 12500)

# The age effects past each bracket (Str Int Wis Dex Con Cha)
AGE_FX = [[1, -1, -2, -1], [0, 1, 0, 1], [-1, 1, 1, 1], [0, 0, -2, -1], [0, -1, -1, -1], [0, 0, 0, 0]]
for i, r in enumerate(AGE_FX):
    w.row('age_effect', i, r)
# Effects at creation: dwarves Constitution saves 0x61, vs orcs 0x1A, vs giants
# 0x2F; elves 0x6B; gnomes 0x61, 0x12, 0x2F, 0x30; half-elves 0x7C; halflings
# 0x61; paladins 0x08; rangers 0x86 (the cleric / ranger too)
for race, fx in ((1, [0x61, 0x1A, 0x2F]), (2, [0x6B]), (3, [0x61, 0x12, 0x2F, 0x30]), (4, [0x7C]), (5, [0x61])):
    w.row('race_fx', race, fx)
for cls, fx in ((3, [0x08]), (4, [0x86]), (10, [0x86])):
    w.row('class_fx', cls, fx)
# A new magic-user's book; silent training's by level; Human Change's
w.row('mu_first', 0x0B, 0x12, 0x0C, 0x15)
for lv, sp in ((2, [0x0F]), (3, [0x22, 0x0A]), (4, [0x1F]), (5, [0x2F])):
    w.row('mu_silent', lv, sp)
w.row('mu_change', 0x0B, 0x12, 0x0C, 0x15)

# Strength / Dexterity in play (DMG) and carrying
def grp(s, e):
    if s <= 17: return s
    if s == 18: return 18 if e == 0 else 19 if e <= 50 else 20 if e <= 75 else 21 if e <= 90 else 22 if e <= 99 else 23
    return s + 5
for s, e in ((3, 0), (5, 0), (7, 0), (9, 0), (12, 0), (14, 0), (16, 0), (17, 0), (18, 0), (18, 1), (18, 50), (18, 51),
             (18, 75), (18, 76), (18, 90), (18, 91), (18, 99), (18, 100)):
    g = grp(s, e)
    w.row('str', s, e, gb.STR_HIT[g], gb.STR_DMG[g], gb.STR_ENC[g])
for d in range(3, 19):
    w.row('dex', d, gb.DEX_AC[d], gb.DEX_MISSILE[d])
for over, move in ((0x200, 12), (0x201, 9), (0x300, 9), (0x301, 6), (0x400, 6), (0x401, 3)):
    w.row('move', over, move)
# The original's saved game: SAVGAMA.DAT is 13,149 bytes (coab's loadSaveGame layout)
w.row('save_sizes', 13149)
w.done()
