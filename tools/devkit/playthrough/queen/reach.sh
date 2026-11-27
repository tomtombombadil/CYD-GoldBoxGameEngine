#!/bin/bash
# reach.sh <slot> [x,y]: the map's reachable squares, one a code, nearest-first from x,y (the party's square)
P=/tmp/claude-0/qplay; cd $P; TAILN=0 bin/p.sh "" SAVE=$1 MAPREACH=1 >/dev/null
python3 - "$2" <<'PY'
import re,sys
L=open('/tmp/claude-0/qplay/run.log').read()
m=re.search(r'MAPREACH \d+ squares: (.*?) MAPREACH',L) or re.search(r'MAPREACH \d+ squares: (.*)',L)
pts={}
for t in m.group(1).split():
    if '=' not in t: continue
    xy,c=t.split('=')
    if not c: continue
    pts.setdefault(c,tuple(map(int,xy.split(','))))
cur=tuple(map(int,sys.argv[1].split(','))) if sys.argv[1] else (0,0)
pl=list(pts.items()); out=[]
while pl:
    pl.sort(key=lambda q: abs(q[1][0]-cur[0])+abs(q[1][1]-cur[1])); c,q=pl.pop(0); out.append('%d,%d'%q); cur=q
    print('%d,%d=%s'%(q[0],q[1],c),end=' ',file=sys.stderr)
print(' '.join(out))
PY
