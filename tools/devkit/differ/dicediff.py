#!/usr/bin/env python3
# dicediff.py <run name> - the original's draws (runs/<name>/rand.txt: a
# scenario with "randlog") against the engine's (its eng.log with DICELOG=1 -
# "env": {"DICELOG": "1"}): both lists of faces side by side up to the first
# that differs, with the keys / tokens around it. The engine's dice are the
# original's generator (TPSEED), so the same faces in the same order mean
# the same rolls.
import os, re, sys

ROOT = '/tmp/claude-0/differ/runs'
name = sys.argv[1]
w = os.path.join(ROOT, name)
dos, eng = [], []
mark = 'start'
for l in open(w + '/rand.txt'):
    if l.startswith('key') or l.startswith('shot'):
        mark = l.strip()
    elif l.startswith('rand'):
        p = l.split()
        dos.append((int(p[6]), mark, ' '.join(x.split(':')[0] for x in p[7:])))
mark = 'start'
for l in open(w + '/eng.log'):
    m = re.match(r'  \(die (\d+)', l)
    if m:
        eng.append((int(m.group(1)), mark))
    elif l.startswith('  TP after'):
        mark = l.strip()[:60]
dos = dos[2:]          # (the two draws before a load's first screen: TPSKIP)
n = min(len(dos), len(eng))
k = next((i for i in range(n) if dos[i][0] != eng[i][0]), n)
print('original %d draws, engine %d; the same up to %d' % (len(dos), len(eng), k))
for i in range(max(0, k - 6), min(max(len(dos), len(eng)), k + 8)):
    a = dos[i] if i < len(dos) else ('-', '', '')
    b = eng[i] if i < len(eng) else ('-', '')
    print('%5d %s d%-4s %-14s | d%-4s %s   %s' % (i, '>' if i == k else ' ', a[0], a[1][:14], b[0], b[1][:30], a[2]))
