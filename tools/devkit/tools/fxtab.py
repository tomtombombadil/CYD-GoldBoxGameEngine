#!/usr/bin/env python3
# The far-pointer handler tables a DAX game's GAME.OVR fills at run time
# (runs of "mov ax, off; mov dx, seg; mov [t], ax; mov [t + 2], dx"), each
# entry resolved through its overlay stub to a GAME.OVR file offset.
# usage: fxtab.py <game> [table DS offset] [first id] [ids hex ...]
#   no table: lists the tables found (DS address, entries, where filled)
#   a table: prints "id -> GAME.OVR offset" for the ids (all when none given)
# (2026-11-13, the round 22 effects sweep: which handler an effect id has)
import os, re, struct, sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'oracles'))
import gb

game = sys.argv[1]
exe = {'darkness': 'GAME.EXE', 'treasures': 'GAME.EXE', 'gateway': 'GAME.EXE'}.get(game, 'START.EXE')
img = gb.image(game, exe)
ovr = open(gb.find(game, 'GAME.OVR'), 'rb').read()


def stub(seg, off):
    s = seg * 16
    if img[s:s + 2] != b'\xcd\x3f' or img[s + off:s + off + 2] != b'\xcd\x3f':
        return None
    return struct.unpack_from('<I', img, s + 4)[0] + struct.unpack_from('<H', img, s + off + 2)[0]


fills = [(m.start(), struct.unpack('<HHH', m.group(1) + m.group(2) + m.group(3)))
         for m in re.finditer(rb'\xb8(..)\xba(..)\xa3(..)\x89\x16', ovr, re.S)]
entries = {}
for at, (off, seg, addr) in fills:
    entries.setdefault(addr, (off, seg, at))
if len(sys.argv) < 3:
    runs, i = [], 0
    fs = sorted(fills)
    while i < len(fs):
        j = i
        while j + 1 < len(fs) and fs[j + 1][0] - fs[j][0] <= 16:
            j += 1
        if j - i + 1 >= 30:
            addrs = sorted(a for _, (_, _, a) in fs[i:j + 1])
            runs.append((addrs[0], addrs[-1], j - i + 1, fs[i][0]))
        i = j + 1
    for lo, hi, n, at in runs:
        print('table DS 0x%X - 0x%X: %d entries, filled at GAME.OVR 0x%X' % (lo, hi, n, at))
    sys.exit()
table = int(sys.argv[2], 16)
first = int(sys.argv[3], 16) if len(sys.argv) > 3 else 1
ids = [int(x, 16) for x in sys.argv[4:]] or range(first, first + 256)
for e in ids:
    a = table + 4 * (e - first)
    if a not in entries:
        if sys.argv[4:]:
            print('0x%02X: none' % e)
        continue
    off, seg, at = entries[a]
    print('0x%02X -> %s  (seg %04X:%04X)' % (e, '0x%X' % stub(seg, off) if stub(seg, off) else 'not a stub', seg, off))
