# mapbfs.py <x,y from> <x,y to>: a path on run.log's MAPWALLS map (' ' open sides only; S/L shown as passable with -s)
import sys,re
L=open('/tmp/claude-0/qplay/run.log').read().split('\n')
i=[k for k,l in enumerate(L) if 'MAPWALLS map' in l][0]; w,h=map(int,re.search(r'(\d+)x(\d+)',L[i]).groups())
tops=[L[i+1+2*y] for y in range(h)]+[L[i+1+2*h]]; mids=[L[i+2+2*y] for y in range(h)]
ok=set(' ')|({'S','L'} if '-s' in sys.argv else set())
def side_ok(c): return c in ok
def nb(x,y):
    if y>0 and side_ok(tops[y][5+2*x]): yield x,y-1
    if y<h-1 and side_ok(tops[y+1][5+2*x]): yield x,y+1
    if x<w-1 and side_ok(mids[y][6+2*x]): yield x+1,y
    if x>0 and side_ok(mids[y][4+2*x]): yield x-1,y
a=tuple(map(int,sys.argv[1].split(','))); b=tuple(map(int,sys.argv[2].split(',')))
prev={a:None}; q=[a]
for p in q:
    for n in nb(*p):
        if n not in prev: prev[n]=p; q.append(n)
if b not in prev: print('no way; reachable',len(prev)); sys.exit()
p=b; path=[]
while p: path.append(p); p=prev[p]
print(' '.join('%d,%d'%p for p in path[::-1]))
