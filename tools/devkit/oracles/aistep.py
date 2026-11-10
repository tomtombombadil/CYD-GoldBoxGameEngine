#!/usr/bin/env python3
# aistep.py [game ...]   (the eight DAX games; none: all eight)
# The computer's step in each DAX game, read from its GAME.OVR's code (rules
# round 15) - for sweep's behave rules4 ONLY=aimood. Found by their shapes:
#   - the try (one of the mood row's columns): "push bp / mov bp,sp / sub sp"
#     just before a read of the row table (mood x 5 + column + table - shl
#     di,1 x2 / add di,si / add di,dx, or Secret's mul by 5), to its retf 0xc;
#   - the step: the same table read a second time (the way it went, its
#     reverse test), after the flee's way from the party's facing
#     (mov al,[facing] / inc ax x2 / cwd / mov cx,4 - Secret: add ax,2).
# Output /tmp/claude-0/bt/oracle_aistep_<game>.txt, a line each:
#   rows <ds hex>                the row table (rows 0-6 of 5 columns + 1)
#   try <ovr hex> step <ovr hex> where they are (for the facts)
#   facing <ds hex> inc <0|1>    the flee's way: fd - ((fd + 2) % 4) / 2 (+ 1), + 4 for the party, & 7
#   cost strict | cost pool      strict: the step's cost < the moves left; pool: the
#                                ground's value < the moves and the cost <= the moves
#   cloud <kind> <ground|flag> eff <hex ...> driven <0|1> save <0|1> hd7 <0|1> flags2 <hex>
#                                a cloud square (kind stink / poison / knives) taken
#                                only by one with one of the effects, driven (combat
#                                +0x10 / Darkness-type +0xF: turned, run in confusion),
#                                flags2 & the mask, 7+ Hit Dice (hd7: under 7 never),
#                                or (save 1) a save against poison made; otherwise never
import os, re, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import x86

ALL = ['poolrad', 'curse', 'secret', 'darkness', 'champions', 'knights', 'gateway', 'treasures']
if len(sys.argv) < 3 or not os.path.exists(sys.argv[2]):
    import gb
    for g in sys.argv[1:] or ALL:
        args = [g, gb.find(g, 'GAME.OVR'), os.path.join(gb.OUT, 'oracle_aistep_%s.txt' % g)]
        subprocess.check_call([sys.executable, '-I', os.path.abspath(__file__)] + args)
    sys.exit(0)
game, path, out = sys.argv[1], sys.argv[2], sys.argv[3]
b = open(path, 'rb').read()

# the table reads: shl di,1 / shl di,1 / add di,si / add di,dx / mov al,[di+T]
# or mov dx,5 / mul dx / mov di,ax / add di,cx / mov al,[di+T]
READ = re.compile(rb'\xd1\xe7\xd1\xe7\x01\xf7\x03\xfa\x8a\x85(..)|\xba\x05\x00\xf7\xe2\x8b\xf8\x03\xf9\x8a\x85(..)', re.S)
reads = {}
for m in READ.finditer(b):
    t = int.from_bytes(m.group(1) or m.group(2), 'little')
    reads.setdefault(t, []).append(m.start())
# the table read twice within 0x1000 bytes, the first in a routine starting
# just before it (the try), the second after a flee way (the step)
FLEE = re.compile(rb'\xa0(..)\x30\xe4(?:\x40\x40|\x05\x02\x00)\x99\xb9\x04\x00', re.S)
found = None
for t, at in reads.items():
    for i in range(len(at) - 1):
        a, s = at[i], at[i + 1]
        if s - a > 0x1000 or s < a:
            continue
        st = b.rfind(b'\x55\x89\xe5', a - 0x60, a)
        fl = FLEE.search(b, s - 0x200, s)
        if st >= 0 and fl:
            found = (t, st, s, fl)
            break
    if found:
        break
if not found:
    raise SystemExit('%s: no step rows found' % game)
rows, try_at, step_at, fl = found
facing = int.from_bytes(fl.group(1), 'little')
# after the subtraction (sub ax,dx): an inc ax?
sub = b.find(b'\x2b\xc2', fl.end(), fl.end() + 0x30)
inc = 1 if sub >= 0 and b[sub + 2] == 0x40 else 0

