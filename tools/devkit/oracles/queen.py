#!/usr/bin/env python3
# oracle_queen.txt for behave queen (behave_queen.inc; GAME=7). The Dark
# Queen of Krynn's own tables and code, read from the player's DQK.EXE (not
# packed; its C data segment DS:0 at file 0x63A20) and DISK2/MONCHA.GLB:
#  DS tables (profile.cpp's Queen layout, its own shapes): race classes
#   0x0328 (29 a race: count, u16 classes in its numbers - 1 the knight),
#   class alignments 0x0563 (21 a class: count, u16 alignments), the level
#   limits 0x17DC (7 a race, its class order), experience 0x0852 (76 a row:
#   i32 for levels 0-18; 11 rows) and its steps 0x028C (i32 a row), the
#   spells 0x27E9 (22 a spell: class byte, -, level), the knights' least
#   stats 0x7D98 / 0x7DA4 (6 x u16).
#  Code (file offsets): createPlayer's experience words (record 0x44 / 0x46,
#   0x3B2A0-0x3B406: a class's own, two's, three's - sorted, highest a
#   single class), its steel (record 0x4C word, 0x3C28F); Knight New Order
#   (0x487F2): each order's least level (cmp byte [es:bx+0xa6]) and
#   experience dword (cmp ax,[..]).
#  MONCHA.GLB (an HLIB library: "HLIB", u32 size, u16 count, a directory
#   entry 0 when byte 11 is 1 - u16 n, n x (u16 id, u16 index); entries:
#   a big-endian size then runs): each monster's experience word 0x54 and
#   flag bytes 0xC7 / 0xC8.
import re, struct
import gb, x86

EXE = gb.find('queen', 'DQK.EXE')
img = open(EXE, 'rb').read()
DSB = 0x63A20
ds = gb.DS(img, DSB)
w = gb.Writer('queen')
w.c('behave queen oracle - tools/devkit/oracles/queen.py')

for race in range(7):
    at = 0x0328 + race * 29
    n = min(ds.u8(at), 14)
    w.row('raceclasses', race, [ds.u16(at + 1 + i * 2) for i in range(n)])
    w.row('cap', race, list(ds.bytes(0x17DC + race * 7, 7)))
for cls in range(17):
    at = 0x0563 + cls * 21
    n = min(ds.u8(at), 10)
    w.row('align', cls, [ds.u16(at + 1 + i * 2) for i in range(n)])
for r in range(11):
    w.row('xp', r, [ds.i32(0x0852 + r * 76 + L * 4) for L in range(19)])
    w.row('xpstep', r, ds.i32(0x028C + r * 4))
for s in range(1, 138):          # (spell 0: no spell - its row is unused)
    at = 0x27E9 + s * 22
    w.row('spell', s, ds.u8(at), ds.u8(at + 2))

c = x86.Code(EXE, 0x3B2A0, 0x166, base=0)
lo = {a: c.imm(a) for a in c.find(r'^mov word \[es:bx\+0x44\],0x')}
hi = {a: c.imm(a) for a in c.find(r'^mov word \[es:bx\+0x46\],0x')}
xps = sorted({hi[h] << 16 | lo[c.after(h)[0]] for h in hi if c.after(h)[0] in lo}, reverse=True)
w.row('startxp', xps)
s = x86.Code(EXE, 0x3C28F, 6, base=0)
w.row('steel', s.imm(s.find(r'^mov word \[es:bx\+0x4c\],0x')[0]))

k = x86.Code(EXE, 0x487F2, 0x400, base=0)
mins = [k.imm(a) for a in k.find(r'^mov ax,0x7d', 0x487F2, 0x48820)]
for order, a in enumerate(k.find(r'^cmp byte \[es:bx\+0xa6\],0x[34]$'), 1):
    xpat = int(re.search(r'\[0x([0-9a-f]+)\]', k.text(k.find(r'^cmp ax,\[0x', a, a + 0x30)[0])).group(1), 16)
    m = mins[order - 1]
    w.row('knight', order, k.imm(a), ds.i32(xpat), [ds.u16(m + i * 2) for i in range(6)])

# the monsters
g = open(gb.find('queen', 'DISK2') + '/MONCHA.GLB', 'rb').read()
assert g[:4] == b'HLIB'
count = struct.unpack_from('<H', g, 8)[0]
directory = g[11] == 1 and count > 0
offs = [struct.unpack_from('<I', g, 16 + 4 * i)[0] for i in range(count + 1)]
ids = {}
if directory:
    d0 = offs[0]
    n = struct.unpack_from('<H', g, d0)[0]
    for j in range(n):
        i_d, idx = struct.unpack_from('<HH', g, d0 + 2 + 4 * j)
        ids[idx] = i_d
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
    if len(q) < 0xC9:
        continue
    w.row('monster', ids.get(i, i), struct.unpack_from('<H', q, 0x54)[0], q[0xC7], q[0xC8])
w.done()
