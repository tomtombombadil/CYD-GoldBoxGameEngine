#!/usr/bin/env python3
# plan.py <probe.json> <x,y,l,facing> <goal x,y,l> [last-dir]: tokens for the shortest walk through a probe's graph
# (F a step, L / R / B turns); with last-dir, the walk ends with the step that way off the goal (into an event)
import json, sys
from collections import deque
E = json.load(open(sys.argv[1])); sx, sy, sl, sf = map(int, sys.argv[2].split(',')); g = tuple(map(int, sys.argv[3].split(',')))
turn = {0: '', 2: 'R', 4: 'B', 6: 'L'}
start = (sx, sy, sl, sf); prev = {start: None}; q = deque([start]); end = None
while q:
    s = q.popleft(); x, y, l, f = s
    if (x, y, l) == g: end = s; break
    for d in (0, 2, 4, 6):
        r = E.get(f'{x},{y},{l},{d}')
        if not r: continue
        n = (r[0], r[1], r[2], d)
        if (r[0], r[1]) == (x, y) and r[2] == l: continue
        if n[:3] in [p[:3] for p in prev]: continue
        prev[n] = (s, turn[(d - f) % 8] + 'F'); q.append(n)
if not end: print('NO WAY'); sys.exit(1)
toks = []; s = end
while prev[s]: s, t = prev[s]; toks.append(t)
t = ''.join(reversed(toks))
if len(sys.argv) > 4: t += turn[(int(sys.argv[4]) - end[3]) % 8] + 'F'
print(t)
