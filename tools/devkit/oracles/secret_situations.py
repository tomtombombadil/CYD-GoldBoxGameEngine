#!/usr/bin/env python3
# oracle_secret_situations.txt for behave secret's allsituations part
# (behave_secret_situations.inc; with oracle_secret.txt's xp rows). From
# Secret's START.EXE data segment (image 0xDE20; classes::Layout secret):
# spells 0x449C (row byte 1 the class code, 2 the level), class flags
# 0x4C92, masks 0x4C99, max hit dice 0x4CA9, thief skills 0x4CAD / 0x4D3D /
# 0x4D48, stat limits 0x4D9D, race classes 0x4DFF (14 a race), ages 0x4E45,
# age brackets 0x4EFF, class minimums 0x4F45, alignments 0x4FAB, hit dice
# 0x9B0 (count) / 0x9B7 (sides). Races in Secret's own order (1 elf,
# half-elf, dwarf, gnome, halfling, 6 human). The PHB: Constitution's hit
# points (non-fighters), the races' stat adjustments. Facts
# (secret_facts_2.md 21, GAME.OVR 0x1DF40): the races' effects; a new
# magic-user's book; a new cleric's spells 1-8. Silent training's
# magic-user lists by level 6-9: read from its code (GAME.OVR 0x22921: a
# switch on the magic-user level, record 0x116, each case setting the
# record's known-spell bytes - spell n at 0x70 + n, the record's spell
# list from 0x71).
import re
import gb, x86

B = 0xDE20
ds = gb.DS(gb.image('secret'), B)
w = gb.Writer('secret_situations')
w.c('behave secret allsituations oracle - tools/devkit/oracles/secret_situations.py')
for sp in range(1, 120):
    w.row('sit_spell', sp, ds.u8(0x449C + sp * 16 + 1), ds.u8(0x449C + sp * 16 + 2))
w.row('sit_classflags', list(ds.bytes(0x4C92, 7)))
w.row('sit_classmask', list(ds.bytes(0x4C99, 7)))
w.row('sit_maxhd', list(ds.bytes(0x4CA9, 7)))
w.row('sit_hpcount', list(ds.bytes(0x9B0, 7)))
w.row('sit_hpdice', list(ds.bytes(0x9B7, 7)))
for lv in range(1, 19):
    w.row('sit_thiefbase', lv, list(ds.bytes(0x4CAD + lv * 8 + 1, 8)))
for dx in range(3, 20):
    w.row('sit_thiefdex', dx, ds.sbytes(0x4D48 + dx * 5 + 1, 5))
ADJ = {1: [0, 0, 0, 1, -1, 0], 3: [0, 0, 0, 0, 1, -1], 5: [-1, 0, 0, 1, 0, 0]}
FX = {1: [0x5F], 2: [0x12], 3: [0x61, 0x1A, 0x2F], 4: [0x61, 0x07, 0x30], 5: [0x61], 6: []}
for sr in range(1, 7):
    w.row('sit_thiefrace', sr, ds.sbytes(0x4D3D + sr * 8 + 1, 8))
    w.row('sit_racelim', sr, list(ds.bytes(0x4D9D + sr * 16, 16)))
    at = 0x4DFF + sr * 14
    w.row('sit_raceclasses', sr, list(ds.bytes(at + 1, min(ds.u8(at), 13))))
    ages = []
    for e in range(7):
        a = 0x4E45 + sr * 28 + e * 4
        ages += [ds.u16(a), ds.u8(a + 2), ds.u8(a + 3)]
    w.row('sit_raceage', sr, ages)
    w.row('sit_agebr', sr, [ds.u16(0x4EFF + sr * 10 + b * 2) for b in range(5)])
    w.row('sit_raceadj', sr, ADJ.get(sr, [0] * 6))
    w.row('sit_racefx', sr, FX[sr])
for cls in range(17):
    w.row('sit_classmin', cls, list(ds.bytes(0x4F45 + cls * 6, 6)))
    at = 0x4FAB + cls * 10
    w.row('sit_classalign', cls, list(ds.bytes(at + 1, min(ds.u8(at), 9))))
# Slots gained by level 2-18 (the class records DS 0x506C, 187 bytes a class:
# after 17 x i32 experience, 17 x 7)
for name, k in (('cleric', 0), ('paladin', 3), ('ranger', 4), ('mu', 5)):
    for lv in range(2, 19):
        w.row('sit_slots_' + name, lv, list(ds.bytes(0x506C + k * 187 + 68 + (lv - 2) * 7, 7)))
w.row('sit_conhp', [-2, -2, -2, -2, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2])
w.row('sit_mufirst', 0x0B, 0x12, 0x13, 0x15)
st = x86.Code(gb.find('secret', 'GAME.OVR'), 0x22921, 0x100, base=0x22921)
for vals, a, e in x86.cases(st, 0, 0x100):
    sp = [int(re.search(r'\+0x([0-9a-f]+)\]', st.text(x)).group(1), 16) - 0x70
          for x in st.find(r'^mov byte \[es:di\+0x[0-9a-f]+\],0x1$', a, e)]
    for lv in vals:
        w.row('sit_musilent', lv, sp)
w.row('sit_clericfirst', 1, 2, 3, 4, 5, 6, 7, 8)
w.done()
