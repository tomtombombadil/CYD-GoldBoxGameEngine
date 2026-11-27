import sys, json, random; sys.path.insert(0,'.')
from cluemap import render
from sides import oriented
from match import geo_blocks
from PIL import Image, ImageDraw
g,pdf,eng_want,kind_want,out=sys.argv[1:6]
R=json.load(open(f'./read_{g}.json')); blocks={b['name']:b for b in geo_blocks(f'./geo_{g}.txt')}
cands=[]
for name,m in R.items():
    bk=blocks[name]; plain=bk['plain']
    for k,v in m['sides'].items():
        x,y,d=map(int,k.split(',')); q=bk['sq'][y*16+x]; i=d//2; w,door,pas=q[i*3:i*3+3]
        eng='none' if not w else ('S?' if door==1 and plain>>w&1 else f'door{door}' if door else 'wall')
        if eng==eng_want and v==kind_want: cands.append((name,m,x,y,d))
random.seed(2); random.shuffle(cands); cands=cands[:24]
pages={}; ims=[]
for name,m,x,y,d in cands:
    p=m['page']
    if p not in pages: pages[p]=render(pdf,p,200)
    s,X,Y,C=m['reg']; im=pages[p]
    if d==0: cx,cy=X+(x+0.5)*C, Y+y*C
    elif d==4: cx,cy=X+(x+0.5)*C, Y+(y+1)*C
    elif d==2: cx,cy=X+(x+1)*C, Y+(y+0.5)*C
    else: cx,cy=X+x*C, Y+(y+0.5)*C
    h=int(0.8*C); crop=Image.fromarray(im[int(cy-h):int(cy+h),int(cx-h):int(cx+h)].astype('uint8')).resize((80,80))
    ims.append((f'{name.split(":")[1]} {x},{y}{"NESW"[d//2]}',crop))
o=Image.new('RGB',(8*84,((len(ims)+7)//8)*94),'white'); dr=ImageDraw.Draw(o)
for i,(t,c) in enumerate(ims): o.paste(c,((i%8)*84,(i//8)*94+12)); dr.text(((i%8)*84+2,(i//8)*94),t,fill='black')
o.save(out); print(len(cands))
