#!/usr/bin/env python3
# Resolve Turbo Pascal overlay stub calls seg:off -> GAME.OVR file offset
# usage: stub.py <unpacked exe image> seg:off [...]
import struct, sys
img = open(sys.argv[1], 'rb').read()
for a in sys.argv[2:]:
    seg, off = (int(x, 16) for x in a.split(':'))
    s = seg * 16
    h = img[s:s + 0x20]
    if h[0:2] != b'\xcd\x3f':
        print(a, 'not a stub:', h[:16].hex()); continue
    fileofs = struct.unpack_from('<I', h, 4)[0]
    e = img[s + off:s + off + 5]
    if e[0:2] != b'\xcd\x3f':
        print(a, 'entry not int 3f', e.hex()); continue
    tgt = struct.unpack_from('<H', e, 2)[0]
    print('%s -> ovr file 0x%X (overlay at 0x%X + 0x%X)' % (a, fileofs + tgt, fileofs, tgt))
