#!/usr/bin/env python3
# find_bytes.py <file> <hex A> [<hex B> <within>]: offsets of byte string A
# (with B no more than `within` bytes after it, when given) - e.g. a
# training routine's "save the old levels" pair 8846E9 ... 8846E8
import sys
d = open(sys.argv[1], 'rb').read()
a = bytes.fromhex(sys.argv[2])
b = bytes.fromhex(sys.argv[3]) if len(sys.argv) > 3 else None
w = int(sys.argv[4]) if len(sys.argv) > 4 else 20
i = d.find(a)
while i >= 0:
    if b is None or b in d[i:i + w]:
        print(hex(i))
    i = d.find(a, i + 1)
