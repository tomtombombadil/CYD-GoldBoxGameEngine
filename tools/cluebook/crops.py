import sys, json, numpy as np
sys.path.insert(0,'.')
from cluemap import render
from match import geo_blocks
from PIL import Image, ImageDraw
g=sys.argv[1]; pdf=sys.argv[2]; want=sys.argv[3]  # 'secret' | 'door1' ...
blocks={b['name']:b for b in geo_blocks(f'./geo_{g}.txt')}
r=json.load(open(f'./res_{g}.json')); best={}
for sc,p,name,c,x,y in r:
    if name not in best or sc>best[name][0]: best[name]=(sc,p,c,x,y)
crops=[]; pages={}
for name,(sc,p,c,ox,oy) in best.items():
    if sc<0.7: continue
    bk=blocks[name]; plain=bk['plain']
    if p not in pages: pages[p]=Image.fromarray(render(pdf,p,200).astype('uint8'))
    k2=2.0
    for yy in range(16):
        for xx in range(16):
            q=bk['sq'][yy*16+xx]
            for k,d in enumerate([0,2,4,6]):
                w,door,pas=q[k*3:k*3+3]
                sec = door==1 and w and (plain>>w&1)
                if (want=='secret' and sec) or (want=='door2' and door==2) or (want=='door1' and door==1 and not sec):
                    cx=(ox+(xx+0.5)*c)*k2; cy=(oy+(yy+0.5)*c)*k2
                    if d==0: cy-=c*k2/2
                    if d==4: cy+=c*k2/2
                    if d==2: cx+=c*k2/2
                    if d==6: cx-=c*k2/2
                    h=c*k2*0.9
                    im=pages[p].crop((int(cx-h),int(cy-h),int(cx+h),int(cy+h))).resize((90,90))
                    crops.append((f'{name.split(":")[1]} {xx},{yy}{"NESW"[k]}',im))
crops=crops[:40]
cols=8; rows=(len(crops)+cols-1)//cols
o=Image.new('RGB',(cols*94,rows*104),'white'); dr=ImageDraw.Draw(o)
for i,(t,im) in enumerate(crops):
    x=(i%cols)*94; y=(i//cols)*104; o.paste(im,(x,y+12)); dr.text((x+2,y),t,fill='black')
o.save(sys.argv[4]); print(len(crops))
