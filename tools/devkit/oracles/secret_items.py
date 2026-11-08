#!/usr/bin/env python3
# oracle_secret_items.txt for behave secret's allitems part
# (behave_secret_items.inc). From the player's files: the ITEMS type table
# (2-byte head, 16 bytes a type: slot, hands, large dice / sides / bonus,
# attacks, AC, blow, -, dice, sides, bonus, range, classes, flags), START.EXE's
# data segment (image 0xDE20): the class flags DS 0x4C92, the random
# treasure rows DS 0x0A2A (7 rows of 8 words - profile random_rows), the
# random scrolls' spells (mage's DS 0x585C, 11 bytes a level; clerics' DS
# 0x580C, 10 a level - profile kSecretMake). AD&D's Strength / Dexterity
# (gb.py, the groups numbered as there); the +1 types of GAME.OVR 0x382C6.
import struct
import gb

B = 0xDE20
ds = gb.DS(gb.image('secret'), B)
items = gb.raw('secret', 'ITEMS')
w = gb.Writer('secret_items')
w.c('behave secret allitems oracle - tools/devkit/oracles/secret_items.py')
t = 0
while 2 + t * 16 + 16 <= len(items):
    r = items[2 + t * 16:2 + t * 16 + 16]
    sb = lambda b: b - 256 if b > 127 else b
    w.row('it_type', t, r[0], r[1], r[2], r[3], sb(r[4]), r[5], r[6], r[7], r[9], r[10], sb(r[11]), r[12], r[13], r[14])
    t += 1
w.row('it_class_flags', list(ds.bytes(0x4C92, 7)))
for r in range(7):
    w.row('it_row', r, [ds.u16(0x0A2A + r * 16 + k * 2) for k in range(8)])
for lv in range(1, 8):
    # (each list up to its first 0 - the die is the list's length)
    for key, at, n in (('it_muspells', 0x585C + (lv - 1) * 11 + 1, 11), ('it_clspells', 0x580C + (lv - 1) * 10 + 1, 10)):
        v = list(ds.bytes(at, n))
        w.row(key, lv, v[:v.index(0)] if 0 in v else v)
w.row('it_plus1_types', 22, 23, 24, 25, 26, 18, 19)
# Strength: "it_strgroup Str 18/xx-edge group", "it_str group hit damage allowance"
for s in range(3, 26):
    if s != 18:
        w.row('it_strgroup', s, 0, s if s <= 17 else s + 5)
for e, g in ((0, 18), (1, 19), (50, 19), (51, 20), (75, 20), (76, 21), (90, 21), (91, 22), (99, 22), (100, 23)):
    w.row('it_strgroup', 18, e, g)
for g in range(31):
    w.row('it_str', g, gb.STR_HIT[g], gb.STR_DMG[g], gb.STR_ENC[g])
for d in range(26):
    w.row('it_dex', d, gb.DEX_AC[d], gb.DEX_MISSILE[d])
# The original's code (secret_facts_2.md 14, GAME.OVR offsets in the .inc):
# armour's movement by weight (to 150 as the character's, to 399 9, more 6,
# +3 when magic), carrying past the allowance (512 / 768 / 1024: 9 / 6 / 3),
# Sell (value / 2, arrows / quarrels x count / 20), the Gauntlets of Ogre
# Power 18/00, the girdle: 0x3D 0 18/00, else hill 19 ... storm 24 (AD&D)
w.row('it_armour_move', 150, 399, 9, 6, 3)
w.row('it_load', 0x200, 0x300, 0x400, 9, 6, 3)
w.row('it_sell', 2, 20)
w.row('it_ogre', 18, 100)
w.row('it_girdle', 18, 100, 19, 20, 21, 22, 23, 24)
w.done()
