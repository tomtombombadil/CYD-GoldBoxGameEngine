"""The engine's sides against what the cluebook draws: counts per game, and the secret-door rule's hits and misses."""
import sys, json, collections
sys.path.insert(0,'.')
from match import geo_blocks
for g in sys.argv[1:]:
    try: R=json.load(open(f'./read_{g}.json'))
    except Exception: continue
    blocks={b['name']:b for b in geo_blocks(f'./geo_{g}.txt')}
    tab=collections.Counter(); hits=miss=false=0
    for name,m in R.items():
        bk=blocks[name]; plain=bk['plain']
        for k,v in m['sides'].items():
            x,y,d=map(int,k.split(',')); q=bk['sq'][y*16+x]; i=d//2
            w,door,pas=q[i*3:i*3+3]
            eng = 'none' if not w else ('S?' if door==1 and plain>>w&1 else f'door{door}' if door else 'wall')
            tab[(eng,v)]+=1
    print(f'== {g}: {len(R)} maps read')
    engs=sorted(set(e for e,_ in tab))
    kinds=['wall','door','secret','open','part','one-side']
    print('   engine \\ cluebook  '+'  '.join(f'{k:>8s}' for k in kinds))
    for e in engs: print(f'   {e:18s} '+'  '.join(f'{tab[(e,k)]:8d}' for k in kinds))
