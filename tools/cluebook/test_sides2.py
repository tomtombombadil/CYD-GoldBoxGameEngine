import sys, json; sys.path.insert(0,'.')
from cluemap import render, ink
from sides import oriented, features, kind
from refine import refine
from match import geo_blocks
g,pdf=sys.argv[1],sys.argv[2]; cases=json.loads(sys.argv[3])
blocks={b['name']:b for b in geo_blocks(f'./geo_{g}.txt')}
r=json.load(open(f'./res_{g}.json')); best={}
for sc,p,name,c,x,y in r:
    if name not in best or sc>best[name][0]: best[name]=(sc,p,c,x,y)
pages={}; reg={}
for lab,name,x,y,d in cases:
    sc,p,c,ox,oy=best[name]
    if p not in pages: pages[p]=ink(render(pdf,p,200))
    if name not in reg: reg[name]=refine(pages[p],blocks[name],ox*2,oy*2,c*2)
    s,X,Y,C=reg[name]
    f=features(oriented(pages[p],X,Y,C,x,y,'NESW'.index(d)*2),C)
    print(f"{lab:7s} -> {kind(f):8s}", {k:round(float(v),2) for k,v in f.items()})
