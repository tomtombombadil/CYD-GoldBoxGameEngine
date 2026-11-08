#!/usr/bin/env python3
# oracle_secret_spells.txt for behave secret's allspells part
# (behave_secret_spells.inc). From Secret's START.EXE (data segment at image
# 0xDE20): the spell table's rows (DS 0x449D + 16 x spell: the 15 bytes
# after each row's own first byte), the slots gained by level (the class
# records DS 0x506C, 187 bytes a class: after 17 x i32 experience, 17 x 7 for
# levels 2-18). The PHB: Wisdom's bonus spells, the Wisdom / Intelligence
# 6th and 7th level spells need. The facts of the original's code
# (secret_facts_2.md): Sleep's costs by Hit Dice and its 4d4, Enlarge's
# Strength by caster level (12th on: 23).
# NOT rebuilt (the facts are in the Project, not in the repo): sp_word (where
# each spell's word is in GAME.OVR), sp_dur_fixed / sp_dur_dice (the special
# durations of GAME.OVR 0x2D5E2), sp_ground (the clouds' ground values) -
# see EXTRA below: rows added from a file of your own (EXTRA=<file>).
import os
import gb

B, SPELLS, RECORDS = 0xDE20, 0x449C, 0x506C
ds = gb.DS(gb.image('secret'), B)
w = gb.Writer('secret_spells')
w.c('behave secret allspells oracle - tools/devkit/oracles/secret_spells.py')
for s in range(1, 120):
    r = list(ds.bytes(SPELLS + s * 16 + 1, 15))
    r[2] = ds.s8(SPELLS + s * 16 + 3)          # (the range: signed)
    w.row('sp_row', s, r)
for k in range(7):
    for lv in range(2, 19):
        w.row('sp_slots', k, lv, list(ds.bytes(RECORDS + k * 187 + 68 + (lv - 2) * 7, 7)))
for wis, sl in ((13, 1), (14, 1), (15, 2), (16, 2), (17, 3), (18, 4)):
    w.row('sp_wis', wis, sl)
w.row('sp_cl67', 17, 18)
w.row('sp_mu67', 12, 14)
w.row('sp_sleep_cost', 1, 1, 2, 4, 6, 20, 20, 20)
w.row('sp_sleep_budget', 4, 4)
ENL = {1: (18, 0), 2: (18, 1), 3: (18, 51), 4: (18, 76), 5: (18, 91), 6: (18, 100), 7: (19, 0), 8: (20, 0),
       9: (21, 0), 10: (22, 0), 11: (22, 0)}
for lv in range(1, 19):
    w.row('sp_enlarge', lv, *ENL.get(lv, (23, 0)))
# The special durations (GAME.OVR 0x2D5E2, overlay 0x2C64B + 0xF97, read with
# tools/dis.sh): spell 0x1A a fixed 0xEC4; 0x28 d6 x 10; 0x39 / 0x3D 5d4;
# 0x3B d4 x 10 + 40 ("spell n sides plus mult"); the rest the row's time + time a level
w.row('sp_dur_fixed', 0x1A, 0xEC4)
w.row('sp_dur_dice', 0x28, 1, 6, 0, 10)
w.row('sp_dur_dice', 0x39, 5, 4, 0, 1)
w.row('sp_dur_dice', 0x3D, 5, 4, 0, 1)
w.row('sp_dur_dice', 0x3B, 1, 4, 40, 10)
# The clouds' ground: Stinking Cloud's (GAME.OVR 0x2F39F) and Cloudkill's
# (0x31A43) "mov byte [es:di+7], n" - n read from the player's GAME.OVR
ovr = gb.raw('secret', 'GAME.OVR')
assert ovr[0x2F39F:0x2F3A3] == b'\x26\xc6\x45\x07' and ovr[0x31A43:0x31A47] == b'\x26\xc6\x45\x07'
w.row('sp_ground', ovr[0x2F3A3], ovr[0x31A47])
extra = os.environ.get('EXTRA')
if extra and os.path.exists(extra):
    w.lines += [l.rstrip('\n') for l in open(extra)]
w.done()
