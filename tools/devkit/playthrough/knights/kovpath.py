# kovpath.py <x,y> [slot]: tokens for the shortest walk on Death Knights' overland (ov17.txt: block 17's code
# grid, rows at 9965 + 38 * y, read by PEEK) from the party (PEEK 4BC3 / 4BC4) to x,y. Keys: F north, R north-east,
# > east, B south, < west, L north-west (no south-east / south-west key). Mountains (0x14) and water (0x19) refuse
# the step; other places (codes 1-0x13) are avoided on the way (their questions).
import subprocess, os, sys, collections
P = '/tmp/claude-0/knplay'; S = sys.argv[2] if len(sys.argv) > 2 else 'C'
tx, ty = map(int, sys.argv[1].split(','))
g = [list(map(int, l.split())) for l in open(P + '/ov17.txt')]
subprocess.run([P + '/bin/p.sh', '', f'SAVE={S}', 'PEEK=4BC3,4BC4'], env=dict(os.environ, TAILN='0'), capture_output=True)
sx, sy = [int(l.split()[3], 16) for l in open(P + '/run.log') if l.startswith('  PEEK ')][:2]
M = [('F', 0, -1), ('R', 1, -1), ('>', 1, 0), ('B', 0, 1), ('<', -1, 0), ('L', -1, -1)]
prev = {(sx, sy): None}; q = collections.deque([(sx, sy)])
while q:
    x, y = q.popleft()
    if (x, y) == (tx, ty): break
    for k, dx, dy in M:
        nx, ny = x + dx, y + dy
        if not (0 <= nx < 38 and 0 <= ny < 15) or (nx, ny) in prev: continue
        c = g[ny][nx]
        if c in (0x14, 0x19) or (0 < c < 0x14 and (nx, ny) != (tx, ty)): continue
        prev[(nx, ny)] = ((x, y), k); q.append((nx, ny))
if (tx, ty) not in prev: sys.exit('no way')
t = []; p = (tx, ty)
while prev[p]: p, k = prev[p]; t.append(k)
print('/M' + ''.join(reversed(t)))
