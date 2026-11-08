#!/usr/bin/env python3
# oracle_champions.txt for behave champions (behave_champions.inc; GAME=5).
# Champions of Krynn's own tables and code, read from the player's files:
#  START.EXE's data segment (image 0xBC60; profile.cpp's Champions Layout):
#   race classes 0x3A50 (15 a race), class alignment masks 0x3C45, the six
#   alignments 0x0A9A, stat limits 0x39E0 (16 a race), age brackets 0x3B99
#   (5 x u16 a race), class minimums 0x3BDF (6 a class), class records
#   0x3C6D (99 bytes: i32 experience for levels 2-12; ten rows), saves
#   0x4056 (class x 60 + level x 5), the knights' least stats 0x0B7E.
#  GAME.OVR's code (overlay seg 0xA4 at file 0x20209 - createPlayer; seg
#   0x65 / 0xDB - the kapak's acid):
#   +0x8C0  the race's effects (a cmp chain; effect = the byte pushed
#           before the far call 0xDB:0x57)
#   +0xC75  the class's start: levels (record 0xF9 + class) and experience
#           (record 0x116) by class code
#   +0x1164 the god's alignment mask ([bp-0x55]) by god
#   +0x1680 the race's adjustment of each 3d6 + 1 roll ([bp-0x15]), the
#           race tested being the race-list loop's last counter ([bp-0x17],
#           +0x7C1 .. its bound at +0x814)
#   +0x171E the age effects per stat (bracket word, jnc: older than it,
#           jc: not older - the young), then limits / class minimums, the
#           Wisdom-13 classes (a Pascal set in the overlay, cs:0x692)
#   +0x219A the steel dice per class (count, sides pushed before 0xDB:0x4D)
#   +0x291E Modify's experience figures (cmp word [es:di+0x116],imm)
#   +0x5952 / +0x5ABD the knights' petitions: least level, the experience
#           dword and the least stats' DS places
#  The knights' saves: the original's row 7 level 0, with Game Fixes the
#  knight's own level 1 row (behave_champions.inc).
import re, sys
import gb, krynn, x86

B = 0xBC60
ds = gb.DS(gb.image('champions'), B)
OVR = gb.find('champions', 'GAME.OVR')
A4 = 0x20209
c = x86.Code(OVR, A4, 0x67C8)
w = gb.Writer('champions')
w.c('behave champions oracle - tools/devkit/oracles/champions.py')

# race classes, alignments, masks
for race in range(7):
    at = 0x3A50 + race * 15
    w.row('raceclasses', race, list(ds.bytes(at + 1, min(ds.u8(at), 14))))
w.row('alignlist', list(ds.bytes(0x0A9A, 6)))
for cls in range(17):
    w.row('classmask', cls, ds.u8(0x3C45 + cls))
for god in range(1, 8):
    mem = x86.run(c, 0x1164, 0x1214, {'al': god}, {})
    w.row('godmask', god, mem['[bp-0x55]'])

# the race's effects
for vals, s, e in x86.cases(c, 0x8C0, 0x9B3):
    fx = [c.imm(c.after(a, -8)[0]) for a in c.find(r'^call 0xdb:0x57$', s, e)]
    for r in vals:
        w.row('racefx', r, fx)
# (a race not in the chain - human - gets none)
w.row('racefx', 6)

# the class's start
for vals, s, e in x86.cases(c, 0xC75, 0xED6):
    lv, xp = [0] * 8, 0
    for a in c.find(r'^mov byte \[es:di\+0x(f9|fa|fb|fc|fd|fe|ff|100)\],0x', s, e):
        lv[int(re.search(r'\+0x([0-9a-f]+)\]', c.text(a)).group(1), 16) - 0xF9] = c.imm(a)
    for a in c.find(r'^mov word \[es:di\+0x116\],0x', s, e):
        xp = c.imm(a)
    for v in vals:
        w.row('start', v, xp, lv)
# (the cleric's own case sets no experience - 0: 2,000 / 1,500 by alignment, +0x13E8)

# steel: dice per class
for vals, s, e in x86.cases(c, 0x219A, 0x2277):
    for a in c.find(r'call 0xdb:0x4d', s, e):
        cnt, sides = c.imm(c.after(a, -4)[0]), c.imm(c.after(a, -2)[0])
        for v in vals:
            w.row('steel', v, cnt, sides)
        break

# experience rows (10)
for r in range(10):
    w.row('xp', r, [ds.i32(0x3C6D + r * 99 + i * 4) for i in range(11)])

# Modify's figures
mod = [c.imm(a) for a in c.find(r'^cmp word \[es:di\+0x116\],0x', 0x2900, 0x29E0)]
w.row('modxp', mod)

# the knights' petitions
for order, lvat in ((1, 0x5952), (2, 0x5ABD)):
    least = c.imm(lvat)
    xpat = [int(re.search(r'\[0x([0-9a-f]+)\]', c.text(a)).group(1), 16)
            for a in c.find(r'^cmp ax,\[0x', lvat, lvat + 0x30)][0]
    minat = [int(re.search(r'\+0x([0-9a-f]+)\]', c.text(a)).group(1), 16)
             for a in c.find(r'^mov dl,\[di\+0x', lvat, lvat + 0x100)][0]
    w.row('knight', order, least, ds.i32(xpat), list(ds.bytes(minat, 6)))
w.row('knightsave', list(ds.bytes(0x4056 + 7 * 60, 5)))
w.row('knightrow1', list(ds.bytes(0x4056 + 7 * 60 + 5, 5)))

# the creation roll (b_krynn_stats): limits, brackets, minimums, age
# effects, race adjustments, the race tested, the Wisdom-13 classes
krynn.stats_rows(w, ds, c, OVR, A4, statlim=0x39E0, agebr=0x3B99, classmin=0x3BDF, stat_switch=(0x171E, 0x2100),
                 adj=(0x1680, 0x16E6, '[bp-0x17]', '[bp-0x1a]', '[bp-0x15]'), tested=(0x7C1, 0x820, 'bp-0x17'),
                 wis_set=0x692)

# the kapak's acid: its ground (0x12C1F + 0x300), effect / time dice pushed
# before 0xDB:0x57 / 0xDB:0x4D, the damage dice (0x34F7F)
k = x86.Code(OVR, 0x12C1F, 0x360)
ground = k.imm(k.find(r'^mov byte \[es:di\+0x7\],0x')[0])
dice = k.find(r'call 0xdb:0x4d')[0]
eff = k.imm(k.after(dice, -6)[0])
tc, ts = k.imm(k.after(dice, -4)[0]), k.imm(k.after(dice, -2)[0])
h = x86.Code(OVR, 0x34F7F, 0x20)
hc, hs = h.imm(h.find(r'^mov al,0x')[0]), h.imm(h.find(r'^mov al,0x')[1])
w.row('acid', ground, eff, tc, ts, hc, hs)
w.done()
