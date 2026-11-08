#!/usr/bin/env python3
# The DOSBox differ's original with a FIXED SEED: a scratch copy of the
# player's START.EXE written un-EXEPACKed (the image unpacked by the repo's
# exepack - tools/devkit/oracles/gb.py image() - and an MZ header rebuilt with
# the relocations of EXEPACK's packed table, the 16 segment groups after the
# stub's "Packed file is corrupt"), then Randomize's "mov ah,2Ch / int 21h"
# (the DOS time as the seed) replaced by "xor cx,cx / xor dx,dx" - seed 0.
# Only offsets live here; the output goes to the scratch DOS folder given.
# Usage: fixseed.py <short> <dos folder of the game>   (poolrad)
import os, struct, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'oracles'))
import gb

RANDOMIZE = {'poolrad': ('START.EXE', 0xC1C7)}       # image offset of "B4 2C CD 21"
short, out = sys.argv[1], sys.argv[2]
exe, at = RANDOMIZE[short]
packed = gb.raw(short, exe)
img = bytearray(gb.image(short, exe))
h = struct.unpack_from('<14H', packed, 0)
hdr, cs, ip = h[4], h[11], h[10]
xp = hdr * 16 + cs * 16
real_ip, real_cs, mem_start, xsize, real_sp, real_ss, dest_len = struct.unpack_from('<7H', packed, xp)
k = packed.find(b'Packed file is corrupt', xp) + len('Packed file is corrupt')
relocs = []
for seg in range(16):
    n = struct.unpack_from('<H', packed, k)[0]
    k += 2
    for i in range(n):
        relocs.append((struct.unpack_from('<H', packed, k + 2 * i)[0], seg * 0x1000))
    k += 2 * n
assert img[at:at + 4] == b'\xb4\x2c\xcd\x21', 'Randomize not at %X' % at
img[at:at + 4] = b'\x31\xc9\x31\xd2'                 # xor cx,cx; xor dx,dx
hsize = (0x1C + 4 * len(relocs) + 15) // 16
total = hsize * 16 + len(img)
orig_par = (h[2] * 512 - (512 - h[1] if h[1] else 0)) // 16 - hdr + h[5]   # (load paragraphs + min alloc)
min_alloc = max(0, orig_par - (len(img) + 15) // 16, dest_len - (len(img) + 15) // 16)
head = struct.pack('<14H', 0x5A4D, total % 512, (total + 511) // 512, len(relocs), hsize, min_alloc, 0xFFFF, real_ss,
                   real_sp, 0, real_ip, real_cs, 0x1C, 0)
head += b''.join(struct.pack('<HH', o, s) for o, s in relocs)
head += b'\0' * (hsize * 16 - len(head))
with open(os.path.join(out, exe), 'wb') as f:
    f.write(head + bytes(img))
print('%s: %d relocations, header 0x%X, Randomize at file 0x%X seeded 0' % (exe, len(relocs), hsize * 16, hsize * 16 + at))
