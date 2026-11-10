#!/usr/bin/env python3
# qstep.py [DQK.EXE]   The Dark Queen's computer step (rules round 19), read
# from DQK.EXE's own code for sweep's behave rules4 ONLY=aimood - the same
# lines as aistep.py's (oracle_aistep_queen.txt) and the round's new ones.
# Found by their shapes (file offsets; DQK.EXE has no EXEPACK, its C data
# segment DS:0 at file 0x63A20):
#   - the row table read twice: "mov al,[es:bx+0x16] / mov ah,0 / mov dx,6 /
#     mul dx / mov bx,ax / add bx,T" (combat byte 0x16's row of 6 bytes) - the first in the try (its routine start
#     just before it), the second in the step routine (the one after it);
#   - the step routine: the flee's way (mov al,[F] / mov ah,0 / inc ax x2 /
#     mov bx,4 / cwd / idiv bx ... sub dl,al / inc dl), the NPC test (test
#     byte [es:bx+C],0x80 / a d100 / mov dl,[M] ... mov dl,[H]) and the
#     magic-user's (cmp word [es:bx+K],byte +0x5 after the running byte's
#     test, a pointer test between them or not);
#   - the try: its cost test (mov al,[es:bx+0x7] / cmp al,[bp-0x4] / ja:
#     strict) and its cloud grounds' switch (cmp ax,0x1b / 0x1c / 0x1d):
#     each branch's effects (mov al,E before the has-effect far call), the
#     driven byte (cmp byte [es:bx+0x11],0x0), flags 2 (test byte
#     [es:bx+0xc8],F), run into the next branch's code where it falls through.
# Output (ORACLE_DIR, default /tmp/claude-0/bt) oracle_aistep_queen.txt:
#   rows <ds> stride 6 / rowbytes <7 x 5 bytes: row m's columns 1-5>
#   try <file> step <file> / facing <ds> inc <0|1> / cost strict
#   cloud <stink|poison|knives> <ground> eff <..> driven d save 0 hd7 0 flags2 f
#   npc <control byte> fig <ds> health <ds> side1 <0|1>
#   mage class <offset> armour <0|1> then guard   (armour 1: a pointer test first)
#   morale fig <ds> cap <0|1>             (the morale routine: cap 1 - over 102 counts as 0)
#   ground46 hd7 <0|1>                    (the try's other flag: under 7 Hit Dice, not driven - no step)
import os, re, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import x86

if len(sys.argv) > 1:
    path = sys.argv[1]
else:
    import gb
    path = gb.find('queen', 'DQK.EXE')
OUT = os.environ.get('ORACLE_DIR', '/tmp/claude-0/bt')
DS = 0x63A20
b = open(path, 'rb').read()

# (the mood: combat byte 0x16 - mov al,[es:bx+0x16] / mov ah,0 first)
READ = re.compile(rb'\x26\x8a\x47\x16\xb4\x00\xba\x06\x00\xf7\xe2\x8b\xd8\x81\xc3(..)', re.S)
reads = {}
for m in READ.finditer(b):
    reads.setdefault(struct.unpack_from('<H', m.group(1))[0], []).append(m.start())
found = None
for t, at in reads.items():
    for i in range(len(at) - 1):
        if 0 < at[i + 1] - at[i] < 0x800:
            found = (t, at[i], at[i + 1])
            break
    if found:
        break
if not found:
    raise SystemExit('queen: no step rows found')
rows, r1, r2 = found
try_at = b.rfind(b'\x55\x8b\xec', r1 - 0x80, r1)
step_at = b.rfind(b'\x55\x8b\xec', r2 - 0x480, r2)


def routine(start):
    c = x86.Code(path, start, 0x800, base=0)
    end = next(a for a, _, t in c.ls if t.startswith('retf'))
    return [(a, t) for a, _, t in c.ls if a <= end]


S = routine(step_at)
T = routine(try_at)

# the flee's way
facing = inc = None
for k, (a, t) in enumerate(S):
    m = re.match(r'mov al,\[0x(\w+)\]$', t)
    if m and k + 6 < len(S) and S[k + 2][1] == 'inc ax' and S[k + 3][1] == 'inc ax' and S[k + 4][1] == 'mov bx,0x4':
        facing = int(m.group(1), 16)
        for j in range(k + 5, k + 20):
            if S[j][1].startswith('sub dl,al'):
                inc = 1 if S[j + 1][1] == 'inc dl' else 0
                break
        break

# the NPC test
npc = None
for k, (a, t) in enumerate(S):
    m = re.match(r'test byte \[es:bx\+0x(\w+)\],0x80$', t)
    if not m:
        continue
    ctrl = int(m.group(1), 16)
    seg = [x for _, x in S[k:k + 30]]
    if 'mov al,0x64' not in seg:
        continue
    places = [int(re.match(r'mov dl,\[0x(\w+)\]$', x).group(1), 16) for x in seg if re.match(r'mov dl,\[0x(\w+)\]$', x)]
    side1 = 1 if any(re.match(r'cmp word \[es:bx\+0x66\],byte \+0x1$', x) for x in seg) else 0
    if len(places) >= 2:
        npc = (ctrl, places[0], places[1], side1)
    break

