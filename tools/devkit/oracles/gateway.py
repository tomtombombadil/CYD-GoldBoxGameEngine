#!/usr/bin/env python3
# oracle_gateway.txt for behave gateway (behave_gateway.inc; GAME=8). From
# Gateway's GAME.EXE data segment (image 0x4C70; profile.cpp's Gateway
# classes::Layout): race classes DS 0x4B9A (14 a race), alignments 0x4D7A
# (10 a class), class records 0x4E3C (99 bytes: i32 experience for levels
# 2-12), the classes' last levels DS 0x1498 (8 bytes); its createPlayer's
# constants (GAME.OVR 0x2650E +0xF63, profile.cpp): 3,000 experience shared
# by the classes, 35 platinum.
# list_limit: the party list's race limits (GAME.OVR overlay 0x3EB10 +0x3866,
# run for every race / class / level / Strength 16-18 - the record's race
# 0x74, the class [bp+6], the level [bp-2], Strength 0x11; the result [bp-1]):
# race, class, level, when (0 always, 1 Strength 17, 2 under 17, 3 under 18).
# The menus' words (shopword / templeword / treasureword: key, need - 0 always,
# 1 the member has items, 2 coins on the ground, 3 the party has coins, 4
# coins or items on the ground): each menu builder's Pascal strings (cs:off,
# its overlay) and the test guarding each concat - the far call 0xCB:0x52
# (var coins, var items: the ground's 7 coin dwords DS 0x822C / its item
# list DS 0x8248), 0xCB:0x7A (a member has coins), the member's item count
# (record 0x14C). The treasure menu's "Detect" (offered when a member can
# detect) is left out: its rows can't say it.
import re
import gb, x86

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

# the party list's race limits
OVR = gb.find('gateway', 'GAME.OVR')
fe = x86.Code(OVR, 0x3EB10, 0x3B42)
for race in range(1, 8):
    for k in range(8):
        for lvl in range(1, 13):
            hit = [x86.run(fe, 0x3866, 0x3974, {'al': 0}, {'[es:di+0x74]': race, '[bp+0x6]': k, '[bp-0x2]': lvl,
                                                           '[es:di+0x11]': st}).get('[bp-0x1]', 0) for st in (16, 17, 18)]
            when = {(1, 1, 1): 0, (0, 1, 0): 1, (1, 0, 0): 2, (1, 1, 0): 3}.get(tuple(hit))
            if hit != [0, 0, 0]:
                assert when is not None, (race, k, lvl, hit)
                w.row('list_limit', race, k, lvl, when)


def menu(key, ovl, start, stop, coins_first):
    """The words a builder appends and the test before each (its overlay at ovl)."""
    c = x86.Code(OVR, ovl + start, stop - start, base=ovl)
    raw = open(OVR, 'rb').read()
    # the two var flags of CB:52 (pushed first = coins)
    call = c.find(r'^call 0xcb:0x52$')[0]
    v1 = re.search(r'\[bp-0x[0-9a-f]+\]', c.text(c.after(call, -6)[0])).group(0)
    v2 = re.search(r'\[bp-0x[0-9a-f]+\]', c.text(c.after(call, -3)[0])).group(0)
    flag = {v1: 'c', v2: 'i'}
    guards = []                         # (from, to, set of flags any of which)
    rows, tail = [], []
    for a, _, t in c.ls:
        m = re.match(r'cmp byte (\[bp-0x[0-9a-f]+\]),0x0$', t)
        if m and m.group(1) in flag:
            j = c.after(a)[2].split()
            if j[0] == 'jz':
                prev = guards[-1] if guards and guards[-1][1] is None else None
                if prev:
                    guards[-1] = (prev[0], x86.num(j[-1]), prev[2] | {flag[m.group(1)]})
                else:
                    guards.append((a, x86.num(j[-1]), {flag[m.group(1)]}))
            elif j[0] == 'jnz':
                guards.append((a, None, {flag[m.group(1)]}))
        elif t == 'call 0xcb:0x7a' and c.after(a, 2)[2].startswith('jz'):
            guards.append((a, x86.num(c.after(a, 2)[2].split()[-1]), {'p'}))
        elif re.match(r'cmp byte \[es:di\+0x14c\],0x0$', t) and c.after(a)[2].startswith('jna'):
            guards.append((a, x86.num(c.after(a)[2].split()[-1]), {'n'}))
    for a in c.find(r'^mov di,0x[0-9a-f]+$'):
        if c.after(a)[2] != 'push cs':
            continue
        off = c.imm(a)
        g = [x for x in guards if x[1] is not None and x[0] < a < x[1]]
        need = 0
        if g:
            f = g[-1][2]
            need = {frozenset('n'): 1, frozenset('c'): 2, frozenset('p'): 3, frozenset('ci'): 4}[frozenset(f)]
        word = raw[ovl + off + 1:ovl + off + 1 + raw[ovl + off]].decode('latin-1')
        if word in ('Exit', 'Detect Exit'):
            # (the treasure menu's ending, picked first and appended last)
            tail = [(ord('E'), 0)]
            continue
        for part in word.split():
            k = re.search(r'~(.)', part)
            rows.append((ord(k.group(1) if k else part[0]), need))
    for r in rows + tail:
        w.row(key, *r)


menu('shopword', 0x892A, 0x710, 0x8D2, True)
menu('templeword', 0x58CB, 0xEE0, 0x1000, True)
menu('treasureword', 0x6B85, 0x1240, 0x13F4, True)
w.done()
