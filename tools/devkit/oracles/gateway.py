#!/usr/bin/env python3
# oracle_gateway.txt for behave gateway (behave_gateway.inc; GAME=8). From
# Gateway's GAME.EXE data segment (image 0x4C70; profile.cpp's Gateway
# classes::Layout): race classes DS 0x4B9A (14 a race), alignments 0x4D7A
# (10 a class), class records 0x4E3C (99 bytes: i32 experience for levels
# 2-12), the classes' last levels DS 0x1498 (8 bytes); its createPlayer's
# constants (GAME.OVR 0x2650E +0xF63, profile.cpp): 3,000 experience shared
# by the classes, 35 platinum.
# NOT rebuilt: list_limit (the race limits of its training, GAME.OVR overlay
# 0x3EB10 - gateway_facts / gateway_behave_facts) and the menus part's rows
# (the words its shop / temple / treasure overlays offer per state).
import gb

B = 0x4C70
ds = gb.DS(gb.image('gateway', 'GAME.EXE'), B)
w = gb.Writer('gateway')
w.c('behave gateway oracle - tools/devkit/oracles/gateway.py')
for race in range(1, 8):
    at = 0x4B9A + race * 14
    w.row('raceclasses', race, list(ds.bytes(at + 1, min(ds.u8(at), 13))))
for cls in range(17):
    at = 0x4D7A + cls * 10
    w.row('classalign', cls, list(ds.bytes(at + 1, min(ds.u8(at), 9))))
for k in range(8):
    w.row('xp', k, 0, 0, [ds.i32(0x4E3C + k * 99 + i * 4) for i in range(11)])
w.row('level_cap', list(ds.bytes(0x1498, 8)))
w.row('start_xp', 3000)
w.row('platinum', 35)
w.done()