# the magic-user
mage = None
for k, (a, t) in enumerate(S):
    m = re.match(r'cmp word \[es:bx\+0x(\w+)\],byte \+0x5$', t)
    if not m:
        continue
    back = [x for _, x in S[max(0, k - 8):k]]
    run = any(re.match(r'cmp byte \[es:bx\+0x15\],0x0$', x) for x in back)
    ptr = any(x.startswith('or ax,[es:bx+') for x in back)
    if run:
        nx = [x for _, x in S[k + 2:k + 4]]
        mage = (int(m.group(1), 16), 1 if ptr else 0, 'guard' if nx and nx[0].startswith('jmp') else 'unknown')
    break

# the try: the cost test
cost = None
for k, (a, t) in enumerate(T):
    if t == 'mov al,[es:bx+0x7]' and k + 2 < len(T) and T[k + 1][1] == 'cmp al,[bp-0x4]':
        cost = 'strict' if T[k + 2][1].startswith('ja ') else 'other'
        break

# the cloud switch: cmp ax,G / jz-jnz to each branch
branch = {}
for k, (a, t) in enumerate(T):
    m = re.match(r'cmp ax,0x(1[bcd])$', t)
    if not m:
        continue
    g = int(m.group(1), 16)
    j = T[k + 1][1]
    mj = re.match(r'(jz|jnz) 0x(\w+)$', j)
    if not mj:
        continue
    if mj.group(1) == 'jz':
        tgt = int(mj.group(2), 16)
    else:   # jnz past a jmp: the jmp's target (the near jumps' low 16 bits: the try's own 64 KB)
        nxt = T[k + 2][1]
        mm = re.match(r'jmp (?:short )?0x(\w+)$', nxt)
        tgt = int(mm.group(1), 16) if mm and g != 0x1d else T[k + 2][0]
    branch[g] = (tgt & 0xFFFF) | (try_at & ~0xFFFF)
addrs = [a for a, _ in T]
ends = sorted(branch.values())
endall = None
for a, t in T:
    if t == 'cmp byte [bp-0x1],0x0':
        endall = a
        break
clouds = []
for g, st in sorted(branch.items(), key=lambda x: -x[0]):
    effs, drv, fl2 = [], 0, 0
    i = addrs.index(st)
    while i < len(T) and T[i][0] != endall:
        a, t = T[i]
        if a != st and a in ends and g == 0x1d:
            break
        m = re.match(r'mov al,0x(\w+)$', t)
        if m and i + 4 < len(T) and any('call 0x77e:0x1734' in T[i + q][1] for q in range(1, 5)):
            effs.append(int(m.group(1), 16))
        if re.match(r'cmp byte \[es:bx\+0x11\],0x0$', t):
            drv = 1
        m = re.match(r'test byte \[es:bx\+0xc8\],0x(\w+)$', t)
        if m:
            fl2 = int(m.group(1), 16)
        i += 1
    clouds.append(({0x1d: 'stink', 0x1c: 'poison', 0x1b: 'knives'}[g], g, effs, drv, fl2))

# the other flag ([bp-0x1]): under 7 Hit Dice
g46 = 0
if endall:
    i = addrs.index(endall)
    for a, t in T[i:i + 8]:
        if re.match(r'cmp byte \[es:bx\+0x91\],0x7$', t):
            g46 = 1

# the morale routine's figure: and al,0x7f / shl al,1 / mov [M],al - then
# cmp byte [M],0x66 (the 102 test) or not
cap = None
mm = re.search(rb'\x24\x7f\xd0\xe0\xa2(..)', b, re.S)
if mm:
    cap = (struct.unpack_from('<H', mm.group(1))[0], 1 if b[mm.end():mm.end() + 2] == b'\x80\x3e' and b[mm.end() + 4] == 0x66 else 0)

rb = b[DS + rows:DS + rows + 42]
rowbytes = [rb[m * 6 + c] for m in range(7) for c in range(1, 6)]
out = os.path.join(OUT, 'oracle_aistep_queen.txt')
with open(out, 'w') as f:
    f.write('# queen: the computer\'s step (DQK.EXE try 0x%X, step routine 0x%X)\n' % (try_at, step_at))
    f.write('rows %x stride 6\n' % rows)
    f.write('rowbytes %s\n' % ' '.join('%x' % v for v in rowbytes))
    f.write('try %x step %x\n' % (try_at, step_at))
    f.write('facing %x inc %d\n' % (facing or 0, -1 if inc is None else inc))
    f.write('cost %s\n' % (cost or 'unknown'))
    for kind, g, effs, drv, fl2 in clouds:
        f.write('cloud %s %x eff %s driven %d save 0 hd7 0 flags2 %x\n' % (kind, g, ' '.join('%x' % e for e in effs) or '-', drv, fl2))
    if npc:
        f.write('npc %x fig %x health %x side1 %d\n' % npc)
    if mage:
        f.write('mage class %x armour %d then %s\n' % mage)
    if cap:
        f.write('morale fig %x cap %d\n' % cap)
    f.write('ground46 hd7 %d\n' % g46)
print(open(out).read(), end='')
