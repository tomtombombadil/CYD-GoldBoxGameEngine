# redpath.py <slot> <x,y,facing 0-3> <goal x,y>: Vingaard's red-door trail - the shortest walk (tokens F / L / R /
# B-turns) from the square to the goal that never crosses a door whose type the trail table (9D6B) marks 2 (that
# ends the trail: "whispers fade away"); open sides (MAPWALLS ' ') and red doors (table 1) are crossed. Prints tokens.
import re, subprocess, os, sys, collections
P = '/tmp/claude-0/knplay'; S = sys.argv[1]
sx, sy, sf = map(int, sys.argv[2].split(',')); gx, gy = map(int, sys.argv[3].split(','))
A = ','.join('%X' % (0x9D6B + i) for i in range(16))
subprocess.run([P + '/bin/p.sh', '', f'SAVE={S}', f'PEEK={A}', 'MAPSIDES=1', 'MAPWALLS=1', 'MAPLOCKED=1', 'MAPSECRET=1'],
               env=dict(os.environ, TAILN='0'), capture_output=True)
L = open(P + '/run.log').read().splitlines()
tab = [int(l.split()[3], 16) & 0xFF for l in L if l.startswith('  PEEK ')]
sides = {}
for l in L:
    m = re.match(r'^\s+(\d+):\s(.*)$', l.rstrip())
    if m and '/' in m.group(2): sides[int(m.group(1))] = m.group(2).split()
i = next(k for k, l in enumerate(L) if 'MAPWALLS' in l)
top, mid = [], []
for y in range(16):
    top.append(L[i + 1 + 2 * y].strip('\n')); mid.append(L[i + 2 + 2 * y])
def tch(y, x):   # the N side of x,y: the char after '+' at column 6 + 2x
    return top[y][6 + 2 * x + 1] if len(top[y]) > 6 + 2 * x + 1 else ' '
def mch(y, x):   # the W side of x,y
    return mid[y][4 + 2 * x] if len(mid[y]) > 4 + 2 * x else ' '
def open_side(x, y, k):
    if k == 0: c = tch(y, x)
    elif k == 3: c = mch(y, x)
    elif k == 2: c = tch((y + 1) % 16, x)
    else: c = mch(y, (x + 1) % 16)
    return c not in '|-'
def ttype(x, y, k): return int(sides[y][x].split('/')[0][2 * k], 16)
D = [(0, -1), (1, 0), (0, 1), (-1, 0)]
prev = {(sx, sy, sf): None}; q = collections.deque([(sx, sy, sf)]); end = None
while q:
    x, y, f = q.popleft()
    if (x, y) == (gx, gy): end = (x, y, f); break
    for k, (dx, dy) in enumerate(D):
        t = ttype(x, y, k)
        if not open_side(x, y, k) or tab[t] == 2: continue
        st = ((x + dx) % 16, (y + dy) % 16, k)
        if st in prev: continue
        prev[st] = (x, y, f); q.append(st)
if not end: sys.exit('no way')
path = []; s = end
while prev[s]: path.append(s); s = prev[s]
tok = ''; f = sf
for x, y, k in reversed(path):
    d = (k - f) % 4
    tok += {0: '', 1: 'R', 2: 'RR', 3: 'L'}[d] + 'F'; f = k
print(tok)
