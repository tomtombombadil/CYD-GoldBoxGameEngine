import sys, json, collections
sys.path.insert(0,'.')
from match import geo_blocks
g=sys.argv[1]; minscore=float(sys.argv[2]) if len(sys.argv)>2 else 0.8
R=json.load(open(f'./read_{g}.json')); blocks={b['name']:b for b in geo_blocks(f'./geo_{g}.txt')}
D={0:(0,-1,4),2:(1,0,6),4:(0,1,0),6:(-1,0,2)}
tab=collections.Counter()
for name,m in sorted(R.items()):
    if m['reg'][0]<minscore: continue
    bk=blocks[name]; plain=bk['plain']
    for k,v in m['sides'].items():
        x,y,d=map(int,k.split(','))
        if v not in ('secret','door'): continue
        dx,dy,od=D[d]; nx,ny=x+dx,y+dy
        if not (0<=nx<16 and 0<=ny<16): continue
        q=bk['sq'][y*16+x]; o=bk['sq'][ny*16+nx]
        w,door=q[(d//2)*3],q[(d//2)*3+1]; ow,odr=o[(od//2)*3],o[(od//2)*3+1]
        key=(v, 'P' if w and plain>>w&1 else '-', door, 'P' if ow and plain>>ow&1 else '-', odr, 'same' if w==ow else 'diff')
        tab[key]+=1
print(g,'maps with registration >=',minscore, sum(1 for m in R.values() if m['reg'][0]>=minscore))
print('  cluebook  this side (plain? door)  other side (plain? door)  wall type')
for k,n in sorted(tab.items(), key=lambda a:(a[0][0],-a[1])): print('  %-7s   %s %d                    %s %d                   %s   %d'%(k[0],k[1],k[2],k[3],k[4],k[5],n))
