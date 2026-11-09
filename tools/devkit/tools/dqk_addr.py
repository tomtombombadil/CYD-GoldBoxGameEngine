#!/usr/bin/env python3
# The Dark Queen's DQK.EXE (RTLink/Plus overlays): a far call seg:off -> file offset.
# usage: dqk_addr.py <DQK.EXE> seg:off [seg:off ...]
# Resident code: file = 0x2400 + seg x 16 + off. A stub (resident, 10 bytes:
# E8 <call loader>, EA <off> <seg> jump into the overlay, u16 w) names its
# overlay by w; the overlay table's 18-byte rows end at file 0x20A83 (u16 n,
# u16, u16 region seg, u16 0x0DCB, u16 file pos paras, u16, u16 size, u16
# relocation count, u16). w = 0 is the row labelled 0 (its code at 0x22400),
# w >= 1 the row labelled w + 1: code = file pos x 16 + relocations x 4,
# rounded up to 16. In the overlay: code + (seg - region seg) x 16 + off.
import struct, sys

d = open(sys.argv[1], 'rb').read()
END = 0x20A83
rows = {}
p = END - 18
while p > 0:
    r = struct.unpack_from('<9H', d, p)
    if r[3] != 0x0DCB:
        break
    rows[r[0]] = r
    p -= 18


def overlay(w):
    if w == 0:
        return rows[0][2], 0x22400
    r = rows[w + 1]
    return r[2], (r[4] * 16 + r[7] * 4 + 15) & ~15


def resolve(seg, off):
    f = 0x2400 + seg * 16 + off
    if f < 0x218D5 and d[f] == 0xE8 and d[f + 3] == 0xEA:
        o, s, w = struct.unpack_from('<HHH', d, f + 4)
        region, code = overlay(w)
        return code + (s - region) * 16 + o, 'stub -> w%d %04x:%04x' % (w, s, o)
    return f, 'resident'


def inside(seg, off):
    # a call from inside an overlay names the overlay's own segment (no stub):
    # every overlay whose region (region seg + size paras) holds seg
    out = []
    for w in [0] + [n - 1 for n in rows if n >= 2]:
        if w != 0 and (w + 1) not in rows:
            continue
        r = rows[0] if w == 0 else rows[w + 1]
        region, code = overlay(w)
        if region <= seg < region + r[6]:     # (r[6]: its size in paragraphs)
            out.append((w, code + (seg - region) * 16 + off))
    return out


for a in sys.argv[2:]:
    s, o = (int(x, 16) for x in a.split(':'))
    f, how = resolve(s, o)
    print('%s -> file 0x%X (%s)' % (a, f, how))
    if how == 'resident':
        for w, x in inside(s, o):
            print('    (from inside overlay w%d: file 0x%X)' % (w, x))
