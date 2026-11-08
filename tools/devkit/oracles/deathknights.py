#!/usr/bin/env python3
# oracle_deathknights.txt for behave deathknights (behave_deathknights.inc;
# GAME=6). Death Knights of Krynn's own tables and code:
#  START.EXE's data segment (image 0xBCE0; profile.cpp's Death Knights
#   Layout): race classes 0x7423 (15 a race), class alignment masks 0x7618,
#   the six alignments 0x0584, stat limits 0x73B3, age brackets 0x756C,
#   class minimums 0x75B2, class records 0x7638 (204 bytes: u32 experience
#   by the level reached, + level x 4; 11 rows), the race x class level
#   limits 0x82D4 (8 a race).
#  GAME.OVR's createPlayer (overlay seg 0x61 at file 0x15004):
#   +0xBA3  the class's start: the experience (record 0x62 / 0x64 words) by
#           class code - a class's own, shared by two / three
#   +0x2091 the magic-user's spells (its case of the class loop: the
#           record's spell bytes set), counted
#   +0x2154 the steel (record 0x3F word)
#   +0x1586 / +0x1624 / +0x6A6 / cs:0x5A5 the roll (krynn.py)
#   +0x4FC5 / +0x5125 the knights' petitions: least level, the experience
#           dword and the least stats' DS places
import re
import gb, krynn, x86

B = 0xBCE0
ds = gb.DS(gb.image('knights'), B)
OVR = gb.find('knights', 'GAME.OVR')
O61 = 0x15004
c = x86.Code(OVR, O61, 0x5CE9)
w = gb.Writer('deathknights')
w.c('behave deathknights oracle - tools/devkit/oracles/deathknights.py')

for race in range(7):
    at = 0x7423 + race * 15
    w.row('raceclasses', race, list(ds.bytes(at + 1, min(ds.u8(at), 14))))
    w.row('cap', race, list(ds.bytes(0x82D4 + race * 8, 8)))
w.row('alignlist', list(ds.bytes(0x0584, 6)))
for cls in range(17):
    w.row('classmask', cls, ds.u8(0x7618 + cls))

# the start: experience by the number of classes (the class switch's words)
xps = []
for vals, s, e in x86.cases(c, 0xBA3, 0xE22):
    lo = [c.imm(a) for a in c.find(r'^mov word \[es:di\+0x62\],0x', s, e)]
    hi = [c.imm(a) for a in c.find(r'^mov word \[es:di\+0x64\],0x', s, e)]
    if lo and hi:
        v = lo[0] | hi[0] << 16
        if v not in xps:
            xps.append(v)
w.row('startxp', sorted(xps, reverse=True))

# the magic-user's spells (case 5 of the class loop at +0x2091)
for vals, s, e in x86.cases(c, 0x2091, 0x2148):
    if 5 in vals:
        w.row('muspells', len(c.find(r'^mov byte \[es:di\+0x[0-9a-f]+\],0x1$', s, e)))
w.row('steel', c.imm(c.find(r'^mov word \[es:di\+0x3f\],0x', 0x2140, 0x2180)[0]))

for r in range(11):
    w.row('xp', r, [ds.i32(0x7638 + r * 204 + L * 4) for L in range(19)])

for order, lvat in ((1, 0x4FC5), (2, 0x5125)):
    least = c.imm(lvat)
    xpat = [int(re.search(r'\[0x([0-9a-f]+)\]', c.text(a)).group(1), 16)
            for a in c.find(r'^cmp ax,\[0x', lvat, lvat + 0x30)][0]
    minat = [int(re.search(r'\+0x([0-9a-f]+)\]', c.text(a)).group(1), 16)
             for a in c.find(r'^mov dl,\[di\+0x', lvat, lvat + 0x100)][0]
    w.row('knight', order, least, ds.i32(xpat), list(ds.bytes(minat, 6)))

krynn.stats_rows(w, ds, c, OVR, O61, statlim=0x73B3, agebr=0x756C, classmin=0x75B2, stat_switch=(0x1624, 0x1FC1),
                 adj=(0x1586, 0x15EC, '[bp-0x18]', '[bp-0x1b]', '[bp-0x15]'), tested=(0x6A6, 0x700, 'bp-0x18'),
                 wis_set=0x5A5)
w.done()
