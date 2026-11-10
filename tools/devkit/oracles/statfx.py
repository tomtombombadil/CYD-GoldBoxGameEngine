#!/usr/bin/env python3
# The stats' recalculation's effects (round 16), read from a game's GAME.OVR
# (Darkness, Treasures, Secret): oracle_statfx_<game>.txt -
#   str <effects in order>     the Strength effects looked for (spell, giant, enlarge)
#   fighter <record offsets>   the levels that make an 18/xx (an offset each, the game's record)
#   con <effect> <at> <minutes> <data>   the effect a Constitution of <at>+ keeps on
#   cha <effect>               its data added to Charisma
#   intwis <effect> <value>    Intelligence / Wisdom set while it lasts
# The routine is found by its shape (the Strength part's "mov al,EFF / push
# ax / lea di,[bp-8]"), walked back to its push bp. Values from the player's
# file only. usage: statfx.py <GAME.OVR> <game> [outdir]
import re, subprocess, sys

ovr, game = sys.argv[1], sys.argv[2]
outdir = sys.argv[3] if len(sys.argv) > 3 else '/tmp/claude-0/bt'
d = open(ovr, 'rb').read()
LINE = re.compile(r'^([0-9A-F]{8})\s+([0-9A-F]+)\s+(.*)$')
k = d.find(bytes.fromhex('B026508D7E'))
assert k > 0, 'the recalculation not found'
start = max(d.rfind(bytes.fromhex('5589E5'), k - 0x400, k), d.rfind(bytes.fromhex('558BEC'), k - 0x400, k))
assert start > 0
o = subprocess.run(['ndisasm', '-b16', '-o', str(start), '-e', str(start), ovr], capture_output=True, text=True,
                   check=True).stdout
code = []
for ln in o.splitlines():
    m = LINE.match(ln)
    if m:
        code.append((int(m.group(1), 16), m.group(3).strip()))
        if m.group(3).strip().startswith('retf'):
            break
looked = []          # (address, effect) in order: "mov al,EFF / push ax / lea di,[bp-0x8]"
for i in range(len(code) - 2):
    m = re.match(r'mov al,0x([0-9a-f]+)$', code[i][1])
    if m and code[i + 1][1] == 'push ax' and code[i + 2][1].startswith('lea di,[bp-0x'):
        looked.append((code[i][0], int(m.group(1), 16)))
# the fighter test: the first run of such compares after the first Strength effect
first = looked[0][0]
fl = [int(re.match(r'cmp byte \[es:di\+0x([0-9a-f]+)\],0x0$', t).group(1), 16) for a, t in code
      if a > first and re.match(r'cmp byte \[es:di\+0x(1[0-9a-f]{2})\],0x0$', t)]
fl = fl[:6]
fl = [x for x in fl if x - fl[0] < 0x10]
# Constitution: "cmp byte [es:di+0x19],0x14" then mov al,EFF / mov ax,MIN / mov al,DATA
con = None
for i, (a, t) in enumerate(code):
    m = re.match(r'cmp byte \[es:di\+0x19\],0x([0-9a-f]+)$', t)
    if m:
        vals = [re.match(r'mov a[lx],0x([0-9a-f]+)$', tt) for _, tt in code[i + 1:i + 12]]
        vals = [int(v.group(1), 16) for v in vals if v]
        con = (vals[0], int(m.group(1), 16), vals[1], vals[2])
        break
cha = intwis = None
for i, (a, t) in enumerate(code):
    if re.match(r'mov \[es:di\+0x1b\],al', t):
        cha = [e for aa, e in looked if aa < a][-1]
    m = re.match(r'mov byte \[bp-0x[0-9a-f]+\],0x([0-9a-f]+)$', t)
    if m and intwis is None and looked and a > looked[-1][0] and looked[-1][1] == 0x44:
        intwis = ([e for aa, e in looked if aa < a][-1], int(m.group(1), 16))
strs = [e for a, e in looked if con is None or e not in (con[0],)][:3]
lines = ['# oracle_statfx_%s - %s recalculation at 0x%X' % (game, ovr.split('/')[-1], start),
         'str ' + ' '.join('0x%02X' % e for e in strs),
         'fighter ' + ' '.join('0x%X' % x for x in fl),
         'con 0x%02X %d %d 0x%02X' % con if con else 'con none',
         'cha 0x%02X' % cha if cha is not None else 'cha none',
         'intwis 0x%02X %d' % intwis if intwis else 'intwis none']
open('%s/oracle_statfx_%s.txt' % (outdir, game), 'w').write('\n'.join(lines) + '\n')
print('\n'.join(lines))
