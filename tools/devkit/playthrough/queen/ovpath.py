# ovpath.py X,Y: the Dark Queen's overland - a shortest walk from the party's square (run.log's PEEK 25/26 or
# argv[2]) to X,Y over the CODEDUMP=A97D,1140 grid in run.log (rows of 38; high nibble D blocked; E / F places
# avoided except the goal). Tokens: F north, B south, < west, > east, L north-west, R north-east.
import re,sys
from collections import deque
L=open('/tmp/claude-0/qplay/run.log').read().splitlines()
b=[]
for l in L:
    m=re.match(r'\s+([0-9A-F]{4}):((?: [0-9A-F]{2})+)',l)
    if m and int(m.group(1),16)>=0xA97D: b+=[int(x,16) for x in m.group(2).split()]
b=b[:1140]
gx,gy=map(int,sys.argv[1].split(','))
sx,sy=map(int,sys.argv[2].split(','))
mv={'F':(0,-1),'B':(0,1),'<':(-1,0),'>':(1,0),'L':(-1,-1),'R':(1,-1)}
def ok(x,y):
    if not(0<=x<38 and 0<=y<30): return False
    v=b[y*38+x]>>4
    if v==0xD: return False
    if v in (0xE,0xF) and (x,y)!=(gx,gy): return False
    if (b[y*38+x]&0x0F)==0x0F and x==0: return False
    return True
prev={(sx,sy):None}; q=deque([(sx,sy)])
while q:
    c=q.popleft()
    if c==(gx,gy): break
    for k,(dx,dy) in mv.items():
        n=(c[0]+dx,c[1]+dy)
        if n not in prev and ok(*n): prev[n]=(c,k); q.append(n)
if (gx,gy) not in prev: print('no way'); sys.exit(1)
p=[]; c=(gx,gy)
while prev[c]: c,k=prev[c]; p.append(k)
print(''.join(reversed(p)))
