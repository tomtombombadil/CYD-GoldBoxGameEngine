#!/usr/bin/env python3
# ovrdis.py <GAME.OVR> <offset hex> <bytes hex> [string base hex]
# A 16-bit disassembly of a game's overlay file (capstone; python with capstone
# in /tmp/claude-0/uvenv), "push ax" lines dropped; with a base, each
# "mov di, imm" that lands on a Pascal string at base + imm shows the string
# (an overlay's own words - its code segment starts at base). Facts only: the
# output stays in scratch.
import re, sys
import capstone
d = open(sys.argv[1], 'rb').read()
a, n = int(sys.argv[2], 16), int(sys.argv[3], 16)
base = int(sys.argv[4], 16) if len(sys.argv) > 4 else None
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)


def pstr(o):
    if o < 0 or o >= len(d) or not d[o]:
        return None
    s = d[o + 1:o + 1 + d[o]]
    return s.decode('latin1') if all(32 <= c < 127 for c in s) else None


for i in md.disasm(d[a:a + n], a):
    if i.mnemonic == 'push' and i.op_str == 'ax':
        continue
    t = '%05x %s %s' % (i.address, i.mnemonic, i.op_str)
    m = re.match(r'di, (0x[0-9a-f]+)$', i.op_str)
    if base is not None and i.mnemonic == 'mov' and m:
        s = pstr(base + int(m.group(1), 16))
        if s:
            t += "   ; '%s'" % s
    print(t)
