import sys, json; sys.path.insert(0,'.')
from cluemap import render, ink
from sides import oriented
from refine import refine
from match import geo_blocks
from PIL import Image, ImageDraw
g,pdf=sys.argv[1],sys.argv[2]; cases=json.loads(sys.argv[3]); out=sys.argv[4]
blocks={b['name']:b for b in geo_blocks(f'./geo_{g}.txt')}
r=json.load(open(f'./res_{g}.json')); best={}
for sc,p,name,c,x,y in r:
    if name not in best or sc>best[name][0]: best[name]=(sc,p,c,x,y)
pages={}; ims=[]; reg={}
for lab,name,x,y,d in cases:
    sc,p,c,ox,oy=best[name]
    if p not in pages: pages[p]=ink(render(pdf,p,200))
    if name not in reg: reg[name]=refine(pages[p],blocks[name],ox*2,oy*2,c*2); print(name, [round(v,2) for v in reg[name]])
    s,X,Y,C=reg[name]
    pt=oriented(pages[p],X,Y,C,x,y,'NESW'.index(d)*2)
    ims.append((lab,Image.fromarray(((1-pt)*255).astype('uint8')).resize((pt.shape[1]*3,pt.shape[0]*3),Image.NEAREST)))
W=max(i.width for _,i in ims); H=max(i.height for _,i in ims)
o=Image.new('L',(len(ims)*(W+6),H+14),200); dr=ImageDraw.Draw(o)
for k,(l,i) in enumerate(ims): o.paste(i,(k*(W+6),14)); dr.text((k*(W+6)+2,1),l,fill=0)
o.save(out)
