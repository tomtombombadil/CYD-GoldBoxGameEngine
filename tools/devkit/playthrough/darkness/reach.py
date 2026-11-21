# reach.py [slot]: the squares reachable on foot from the party (MAPWALLS with MAPSECRET) and the event codes on them
import re,subprocess,os,sys
P='/tmp/claude-0/dplay'; S=sys.argv[1] if len(sys.argv)>1 else 'B'
subprocess.run([P+'/bin/p.sh','',f'SAVE={S}','MAPWALLS=1','MAPSECRET=1','MAPFLAGS=1'],env=dict(os.environ,TAILN='0'),capture_output=True)
L=open(P+'/run.log').read().splitlines()
i=[k for k,l in enumerate(L) if 'MAPWALLS map' in l][0]; rows=L[i+1:i+33]
fl=[k for k,l in enumerate(L) if 'MAPFLAGS map' in l][0]
F=[[int(v,16) for v in L[fl+1+y].split(':')[1].split()] for y in range(16)]
top=[rows[2*y][6:] for y in range(16)]; mid=[rows[2*y+1][4:] for y in range(16)]
oN=lambda x,y: top[y][2*x+1]!='-'; oW=lambda x,y: mid[y][2*x]!='|'
s=[(x,y) for y in range(16) for x in range(16) if mid[y][2*x+1]=='@'][0]
seen={s}; q=[s]
while q:
    x,y=q.pop(); nb=[]
    if y>0 and oN(x,y): nb.append((x,y-1))
    if y<15 and oN(x,y+1): nb.append((x,y+1))
    if x>0 and oW(x,y): nb.append((x-1,y))
    if x<15 and oW(x+1,y): nb.append((x+1,y))
    for n in nb:
        if n not in seen: seen.add(n); q.append(n)
print('from',s,'reach',len(seen))
codes={}
for x,y in seen:
    if F[y][x]: codes.setdefault(F[y][x],[]).append((x,y))
for c in sorted(codes): print('  code %02X: %s'%(c,sorted(codes[c])))
