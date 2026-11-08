#!/usr/bin/env python3
# oracle_pool.txt for behave pool (behave_pool.inc; GAME=1). Pool of
# Radiance's own tables from the player's START.EXE (data segment at image
# 0xC7C0; classes::Layout pool in profile.cpp): spells 0x31FA (16 bytes a
# spell), THAC0 0x3C7C [class][11], thief skills 0x3CF3 / 0x3D3B / 0x3D4E,
# stat limits 0x3DA3, race classes 0x3E17 (12 bytes), ages 0x3E5B, age
# brackets 0x3F31, class minimums 0x3F81, alignments 0x3FE7, class records
# 0x40A3 (56 bytes: 8 x i32 experience for levels 2-9, 8 x 3 slots in all),
# money dice 0x4263, saves 0x426E [class * 45 + level * 5], class masks
# 0x5DE, hit dice 0x08B6 (count) / 0x08BE (sides), icon colours 0x3CF6, the
# turning table 0x0447 (type * 10 + column). Rules: the PHB's Constitution
# hit points and coin values; facts (poolrad_create_facts.md): the effects
# made at creation, a new magic-user's book.
import gb

B = 0xC7C0
SPELLS, THAC0, TBASE, TRACE, TDEX = 0x31FA, 0x3C7C, 0x3CF3, 0x3D3B, 0x3D4E
LIMITS, RCLASSES, AGES, BRACKETS, CMIN, CALIGN, RECORDS = 0x3DA3, 0x3E17, 0x3E5B, 0x3F31, 0x3F81, 0x3FE7, 0x40A3
MONEY, SAVES, MASKS, HPCOUNT, HPDICE, ICONS, TURN = 0x4263, 0x426E, 0x5DE, 0x08B6, 0x08BE, 0x3CF6, 0x0447

ds = gb.DS(gb.image('poolrad'), B)
w = gb.Writer('pool')
w.c('behave pool oracle (PoolRad) - tools/devkit/oracles/pool.py')
for k in range(8):
    w.row('xp', k, 0, [ds.i32(RECORDS + k * 56 + i * 4) for i in range(8)])
    w.row('thac0', k, list(ds.bytes(THAC0 + k * 11, 11)))
    for lv in range(1, 10):
        w.row('save', k, lv, list(ds.bytes(SAVES + k * 45 + lv * 5, 5)))
for key, k in (('cleric_slots', 0), ('mu_slots', 5)):
    for lv in range(2, 10):
        w.row(key, lv, list(ds.bytes(RECORDS + k * 56 + 32 + (lv - 2) * 3, 3)))
for s in range(1, 0x44):
    w.row('spell', s, list(ds.bytes(SPELLS + s * 16, 16)))
for lv in range(1, 10):
    w.row('thief_base', lv, list(ds.bytes(TBASE + lv * 8 + 1, 8)))
for race in range(1, 8):
    w.row('thief_race', race, ds.sbytes(TRACE + race * 8 + 1, 8))
for dex in range(3, 20):
    w.row('thief_dex', dex, ds.sbytes(TDEX + dex * 5 + 1, 5))
for race in range(1, 8):
    at = RCLASSES + race * 12
    w.row('race_classes', race, list(ds.bytes(at + 1, min(ds.u8(at), 11))))
    w.row('limits', race, list(ds.bytes(LIMITS + race * 16, 16)))
    ages = []
    for e in range(7):
        a = AGES + race * 28 + e * 4
        ages += [ds.u16(a), ds.u8(a + 2), ds.u8(a + 3)]
    w.row('ages', race, ages)
    w.row('age_bracket', race, [ds.u16(BRACKETS + race * 10 + b * 2) for b in range(5)])
for cls in range(17):
    w.row('class_min', cls, list(ds.bytes(CMIN + cls * 6, 6)))
    at = CALIGN + cls * 10
    w.row('class_align', cls, list(ds.bytes(at + 1, min(ds.u8(at), 9))))
# Effects at creation (the races' - dwarves and halflings also 0x5A, Pool's poison save)
FX = {1: [0x61, 0x1A, 0x2F, 0x5A], 2: [0x6B], 3: [0x61, 0x12, 0x2F, 0x30], 4: [0x7C], 5: [0x61, 0x5A], 6: [], 7: []}
for race, fx in FX.items():
    w.row('race_fx', race, fx)
w.row('mu_first', 0x0B, 0x12, 0x13, 0x15)
# The PHB: coins in copper (cp sp ep gp pp), Constitution's hit points a class (by Con 0-19)
w.row('coins', 1, 10, 100, 200, 1000)
w.row('con_hp', [0, 0, 0, -2, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2])
w.row('hp_count', list(ds.bytes(HPCOUNT, 8)))
w.row('hp_sides', list(ds.bytes(HPDICE, 8)))
w.row('money', list(ds.bytes(MONEY, 16)))
w.row('icon_colours', list(ds.bytes(ICONS, 6)))
w.row('class_masks', list(ds.bytes(MASKS, 8)))
# The turning table (type 1-10, columns 1-10: cleric level 1-8, 9-13, 14+) as
# the d20 needed - T (1) any roll, D (0) / D* (-1) destroyed (negative), 99 none
for t in range(1, 11):
    row = []
    for col in range(0, 11):
        v = ds.s8(TURN + t * 10 + col)
        row.append(v if v >= 2 else 1 if v == 1 else -1)
    w.row('turn', t, row)
w.done()
