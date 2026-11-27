import numpy as np
def wall_list(bk):
    out=[]
    for y in range(16):
        for x in range(16):
            q=bk['sq'][y*16+x]
            if q[0]: out.append((x,y,True))
            if q[6]: out.append((x,y+1,True))
            if q[9]: out.append((x,y,False))
            if q[3]: out.append((x+1,y,False))
    return list(set(out))
def score(S,walls,ox,oy,c,ts):
    H,W=S.shape; hit=0; n=0
    for x,y,h in walls:
        for t in ts:
            px,py=(ox+(x+t)*c, oy+y*c) if h else (ox+x*c, oy+(y+t)*c)
            px=int(px); py=int(py)
            if 0<=px<W and 0<=py<H: hit+=S[py,px]; n+=1
    return hit/max(n,1)
def refine(S,bk,ox,oy,c,span=8,cspan=0.06):
    walls=wall_list(bk); ts=[0.25,0.5,0.75]
    best=(score(S,walls,ox,oy,c,ts),ox,oy,c)
    for cc in np.arange(c*(1-cspan),c*(1+cspan),c*0.005):
        for dy in range(-span,span+1,2):
            for dx in range(-span,span+1,2):
                s=score(S,walls,ox+dx,oy+dy,cc,ts)
                if s>best[0]: best=(s,ox+dx,oy+dy,cc)
    s,ox,oy,c=best
    for dy in (-1,0,1):
        for dx in (-1,0,1):
            s2=score(S,walls,ox+dx,oy+dy,c,ts)
            if s2>s: s,bx,by=s2,ox+dx,oy+dy
    return best
