#!/bin/bash
# regtour.sh SLOT RAW : tour the event squares reachable from the party's square (doors by the low bit), not
# yet visited (visited.txt), nearest first (PADS=hex codes,... of stairs / teleporters: last); appends to visited.txt. Env as tour.sh.
S=$1; RAW=$2
TAILN=0 bin/p.sh '' SAVE=$S >/dev/null
pos=$(grep "loaded SAV" run.log | sed 's/.*area [0-9]*, \([0-9]*,[0-9]*\),.*/\1/')
touch visited.txt
python3 - "$RAW" "$pos" > regt.txt <<'P'
import sys,subprocess,re
raw,pos=sys.argv[1],sys.argv[2]
d=open(raw,'rb').read(); w,h=d[0],d[1]; P2=d[8+2*w*h:8+3*w*h]
out=subprocess.run(['python3','/tmp/claude-0/qgeo/region.py',raw,pos],capture_output=True,text=True).stdout.split('\n')[0]
reg=set(tuple(map(int,m)) for m in re.findall(r'\((\d+), (\d+)\)',out))
vis=set(tuple(map(int,l.split(','))) for l in open('visited.txt').read().split())
cur=tuple(map(int,pos.split(',')))
left=[p for p in reg if P2[p[0]+p[1]*w]&0x7f and p not in vis and p!=cur]
res=[]
while left:
  n=min(left,key=lambda p:abs(p[0]-cur[0])+abs(p[1]-cur[1])); res.append(n); left.remove(n); cur=n
import os
pads=set(int(c,16) for c in os.environ.get('PADS','').split(',') if c)
res=[p for p in res if P2[p[0]+p[1]*w]&0x7f not in pads]+[p for p in res if P2[p[0]+p[1]*w]&0x7f in pads]
print(' '.join('%d,%d'%p for p in res))
P
echo "from $pos: $(wc -w < regt.txt) squares"
[ -s regt.txt ] || exit 1
# (visited.txt is the caller's: squares really reached)
bash bin/tour.sh $S $(cat regt.txt) > /dev/null 2>&1
