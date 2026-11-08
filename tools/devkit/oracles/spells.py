#!/usr/bin/env python3
# oracle_spells.txt for behave spells (behave_spells.inc, Curse): the
# program's spell table (START.EXE DS 0x37DC, 16 bytes a spell 1-0x64 -
# classes::Layout spells), the spell slots by level in all (the class
# records DS 0x429B, 99 bytes a class: after 11 x i32 experience, 11 x 5
# slots gained at levels 2-12; level 1 has one 1st-level slot - the PHB's
# tables, which these sum to), the PHB's Wisdom bonus spells, Sleep's Hit
# Dice costs and Enlarge's Strengths by caster level (the original's code,
# coab's listing - spell_facts.md).
import gb

BASE, SPELLS, RECORDS = 0xABE0, 0x37DC, 0x429B
ds = gb.DS(gb.image('curse'), BASE)
w = gb.Writer('spells')
w.c('behave spells oracle (Curse) - tools/devkit/oracles/spells.py')
for s in range(1, 0x65):
    r = list(ds.bytes(SPELLS + s * 16, 16))
    r[2] = ds.s8(SPELLS + s * 16 + 2)      # (the range: signed, -1 = touch)
    w.row('row', s, r)
for tag, cls in (('clslots', 0), ('muslots', 5)):
    tot = [1, 0, 0, 0, 0]
    r = RECORDS + cls * 99
    for lv in range(1, 13):
        if lv >= 2:
            g = ds.bytes(r + 44 + (lv - 2) * 5, 5)
            tot = [a + b for a, b in zip(tot, g)]
        w.row(tag, lv, tot)
# PHB: Wisdom 13 / 14 +1 1st, 15 / 16 +1 2nd, 17 +1 3rd, 18 +1 4th (each counted
# when the cleric has that spell level)
for wis, idx in ((13, 0), (14, 0), (15, 1), (16, 1), (17, 2), (18, 3)):
    w.row('wisbonus', wis, idx, 1)
# Sleep: a creature's cost by Hit Dice 0-7 (4d4 to spend)
for hd, cost in enumerate([1, 1, 2, 4, 6, 10, 20, 20]):
    w.row('sleepcost', hd, cost)
# Enlarge: the Strength by caster level 1-12 (Str, 18/xx); 12 falls to the
# code's default (18)
ENL = {1: (18, 0), 2: (18, 1), 3: (18, 51), 4: (18, 76), 5: (18, 91), 6: (18, 100), 7: (19, 0), 8: (20, 0),
       9: (21, 0), 10: (22, 0), 11: (22, 0), 12: (18, 0)}
for lv in range(1, 13):
    w.row('enlarge', lv, *ENL[lv])
w.done()
