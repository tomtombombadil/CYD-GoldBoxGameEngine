#!/usr/bin/env python3
# List Turbo Pascal overlay stubs in an unpacked image: seg, file offset, code size
# usage: ovls.py <image> [file offset to locate]
import struct, sys
img = open(sys.argv[1], 'rb').read()
want = int(sys.argv[2], 16) if len(sys.argv) > 2 else None
for seg in range(0, len(img) // 16):
    s = seg * 16
    if img[s:s + 2] != b'\xcd\x3f' or img[s + 0x20:s + 0x22] != b'\xcd\x3f':
        continue
    fo = struct.unpack_from('<I', img, s + 4)[0]
    size = struct.unpack_from('<H', img, s + 8)[0]
    if want is None:
        print('seg %04X file 0x%X size 0x%X' % (seg, fo, size))
    elif fo <= want < fo + size:
        print('seg %04X file 0x%X size 0x%X  (+0x%X)' % (seg, fo, size, want - fo))
