#!/usr/bin/env python3
# tohit.py [game ...]   (each game's files from scratch via gb.py; none: all nine)
# tohit.py <game> <GAME.OVR | DQK.EXE> [<unpacked START.EXE> <data base hex>] [out]
# The side's bonus in each game's attacker-against-target to-hit roll, read
# from the original's code (round 11) - for sweep's behave rules4 ONLY=tohit.
# The routine is found by its shape: the second "d20 kept in a DS byte, a 20
# made 100" (cmp byte [d],0x14 / jnz / mov byte [d],0x64) in the file. In it:
#   - the attacker's side test (cmp [es:di+X],0 - X the record's side byte),
#   - the party's branch: its bonus byte (game block / area offset), then the
#     difficulty switch (a cmp chain or, in DQK.EXE, a jump table) adding to it,
#   - the others' branch: their bonus byte.
# Output /tmp/claude-0/bt/oracle_tohit_<game>.txt, a line each:
#   party <vm address>      the party's bonus: the engine's script address
#   monsters <vm address>   the others'
#   difficulty <vm address> | difficulty const <value>   what the switch tests
#   adj <value> <delta>     the switch's addition for each value 0..6
# (VM addresses from the games' memory: Curse-type area words 0x7C00 + off/2 /
# 0x4B00 + off/2 - the second pointer the difficulty's; Death Knights' byte
# map 0x7C00 + the DS word's value; Darkness / Treasures game byte off + 1;
# the Dark Queen's game block off itself.)
import os, re, struct, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import x86

PAT = re.compile(rb'\x80\x3e..\x14\x75\x05\xc6\x06..\x64', re.S)

ALL = ['poolrad', 'curse', 'secret', 'darkness', 'champions', 'knights', 'queen', 'gateway', 'treasures']
if len(sys.argv) < 3 or not os.path.exists(sys.argv[2]):
    # each game by its short name: one run of this script a game
    import gb
    for g in sys.argv[1:] or ALL:
        args = [g, gb.find(g, 'DQK.EXE' if g == 'queen' else 'GAME.OVR')]
        if g == 'knights':                   # (its DS words: START.EXE's data, base 0xBCE0 - profile.cpp)
            gb.image('knights')
            args += [os.path.join(gb.PROG, 'knights_start.bin'), 'BCE0']
        args.append(os.path.join(gb.OUT, 'oracle_tohit_%s.txt' % g))
        subprocess.check_call([sys.executable, '-I', os.path.abspath(__file__)] + args)
    sys.exit(0)
game = sys.argv[1]
path = sys.argv[2]
b = open(path, 'rb').read()
start_img = open(sys.argv[3], 'rb').read() if len(sys.argv) > 4 else None
data_base = int(sys.argv[4], 16) if len(sys.argv) > 4 else 0
out = sys.argv[-1] if len(sys.argv) in (4, 6) else '/tmp/claude-0/bt/oracle_tohit_%s.txt' % game

ms = list(PAT.finditer(b))
if len(ms) < 2:
    raise SystemExit('no to-hit routine')
at = ms[1].start()
code = x86.Code(path, at, 0x140, base=0)
L = code.ls

def first(rx, frm, to=None):
    r = re.compile(rx)
    for a, _, t in L:
        if a >= frm and (to is None or a < to) and r.search(t):
            return a, r.search(t)
    return None, None

def tgt(a, t):
    # a jump's target: ndisasm prints 16 bits; the routine's own 64 KB
    x = x86.num(t.split()[-1])
    y = (a & ~0xFFFF) | x
    return y + 0x10000 if y < a - 0x8000 else y - 0x10000 if y > a + 0x8000 else y

def ds_word(o):
    return struct.unpack_from('<H', start_img, data_base + o)[0]

# the side test and the two branches
side_at, m = first(r'cmp (?:byte|word) \[es:(?:di|bx)\+0x([0-9a-f]+)\],(?:byte \+)?0x0$', at)
jnz_at = code.after(side_at)[0]
else_at = tgt(jnz_at, code.text(jnz_at))
join_at, _ = first(r'cmp byte \[0x[0-9a-f]+\],0x0$', else_at)    # the d20 >= 0 test after both

def bonus_place(frm, to):
    """The first byte the branch loads into [bp-1] / [bp-2]: its VM address."""
    a, mm = first(r'mov al,\[es:(?:di|bx)(?:([+-])0x([0-9a-f]+))?\]$', frm, to)
    off = 0 if not mm.group(2) else int(mm.group(2), 16) * (1 if mm.group(1) == '+' else -1)
    a2, m2 = first(r'mov ax,\[0x([0-9a-f]+)\]$', frm, a)
    if a2 is not None:                       # Death Knights: base + a DS word holding the place
        return a, 0x7C00 + ds_word(int(m2.group(1), 16))
    if game in ('darkness', 'treasures'):
        return a, off + 1
    if game == 'queen':
        return a, off
    return a, 0x7C00 + off // 2

