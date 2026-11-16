#!/usr/bin/env python3
# ovgo.py <run.log> <x,y> [allowed hex flags, comma]: Gateway's overland - the way from where the party
# stands (the log's last "now x,y D") to x,y over squares whose MAPFLAGS value is allowed (default: the
# river A and the goal), as playsim tokens (F a step, L / R a turn, B about)
import sys,re
from collections import deque
log=open(sys.argv[1],errors='replace').read().split('\n')
gx,gy=map(int,sys.argv[2].split(','))
allow=set(int(v,16) for v in (sys.argv[3] if len(sys.argv)>3 else 'A').split(','))
g=[]
for i,l in enumerate(log):
    if 'MAPFLAGS map' in l:
        g=[[int(v,16) for v in r.split(':')[1].split()] for r in log[i+1:i+17]]
nw=[l for l in log if re.search(r'^\s+now \d+,',l)]
now=nw[-1] if nw else 'now '+sys.argv[4]
m=re.search(r'now (\d+),(\d+) (\w)',now); sx,sy,d=int(m[1]),int(m[2]),'NESW'.index(m[3])
D=[(0,-1),(1,0),(0,1),(-1,0)]
prev={(sx,sy):None}; q=deque([(sx,sy)])
while q:
    x,y=q.popleft()
    if (x,y)==(gx,gy): break
    for k,(dx,dy) in enumerate(D):
        n=(x+dx,y+dy)
        if not(0<=n[0]<16 and 0<=n[1]<16) or n in prev: continue
        if n!=(gx,gy) and g[n[1]][n[0]] not in allow: continue
        prev[n]=((x,y),k); q.append(n)
if (gx,gy) not in prev: print("no way"); sys.exit(1)
steps=[]; c=(gx,gy)
while prev[c]: steps.append(prev[c][1]); c=prev[c][0]
out=''
for k in reversed(steps):
    t=(k-d)%4; out+=['','R','B','L'][t]+'F'; d=k
print(out)
