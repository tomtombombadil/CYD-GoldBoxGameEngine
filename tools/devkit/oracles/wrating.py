#!/usr/bin/env python3
# The computer's weapon rating in each DAX game's GAME.OVR (coab's
# CalcItemPowerRating; rules round 21, 2026-11-12 - Tom: Quick's missiles).
# Finds the routine by its paralysing-gaze test (cmp byte [es:di+0x3d],0x53),
# walks back to its prologue and prints what it tests: the special item type
# against undead (cmp [es:di+0x2e],T then "mov byte [bp-2],V"), the other
# zeroing tests, and the byte arithmetic. Facts only, printed.
# usage: wrating.py [short ...]   (gb.folder finds the games in scratch)
import re, subprocess, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gb

GAMES = ['poolrad', 'curse', 'secret', 'darkness', 'gateway', 'treasures', 'champions', 'knights']


def dis(data, start, n):
    out = subprocess.run(['ndisasm', '-b16', '-o', str(start), '-e', str(start), '-'], input=data[:start + n],
                         capture_output=True).stdout.decode().splitlines()
    return [(int(l[:8], 16), l[28:].strip()) for l in out if len(l) > 28]


for g in (sys.argv[1:] or GAMES):
    ovr = open(os.path.join(gb.folder(g), 'GAME.OVR'), 'rb').read()
    at = ovr.find(bytes.fromhex('26807d3d53'))
    if at < 0:
        print(g, ': no gaze test'); continue
    st = max(ovr.rfind(b'\x55\x89\xe5', 0, at), ovr.rfind(b'\x55\x8b\xec', 0, at))
    lines = []
    for a, ins in dis(ovr, st, 0x200):
        lines.append((a, ins))
        if ins.startswith('retf'):
            break
    types = [ins for a, ins in lines if re.search(r'cmp byte \[es:di\+0x2e\],', ins)]
    sets = [ins for a, ins in lines if re.search(r'mov byte \[bp-0x[0-9a-f]+\],0x[0-9a-f]+', ins)]
    tests = [ins for a, ins in lines if re.search(r'cmp byte \[es:di\+0x3[de]\]', ins)]
    print('%-9s routine 0x%x-0x%x (%d instructions)' % (g, st, lines[-1][0], len(lines)))
    print('   item type tests: %s' % '; '.join(types))
    print('   constants set:   %s' % '; '.join(sets))
    print('   effect tests:    %s' % '; '.join(tests))
    for k, (a, ins) in enumerate(lines):
        if ins == 'cmp ax,0x3':
            print('   hands rule:      used + the type\'s > or >= 3: %s (jng: > 3 zeroes; jl: >= 3)' % lines[k + 1][1].split()[0])
        if re.search(r'cmp byte \[es:di\+0x2e\],', ins):
            print('   against undead:  %s' % ' / '.join(i for _, i in lines[k + 1:k + 12] if 'cmp' in i or 'mov byte' in i))
    print('   record fields:   %s' % ' '.join(sorted(set(re.findall(r'es:di\+(0x1[0-9a-f]{2}|0xe9)\]', ' '.join(i for _, i in lines))))))


# The chooser (the routine calling the rating): its decision's marks - the
# missile rating against half the melee one (shr ax,1), the thrown-melee test
# (and al,0x14), the sling (flags 0x0a), the start values (missile 1, melee
# the natural attack)
def chooser(g):
    import struct
    ovr = open(os.path.join(gb.folder(g), 'GAME.OVR'), 'rb').read()
    at = ovr.find(bytes.fromhex('26807d3d53'))
    rt = max(ovr.rfind(b'\x55\x89\xe5', 0, at), ovr.rfind(b'\x55\x8b\xec', 0, at))
    calls = [k for k in range(max(0, rt - 0x3000), rt + 0x3000) if ovr[k] == 0xE8 and
             (k + 3 + struct.unpack_from('<h', ovr, k + 1)[0]) & 0xFFFF == rt & 0xFFFF]
    if not calls:
        return None
    st = max(ovr.rfind(b'\x55\x89\xe5', 0, calls[0]), ovr.rfind(b'\x55\x8b\xec', 0, calls[0]))
    lines = dis(ovr, st, 0x700)
    end = next((k for k, (a, ins) in enumerate(lines) if ins.startswith('retf') or ins.startswith('db 0xca')), len(lines))
    txt = [i for _, i in lines[:end]]
    marks = {'half': sum('shr ax,1' in i for i in txt), 'thrown 0x14': sum('0x14' in i for i in txt),
             'sling 0xa': sum(re.search(r',0xa$', i) is not None for i in txt), 'start 1': sum(re.search(r'mov byte \[bp-0x1[59]\],0x1$', i) is not None for i in txt)}
    return st, end, marks


if len(sys.argv) <= 1 or sys.argv[1] != '--no-chooser':
    for g in GAMES:
        c = chooser(g)
        print('%-9s chooser %s' % (g, 'not found' if not c else '0x%x, %d instructions, %s' % c))
