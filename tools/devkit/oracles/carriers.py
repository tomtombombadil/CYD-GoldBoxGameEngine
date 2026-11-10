#!/usr/bin/env python3
# carriers.py <effect hex> [...]   The Dark Queen (rules round 18): who carries
# an effect - read from the player's own files, printed (facts, not an oracle
# file for a check):
#   monsters   DISK2/MONCHA.GLB (an HLIB library, as queen.py reads it): each
#              entry's record (0x19E bytes; its name a C string at 0x68),
#              then its effects (count at 0x19E, 5 bytes each: type, ...)
#              and its items (count at 0x19F, 17 bytes each: byte 16 = 0x80 +
#              code - code 0, "while readied: effect byte 15")
#   items      DISK1/ITEM.DAT (17-byte item records, the same layout)
#   code       DQK.EXE: each "mov al,E / push ax" whose next far call is the
#              effect-giver 0x48F:0x19C2 (its file offset)
import os, re, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gb

REC = 0x19E
want = [int(a, 16) for a in sys.argv[1:]] or [0xB8]
g = open(os.path.join(gb.find('queen', 'DISK2'), 'MONCHA.GLB'), 'rb').read()
assert g[:4] == b'HLIB'
count = struct.unpack_from('<H', g, 8)[0]
directory = g[11] == 1 and count > 0
offs = [struct.unpack_from('<I', g, 16 + 4 * i)[0] for i in range(count + 1)]
ids = {}
if directory:
    n = struct.unpack_from('<H', g, offs[0])[0]
    for j in range(n):
        i_d, idx = struct.unpack_from('<HH', g, offs[0] + 2 + 4 * j)
        ids[idx] = i_d


def name(q):
    s = q[0x68:0x77].split(b'\0')[0]
    return ''.join(chr(c) if 32 <= c < 127 else '.' for c in s).strip()


def item_eff(it):
    return it[15] if it[16] == 0x80 else None


for i in range(1 if directory else 0, count):
    a, e = offs[i], offs[i + 1]
    if e - a < 3:
        continue
    total = g[a] << 8 | g[a + 1]
    q = bytearray()
    p = a + 2
    while p < e and len(q) < total:
        cc = g[p]
        p += 1
        if cc < 0x80:
            q += g[p:p + cc + 1]
            p += cc + 1
        elif p < e:
            q += bytes([g[p]]) * (257 - cc)
            p += 1
    if len(q) < REC + 2:
        continue
    nf, ni = q[REC], q[REC + 1]
    at = REC + 2
    effs = [q[at + 5 * k] for k in range(nf) if at + 5 * k + 5 <= len(q)]
    at += 5 * nf
    its = [q[at + 17 * k:at + 17 * k + 17] for k in range(ni) if at + 17 * k + 17 <= len(q)]
    for w in want:
        if w in effs:
            print('monster %d (%s): effect 0x%02X in its list' % (ids.get(i, i), name(q), w))
        for it in its:
            if item_eff(it) == w:
                print('monster %d (%s): item type %d carries 0x%02X while readied' % (ids.get(i, i), name(q), it[0], w))
d = open(os.path.join(gb.find('queen', 'DISK1'), 'ITEM.DAT'), 'rb').read()
for k in range(len(d) // 17):
    it = d[17 * k:17 * k + 17]
    for w in want:
        if item_eff(it) == w:
            print('ITEM.DAT %d: item type %d carries 0x%02X while readied' % (k, it[0], w))
x = open(gb.find('queen', 'DQK.EXE'), 'rb').read()
for w in want:
    for m in re.finditer(bytes([0xB0, w, 0x50]), x):
        f = x.find(b'\x9a', m.end(), m.end() + 0x20)
        if f >= 0 and x[f + 1:f + 5] == struct.pack('<HH', 0x19C2, 0x48F):
            print('DQK.EXE 0x%X: effect 0x%02X given (0x48F:0x19C2)' % (m.start(), w))
