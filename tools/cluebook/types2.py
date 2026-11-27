import sys, json, collections
sys.path.insert(0,'.')
from match import geo_blocks
g=sys.argv[1]; R=json.load(open(f'./read_{g}.json')); blocks={b['name']:b for b in geo_blocks(f'./geo_{g}.txt')}
for name,m in sorted(R.items()):
    if m['reg'][0]<0.85: continue
    bk=blocks[name]; t=collections.defaultdict(collections.Counter)
    for k,v in m['sides'].items():
        x,y,d=map(int,k.split(',')); q=bk['sq'][y*16+x]; i=d//2; w,door=q[i*3],q[i*3+1]
        if w and door: t[w][v]+=1
        if w and not door: t[w]['solid']+=1
    print(name, '  '.join(f"t{w}:{dict(c)}" for w,c in sorted(t.items())))
