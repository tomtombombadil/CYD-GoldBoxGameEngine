#!/usr/bin/env python3
# oracle_monsters.txt for behave monsters (behave_monsters.inc, Curse): the
# rule tables a fighter's recalculation uses - AD&D's Strength / Dexterity
# (gb.py), the arrow / quarrel item types, the elves' weapon types and the
# thieves' backstab weapons (Curse's item numbering, coab's lists: club 7,
# dagger 8, broad / long / short sword 35-37, drow long sword 97). The
# monsters themselves are read from the player's MON*CHA files at run time.
import gb

w = gb.Writer('monsters')
w.c('behave monsters oracle (Curse) - tools/devkit/oracles/monsters.py')
w.row('str_hit', [gb.STR_HIT[g] for g in range(31)])
w.row('str_dmg', [gb.STR_DMG[g] for g in range(31)])
w.row('max_enc', [gb.STR_ENC[g] for g in range(31)])
w.row('dex_ac', [gb.DEX_AC[d] for d in range(26)])
w.row('dex_react', [gb.DEX_MISSILE[d] for d in range(26)])
w.row('arrow', 73)
w.row('quarrel', 28)
w.row('elf_types', 41, 42, 43, 44, 37, 36)
w.row('backstab_types', 7, 8, 35, 36, 37, 97)
# The DMG's turning undead matrix, undead types 1-10 (skeleton ... vampire) by
# cleric level 1-8, 9-13, 14+: a d20 roll, T = 1, D = 0, D* = -1, none = 99
X, T, D, DS = 99, 1, 0, -1
TURN = [[10, 7, 4, T, T, D, D, DS, DS, DS], [13, 10, 7, T, T, D, D, D, DS, DS], [16, 13, 10, 4, T, T, D, D, D, DS],
        [19, 16, 13, 7, 4, T, T, D, D, D], [20, 19, 16, 10, 7, 4, T, T, D, D], [X, 20, 19, 13, 10, 7, 4, T, T, D],
        [X, X, 20, 16, 13, 10, 7, 4, T, D], [X, X, X, 20, 16, 13, 10, 7, 4, T], [X, X, X, X, 20, 16, 13, 10, 7, T],
        [X, X, X, X, X, 20, 16, 13, 10, 4]]
for i, r in enumerate(TURN):
    w.row('turn%d' % (i + 1), r)
w.done()
