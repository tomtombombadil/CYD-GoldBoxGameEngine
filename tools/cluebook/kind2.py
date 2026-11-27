import numpy as np
def measure(S,ox,oy,c,x,y,d):
    horiz = d in (0,4)
    if d==0: lx,ly=ox+x*c,oy+y*c
    elif d==4: lx,ly=ox+x*c,oy+(y+1)*c
    elif d==6: lx,ly=ox+x*c,oy+y*c
    else: lx,ly=ox+(x+1)*c,oy+y*c
    H,W=S.shape
    def at(t,o):
        px,py=(lx+t*c, ly+o) if horiz else (lx+o, ly+t*c)
        px=int(round(px)); py=int(round(py))
        return 0 if (py<0 or px<0 or py>=H or px>=W) else S[py,px]
    ts=np.linspace(0.12,0.88,17)
    # the drawn line: the offset (within 0.15 c) with the most ink along the side
    best=(-1,0)
    for o in range(-int(0.15*c),int(0.15*c)+1):
        s=sum(at(t,o) for t in ts)
        if s>best[0]: best=(s,o)
    o0=best[1]
    # its thickness: ink rows either side of o0 that carry most of the line
    th=0
    while th<int(0.12*c) and sum(at(t,o0+th+1) for t in ts)>0.6*len(ts): th+=1
    tl=0
    while tl<int(0.12*c) and sum(at(t,o0-tl-1) for t in ts)>0.6*len(ts): tl+=1
    def cov(t0,t1,n):
        return sum(max(at(t,o) for o in range(o0-tl,o0+th+1)) for t in np.linspace(t0,t1,n))/n
    on=cov(0.12,0.88,17); mid=cov(0.42,0.58,5)
    a=b=0;m=0
    for t in np.linspace(0.3,0.7,9):
        for k in range(2,int(0.3*c)):
            a+=at(t,o0-tl-k); b+=at(t,o0+th+k); m+=1
    return on,mid,a/m,b/m
def kind(on,mid,a,b):
    bes=max(a,b)
    if bes<0.1:
        if on>0.8: return 'wall'
        if on<0.15: return 'open'
        return 'mark'
    if mid<0.5 and min(a,b)>0.06: return 'door'
    if mid>=0.5 and on>0.6 and min(a,b)>0.15: return 'secret'
    return 'mark'