pa, party = bonus_place(jnz_at, else_at)
ea, mons = bonus_place(else_at, join_at)

# the difficulty switch in the party's branch
lines = ['party 0x%X' % party, 'monsters 0x%X' % mons]
adj = {}
ca, cm = first(r'mov ax,(0x[0-9a-f]+)$', pa, else_at)
qa, qm = first(r'jmp \[cs:bx\+(0x[0-9a-f]+)\]$', pa, else_at)
if qa is not None:
    # the Dark Queen: mov al,[es:bx+D] / mov ah,0 / dec ax / cmp ax,n / ja / ... / jmp [cs:bx+T]
    da, dm = first(r'mov al,\[es:bx\+0x([0-9a-f]+)\]$', pa + 1, qa)
    lines.append('difficulty 0x%X' % int(dm.group(1), 16))
    na, nm = first(r'cmp ax,(0x[0-9a-f]+)$', da, qa)
    n = x86.num(nm.group(1))
    seg = 0x6CF0 if at < 0x218D5 else None   # the resident segment 0x48F's base (DQK.EXE 0x2400 + 0x48F0)
    if seg is None:
        raise SystemExit('to-hit routine not in the resident code')
    table = x86.num(qm.group(1))
    for v in range(0, 7):
        k = v - 1
        if k < 0 or k > n:
            adj[v] = 0
            continue
        t = seg + struct.unpack_from('<H', b, seg + table + 2 * k)[0]
        ins = subprocess.run(['ndisasm', '-b16', '-o', str(t), '-e', str(t), path], capture_output=True,
                             text=True).stdout.splitlines()[0].split(None, 2)[2]
        mm = re.match(r'(add|sub|inc|dec) byte \[bp-0x[12]\](?:,(0x[0-9a-f]+))?', ins)
        adj[v] = 0 if not mm else {'inc': 1, 'dec': -1}.get(mm.group(1)) or \
            (x86.num(mm.group(2)) * (1 if mm.group(1) == 'add' else -1))
else:
    da, dm = first(r'mov a[lx],\[es:di\+0x([0-9a-f]+)\]$', pa + 1, else_at)
    const = None
    if ca is not None and (da is None or ca < da):
        const = x86.num(cm.group(1))
        lines.append('difficulty const 0x%X' % const)
        start = code.after(ca)[0]
    elif da is not None:
        o = int(dm.group(1), 16)
        lines.append('difficulty 0x%X' % (o + 1 if game in ('darkness', 'treasures') else 0x4B00 + o // 2))
        start = code.after(da)[0]
    else:
        lines.append('difficulty none')
        start = None

    def walk(x):
        # the chain: cmp al|ax,c / jnz / add|inc|dec|sub [bp-2], walked for
        # the value x (a walk of its own: the chain compares ax as often as al)
        got = 0
        a = start
        while a is not None and a < else_at:
            t = code.text(a)
            nxt = code.after(a)[0]
            c = re.match(r'cmp a[lx],(0x[0-9a-f]+)$', t)
            if c:
                eq = x == x86.num(c.group(1))
                j = code.after(a)
                if j[2].startswith('jnz') and not eq:
                    nxt = tgt(a, j[2])
                elif j[2].startswith('jnz'):
                    nxt = code.after(a, 2)[0]
            else:
                mm = re.match(r'(add|sub|inc|dec) byte \[bp-0x2\](?:,(0x[0-9a-f]+))?$', t)
                if mm:
                    got += {'inc': 1, 'dec': -1}.get(mm.group(1)) or \
                        (x86.num(mm.group(2)) * (1 if mm.group(1) == 'add' else -1))
                elif t.startswith('jmp'):
                    break
            a = nxt
        return got

    for v in range(0, 7):
        adj[v] = walk(v if const is None else const)
    if const is not None:
        # what the switch would give had it tested the difficulty (the area
        # word the constant names: 0x4B00 + its value) - the Game Fixes'
        lines[-1] += ' (area word 0x%X)' % (0x4B00 + const)
        for v in range(0, 7):
            lines.append('fixadj %d %d' % (v, walk(v)))
for v in range(0, 7):
    lines.append('adj %d %d' % (v, adj[v]))
os.makedirs(os.path.dirname(out), exist_ok=True)
open(out, 'w').write('\n'.join(lines) + '\n')
print(out)
print('\n'.join(lines))
