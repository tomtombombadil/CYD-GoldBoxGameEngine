#!/usr/bin/env python3
# The rest's "has memorized" message routine in each DAX game (Tom,
# 2026-11-11: rest messages as the originals). Finds the far call after the
# message's words in GAME.OVR, decodes the Turbo Pascal overlay stub it goes
# through (in the unpacked program), and lists the routine's row pushes and
# far calls - which rows the name / words / spell go on, and whether a delay
# follows. Facts only, printed; nothing from the games is kept.
# usage: restmsg.py <GAME.OVR> <unpacked program image> [search words]
import re, struct, subprocess, sys

ovr = open(sys.argv[1], 'rb').read()
img = open(sys.argv[2], 'rb').read()
words = (sys.argv[3] if len(sys.argv) > 3 else 'has memorized').encode()
at = ovr.find(words)
if at < 0:
    sys.exit('words not found')

def dis(data, origin, start, n=400):
    out = subprocess.run(['ndisasm', '-b16', '-o', str(origin), '-e', str(start), '-'], input=data[:start + n],
                         capture_output=True).stdout.decode().splitlines()
    return [l.split(None, 2) for l in out if len(l.split(None, 2)) == 3]

# the first far call into a stub segment after the words (the message routine)
seg = off = None
for a, _, ins in dis(ovr, at, at, 300):
    m = re.match(r'call (0x[0-9a-f]+):(0x[0-9a-f]+)', ins)
    if m:
        s, o = int(m.group(1), 16), int(m.group(2), 16)
        if img[s * 16:s * 16 + 2] == b'\xcd\x3f':   # an overlay stub
            seg, off = s, o
            break
if seg is None:
    sys.exit('no stub call found after the words')
hdr = img[seg * 16:seg * 16 + 0x20]
fo = struct.unpack_from('<I', hdr, 4)[0]
entry = img[seg * 16 + off:seg * 16 + off + 5]
code = struct.unpack_from('<H', entry, 2)[0]
print('words at 0x%x; stub %x:%x -> overlay file 0x%x + 0x%x = 0x%x' % (at, seg, off, fo, code, fo + code))
rows, calls = [], []
for a, _, ins in dis(ovr[fo:], 0, code, 600):
    if ins.startswith('push') or ins.startswith('mov al,') or ins.startswith('mov ax,'):
        rows.append(ins)
    if ins.startswith('call') or ins.startswith('retf'):
        calls.append('%s %s' % (a, ins))
        if ins.startswith('retf'):
            break
print('calls:', '; '.join(calls))
print('small values pushed:', ' '.join(r for r in rows if re.search(r',0x1[0-9a-f]$|,0x[0-9a-f]$', r))[:400])
