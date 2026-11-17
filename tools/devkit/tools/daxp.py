#!/usr/bin/env python3
# daxp.py <in.DAX> <out.DAX> <block id> <find hex> <replace hex>: one block's bytes patched (scratch copies only)
import struct, sys
src, dst, bid, find, repl = sys.argv[1], sys.argv[2], int(sys.argv[3]), bytes.fromhex(sys.argv[4]), bytes.fromhex(sys.argv[5])
d = open(src, 'rb').read()
hs = struct.unpack_from('<H', d, 0)[0]
ents = [struct.unpack_from('<BIHH', d, 2 + i * 9) for i in range(hs // 9)]
base = 2 + hs
def unrle(b, raw):
    o = bytearray(); i = 0
    while i < len(b) and len(o) < raw:
        n = b[i] if b[i] < 128 else b[i] - 256; i += 1
        if n >= 0: o += b[i:i + n + 1]; i += n + 1
        else: o += bytes([b[i]]) * (-n); i += 1
    return bytes(o[:raw])
def rle(b):
    o = bytearray(); i = 0; lit = bytearray()
    def flush():
        nonlocal lit
        for k in range(0, len(lit), 127):
            c = lit[k:k + 127]; o.append(len(c) - 1); o.extend(c)
        lit = bytearray()
    while i < len(b):
        j = i
        while j < len(b) and b[j] == b[i] and j - i < 127: j += 1
        if j - i >= 3:
            flush(); o.append(256 - (j - i)); o.append(b[i]); i = j
        else:
            lit.append(b[i]); i += 1
    flush()
    return bytes(o)
blocks = []
for (i, off, raw, comp) in ents:
    data = d[base + off: base + off + comp]
    if i == bid:
        u = unrle(data, raw)
        n = u.count(find); print('block', i, 'raw', raw, 'found', n, 'at', hex(u.find(find)))
        assert n == 1 and len(find) == len(repl)
        u = u.replace(find, repl); data = rle(u)
    blocks.append((i, raw, data))
out = bytearray(struct.pack('<H', hs)); off = 0; body = bytearray()
for (i, raw, data) in blocks:
    out += struct.pack('<BIHH', i, off, raw, len(data)); body += data; off += len(data)
open(dst, 'wb').write(out + body)
