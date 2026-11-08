#!/usr/bin/env python3
# oracle_items.txt for behave items (behave_items.inc): AD&D's Strength and
# Dexterity tables (gb.py), the armour table (AD&D AC by armour type), the
# elves' bow / sword types and the class bits a class sets (Curse: its
# START.EXE DS 0x3EA2, 8 bytes - classes::Layout class_flags).
# Usage: items.py [curse|poolrad]   (the record checks also need ITEMS_BIN: items_bin.sh)
import sys
import gb

short = sys.argv[1] if len(sys.argv) > 1 else 'curse'
# (DS base, class_flags) - profile.cpp's classes::Layout
LAYOUT = {'curse': (0xABE0, 0x3EA2)}
# The item types (the games' ITEMS numbering, coab's list) and AD&D's AC
ARMOUR = {50: 8, 51: 8, 52: 7, 53: 7, 54: 6, 55: 5, 56: 4, 57: 4, 58: 3, 95: 5}
ELF_TYPES = [41, 42, 43, 44, 37, 36]       # bows (long, short, composite) and the long / short swords

w = gb.Writer('items')
w.c('behave items oracle for %s - AD&D tables, the program\'s class bits (tools/devkit/oracles/items.py)' % short)
w.row('str_hit', [gb.STR_HIT[g] for g in range(31)])
w.row('str_dmg', [gb.STR_DMG[g] for g in range(31)])
w.row('str_enc', [gb.STR_ENC[g] for g in range(31)])
w.row('dex_ac', [gb.DEX_AC[d] for d in range(26)])
w.row('dex_missile', [gb.DEX_MISSILE[d] for d in range(26)])
w.row('elf_types', ELF_TYPES)
if short in LAYOUT:
    base, flags = LAYOUT[short]
    ds = gb.DS(gb.image(short), base)
    w.row('class_bits', list(ds.bytes(flags, 8)))
# The DMG's gem and jewellery tables (d100): gems "top value ...", jewellery
# "top base range ..." (base + 0..range-1, the DMG's spans)
w.row('gems', 25, 10, 50, 50, 70, 100, 90, 500, 99, 1000, 100, 5000)
w.row('jewels', 10, 100, 900, 20, 200, 1000, 40, 300, 1500, 50, 500, 2500, 70, 1000, 5000, 90, 2000, 6000,
      100, 2000, 10000)
for t, ac in sorted(ARMOUR.items()):
    w.row('armour', t, ac)
w.done()
