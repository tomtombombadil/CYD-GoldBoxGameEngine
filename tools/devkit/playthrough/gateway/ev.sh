#!/bin/bash
# ev.sh <slot> [grep words]: the slot's script - each ON-ENTRY event's number, its squares (MAPFLAGS) and the first text it shows
P=/tmp/claude-0/gplay
TAILN=0 $P/bin/p.sh "|B" SAVE=$1 DISASM=8000,FFFF MAPFLAGS=1 > /dev/null
cp $P/run.log $P/disev.txt
python3 - "$P/disev.txt" <<'PY'
import re,sys
L=open(sys.argv[1],errors='replace').read().split('\n')
g=[]
for i,l in enumerate(L):
    if 'MAPFLAGS map' in l: g=[[int(v,16) for v in r.split(':')[1].split()] for r in L[i+1:i+17]]
ops=[(int(l[:4],16),l) for l in L if re.match(r'^[0-9A-F]{4} op ',l)]
tabs=[]
base=0
for a,l in ops:
    m=re.search(r'op 05: 0:([0-9A-F]{4}) 1:4C02',l)
    if m: base=int(m[1],16)
    if ' op 25:' in l:
        t=[int(x,16) for x in re.findall(r'1:([0-9A-F]{4})',l)[1:]]
        tabs.append((base if '7F79' in l.split('op 25:')[1][:8] else 0,t))
for b,t in tabs:
    for i,ad in enumerate(t):
        ev=b+i
        sq=[(x,y) for y in range(16) for x in range(16) if g and g[y][x]==ev]
        txt=''
        for a,l in ops:
            if a>=ad and a<ad+0x120 and '"' in l and (' op 12:' in l or ' op 11:' in l):
                txt=l.split('"',1)[1][:90]; break
        if sq: print('%02X %04X %s %s'%(ev,ad,' '.join('%d,%d'%s for s in sq[:6]),txt))
PY
