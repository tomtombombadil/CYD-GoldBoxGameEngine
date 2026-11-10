#!/usr/bin/env python3
# The second step back's target finder (round 16), every DAX game: in its
# GAME.OVR the count-2 call (pushes 0xFF, 1, 0 [, 0], a far call, then
# "or al,al / jnz" over the guard's near call) resolved through the unpacked
# program's overlay stub; the finder read: its tries per pass (mov byte
# [bp-3],N), the sight-skip flag set in its second pass and tested with the
# second argument ("mov byte [es:di+6],1" / "cmp byte [es:di+6],0").
#   oracle_finder_<game>.txt: count2 call <at> finder <at> args <n> range 0xFF skiparg 1 tries <N> scan <0|1>
#   pass2skip <0|1> (scan: one random place, then the list in order - Darkness-type)
# usage: finder.py <GAME.OVR> <unpacked program image> <game> [outdir]
import re, struct, sys

ovr, img, game = sys.argv[1:4]
outdir = sys.argv[4] if len(sys.argv) > 4 else '/tmp/claude-0/bt'
d = open(ovr, 'rb').read()
im = open(img, 'rb').read()
line = 'count2 none'
for m in re.finditer(re.escape(bytes.fromhex('B0FF50B00150B00050')), d):
    p, n = m.end(), 3
    if d[p:p + 3] == bytes.fromhex('B00050'):
        p, n = p + 3, 4
    if d[p] != 0x9A or d[p + 5:p + 9] != bytes.fromhex('08C0750D'):
        continue
    off, seg = struct.unpack_from('<HH', d, p + 1)
    s = seg * 16
    if im[s:s + 2] != b'\xcd\x3f':
        continue
    t = struct.unpack_from('<I', im, s + 4)[0] + struct.unpack_from('<H', im, s + off + 2)[0]
    body = d[t:t + 0x300]
    tries = re.search(rb'\xC6\x46\xFD(.)', body, re.S)
    skip = int(re.search(rb'\xC6\x45\x06\x01', body) is not None and re.search(rb'\x80\x7D\x06\x00', body) is not None)
    # Darkness-type: the random place drawn once, the tries = the list's count
    # (mov al,[bp-2] / mov [bp-3],al) - a scan round the list from there
    scan = int(tries is None and bytes.fromhex('8A46FE8846FD') in body)
    line = 'count2 call 0x%X finder 0x%X args %d range 0xFF skiparg 1 tries %d scan %d pass2skip %d' % (
        m.start(), t, n, tries.group(1)[0] if tries else 0, scan, skip)
    break
open('%s/oracle_finder_%s.txt' % (outdir, game), 'w').write('# oracle_finder_%s\n%s\n' % (game, line))
print(game, line)
