#!/usr/bin/env python3
# oracle_secret.txt for behave secret (behave_secret.inc; GAME=3). Secret of
# the Silver Blades' own tables from the player's GAME.EXE... START.EXE
# (data segment at image 0xDE20; classes::Layout secret in profile.cpp):
# THAC0 0x4C0C [class][19 levels 0-18], class records 0x506C (187 bytes: 17 x
# i32 experience for levels 2-18), saves 0x5592 [class * 90 + level * 5],
# hit dice sides 0x9B7. The facts (secret_facts_2.md, the original's
# code): Alter's Level multipliers (hit points x (level + 2) / 4), VAULT.DAT's
# layout (7 coin counts of 4 bytes, 67-byte items), Power Word Stun's rounds
# by hit points, Death Spell's cost by Hit Dice, Delayed Blast Fireball's
# level d6 + level.
import gb

B = 0xDE20
THAC0, RECORDS, SAVES, HPDICE = 0x4C0C, 0x506C, 0x5592, 0x9B7
ds = gb.DS(gb.image('secret'), B)
w = gb.Writer('secret')
w.c('behave secret oracle - tools/devkit/oracles/secret.py')
for k in range(7):
    w.row('xp', k, [ds.i32(RECORDS + k * 187 + i * 4) for i in range(17)])
    w.row('thac0', k, list(ds.bytes(THAC0 + k * 19, 19)))
    for lv in range(1, 19):
        w.row('save', k, lv, list(ds.bytes(SAVES + k * 90 + lv * 5, 5)))
w.row('hpdice', list(ds.bytes(HPDICE, 7)))
w.row('level_mul', 2, 3, 4, 5, 6)
w.row('vault', 28, 67)
w.row('pws', 30, 4, 4, 60, 2, 4, 90, 1, 4)
w.row('death_cost', [1] + [hd * hd for hd in range(1, 9)])
w.row('dbf', 6, 1)
w.done()
