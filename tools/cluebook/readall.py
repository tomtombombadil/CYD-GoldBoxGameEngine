"""Every found map of a game, registered closely and each side read; saved for the comparisons."""
import sys, json, collections
sys.path.insert(0,'.')
from cluemap import render, ink
from sides import oriented, features, kind
from refine import refine
from match import geo_blocks
g,pdf,thr=sys.argv[1],sys.argv[2],float(sys.argv[3])
blocks={b['name']:b for b in geo_blocks(f'./geo_{g}.txt')}
r=json.load(open(f'./res_{g}.json')); best={}
for sc,p,name,c,x,y in r:
    if name not in best or sc>best[name][0]: best[name]=(sc,p,c,x,y)
pages={}; out={}
for name,(sc,p,c,ox,oy) in sorted(best.items()):
    if sc<thr: continue
    if p not in pages: pages[p]=ink(render(pdf,p,200))
    S=pages[p]; s,X,Y,C=refine(S,blocks[name],ox*2,oy*2,c*2)
    sides={}
    H,W=S.shape
    for yy in range(16):
        for xx in range(16):
            for d in (0,2,4,6):
                # (a side off the drawn page or beyond the drawing: not read)
                pt=oriented(S,X,Y,C,xx,yy,d)
                if pt is None: continue
                sides[f'{xx},{yy},{d}']=kind(features(pt,C))
    out[name]={'page':p,'score':sc,'reg':[float(s),float(X),float(Y),float(C)],'sides':sides}
    print(name, p, round(sc,2), round(float(s),2), collections.Counter(sides.values()).most_common(6), flush=True)
json.dump(out,open(f'./read_{g}.json','w'))
