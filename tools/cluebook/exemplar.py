"""Each side of each found map named by the examples it looks most like (a few per book, picked by eye)."""
import sys, json, collections, numpy as np
sys.path.insert(0,'.')
from cluemap import render, ink
from match import geo_blocks

def patch(S,X0,Y0,C,xx,yy,d):
    """the side, lying across: N/S sides as drawn, E/W turned; along 1.1 C, across 0.8 C"""
    if d==0: cx,cy,h=X0+(xx+0.5)*C, Y0+yy*C, True
    elif d==4: cx,cy,h=X0+(xx+0.5)*C, Y0+(yy+1)*C, True
    elif d==2: cx,cy,h=X0+(xx+1)*C, Y0+(yy+0.5)*C, False
    else: cx,cy,h=X0+xx*C, Y0+(yy+0.5)*C, False
    a=int(0.55*C); b=int(0.4*C); cx=int(round(cx)); cy=int(round(cy))
    p=S[cy-b:cy+b, cx-a:cx+a] if h else S[cy-a:cy+a, cx-b:cx+b].T
    return p.astype(float) if p.shape==(2*b,2*a) else None

def sim(p,q,sh=4):
    """the best overlap of q on p within sh pixels each way (Dice)"""
    best=0
    H,W=p.shape
    for dy in range(-sh,sh+1):
        for dx in range(-sh,sh+1):
            a=p[max(0,dy):H+min(0,dy), max(0,dx):W+min(0,dx)]
            b=q[max(0,-dy):H+min(0,-dy), max(0,-dx):W+min(0,-dx)]
            s=a.sum()+b.sum()
            if s: best=max(best, 2*(a*b).sum()/s)
    return best

def run(g,pdf,thr,examples):
    blocks={b['name']:b for b in geo_blocks(f'./geo_{g}.txt')}
    r=json.load(open(f'./res_{g}.json')); best={}
    for sc,p,name,c,x,y in r:
        if name not in best or sc>best[name][0]: best[name]=(sc,p,c,x,y)
    pages={}
    def S_of(p):
        if p not in pages: pages[p]=ink(render(pdf,p,200))
        return pages[p]
    ex=[]
    for lab,name,xx,yy,d in examples:
        sc,p,c,ox,oy=best[name]; q=patch(S_of(p),ox*2,oy*2,c*2,xx,yy,'NESW'.index(d)*2)
        ex.append((lab,q))
    out=[]
    for name,(sc,p,c,ox,oy) in sorted(best.items()):
        if sc<thr: continue
        S=S_of(p); bk=blocks[name]; plain=bk['plain']
        for yy in range(16):
            for xx in range(16):
                q=bk['sq'][yy*16+xx]
                for k,d in enumerate([0,2,4,6]):
                    w,door,pas=q[k*3:k*3+3]
                    pt=patch(S,ox*2,oy*2,c*2,xx,yy,d)
                    if pt is None: continue
                    if pt.sum()<3: lab,s='open',1.0
                    else:
                        sc2=sorted(((sim(pt,e),l) for l,e in ex if e is not None and e.shape==pt.shape),reverse=True)
                        s,lab=sc2[0] if sc2 else (0,'?')
                        if s<0.55: lab='other'
                    out.append((name,xx,yy,'NESW'[k],lab,round(float(s),2),w,door,pas,bool(w and plain>>w&1)))
    return out