code = x86.Code(path, try_at, 0x300, base=0)
L = code.ls
end = next(a for a, _, t in L if t.startswith('retf'))
lines = [(a, t) for a, _, t in L if a <= end]

# the cost test: the first compare with the moves byte ([es:di+6] / +5)
cost = None
for k, (a, t) in enumerate(lines):
    if re.match(r'cmp al,\[es:di\+0x[56]\]$', t):
        prev = lines[k - 3][1] + ' ' + lines[k - 2][1] + ' ' + lines[k - 1][1]
        cost = 'pool' if re.search(r'mov al,\[di\+0x', prev) else 'strict'
        break

clouds = []                     # [kind, key, effs, driven, save, hd7, flags2]
cur = None
# the square's query (the first far call): the places it fills - the first
# two the edge / ground and the one there; then Curse-type: the stinking and
# the poison cloud flags; Darkness-type: the cloud's ground value; PoolRad:
# none (the ground itself, [bp-0x9], is compared)
l0 = next(k for k, (a, t) in enumerate(lines) if t.startswith('lea di,[bp-'))
q = next(k for k, (a, t) in enumerate(lines) if k > l0 and t.startswith('call 0x'))
outs = [re.search(r'\[bp-0x(\w+)\]', t).group(1) for a, t in lines[:q] if t.startswith('lea di,[bp-')]
flags = outs[2:]
def start(kind, key):
    global cur
    cur = [kind, key, [], 0, 0, 0, 0]
    clouds.append(cur)
for k, (a, t) in enumerate(lines):
    m = re.match(r'cmp byte \[bp-0x(\w+)\],0x0$', t)
    if m and len(flags) == 2 and m.group(1) in flags:
        start('stink' if m.group(1) == flags[0] else 'poison', 'flag')
        continue
    m = re.match(r'cmp byte \[bp-0x%s\],0x(\w+)$' % outs[1], t)
    if m and not flags and int(m.group(1), 16):
        start('stink', '%x' % int(m.group(1), 16))      # (PoolRad: the ground value itself)
        continue
    m = re.match(r'cmp al,0x(1[bcd])$', t)
    if m:                       # Darkness-type: the cloud's ground value
        g = int(m.group(1), 16)
        start({0x1d: 'stink', 0x1c: 'poison', 0x1b: 'knives'}[g], '%x' % g)
        continue
    if cur is None:
        continue
    m = re.match(r'mov al,0x(\w+)$', t)
    if m and k + 3 < len(lines) and lines[k + 2][1].startswith('lea di,[bp-') and lines[k + 5][1].startswith('call 0x'):
        cur[2].append(int(m.group(1), 16))
        continue
    if re.match(r'cmp byte \[es:di\+0x(10|f)\],0x0$', t):
        cur[3] = 1
        continue
    if re.match(r'cmp byte \[es:di\+0x\w+\],0x7$', t):
        cur[5] = 1
        continue
    m = re.match(r'and al,0x(\w+)$', t)
    if m:
        cur[6] = int(m.group(1), 16)
        continue
    # a save against poison: two zeros pushed, then a far call (the save)
    if t.startswith('call 0x') and k >= 4 and lines[k - 4][1] == 'mov al,0x0' and lines[k - 2][1] == 'mov al,0x0':
        cur[4] = 1

with open(out, 'w') as f:
    f.write('# %s: the computer\'s step (GAME.OVR try 0x%X, step 0x%X)\n' % (game, try_at, step_at))
    f.write('rows %x\n' % rows)
    f.write('try %x step %x\n' % (try_at, step_at))
    f.write('facing %x inc %d\n' % (facing, inc))
    f.write('cost %s\n' % cost)
    for kind, key, effs, drv, sv, hd, fl2 in clouds:
        f.write('cloud %s %s eff %s driven %d save %d hd7 %d flags2 %x\n' %
                (kind, key, ' '.join('%x' % e for e in effs) or '-', drv, sv, hd, fl2))
print(open(out).read(), end='')
