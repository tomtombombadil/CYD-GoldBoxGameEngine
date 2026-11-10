#!/usr/bin/env python3
# Death Knights' on-hit specials (round 16), read from its GAME.OVR:
#   oracle_knights_hit.txt -
#     event2 <effects>   the hook switch's case 2 (attack 1's hit) list
#     event3 <effects>   case 3 (attack 2's)
#     needs_damage <0|1> the blow calls the event (slot + 1) only past
#                        "cmp byte [damage],0 / jna" - a blow of no damage has none
#     any_side <0|1>     no side test between the damage test and the call
# The switch found by shape (mov al,[bp+0xa] / cmp al,0 / jnz / jmp, then
# "cmp al,N / jnz" cases of "mov al,EFF / push ax / push cs / call"), the
# caller by "mov al,[bp-x] / xor ah,ah / inc ax / push ax".
# usage: knights_hit.py <GAME.OVR> [outdir]
import re, subprocess, sys

ovr = sys.argv[1]
outdir = sys.argv[2] if len(sys.argv) > 2 else '/tmp/claude-0/bt'
d = open(ovr, 'rb').read()
LINE = re.compile(r'^([0-9A-F]{8})\s+([0-9A-F]+)\s+(.*)$')


def dis(foff, n):
    o = subprocess.run(['ndisasm', '-b16', '-o', str(foff), '-e', str(foff), ovr], capture_output=True, text=True,
                       check=True).stdout
    out = []
    for ln in o.splitlines():
        m = LINE.match(ln)
        if m:
            a = int(m.group(1), 16)
            if a >= foff + n:
                break
            out.append((a, m.group(3).strip()))
    return out


sw = None
for m in re.finditer(re.escape(bytes.fromhex('8A460A3C007503E9')), d):
    # the one whose case 2's list holds a run of effects
    code = dis(m.start(), 0x400)
    if sum(1 for _, t in code if re.match(r'mov al,0x4[0-9a-f]$', t)) >= 4:
        sw = m.start()
        break
assert sw, 'the hook switch not found'
cases, cur, last = {}, None, None
for a, t in dis(sw, 0x900):
    m = re.match(r'cmp al,0x([0-9a-f]+)$', t)
    if m:
        cur = int(m.group(1), 16)
        cases.setdefault(cur, [])
        continue
    m = re.match(r'mov al,0x([0-9a-f]+)$', t)
    if m:
        last = int(m.group(1), 16)
    if t.startswith('call 0x') and cur is not None and last is not None:
        cases[cur].append(last)
        last = None
need = side = 0
for m in re.finditer(rb'\x8A\x46(.)\x30\xE4\x40\x50', d, re.S):
    before = d[m.start() - 0x20:m.start()]
    if bytes.fromhex('803E') in before and b'\x76' in before:
        need = 1
        side = int(bytes.fromhex('2683') not in before and b'\x97\x01' not in before)
        break
lines = ['# oracle_knights_hit - GAME.OVR hook switch at 0x%X' % sw,
         'event2 ' + ' '.join('0x%02X' % e for e in cases.get(2, [])),
         'event3 ' + ' '.join('0x%02X' % e for e in cases.get(3, [])),
         'needs_damage %d' % need, 'any_side %d' % side]
open('%s/oracle_knights_hit.txt' % outdir, 'w').write('\n'.join(lines) + '\n')
print('\n'.join(lines))
