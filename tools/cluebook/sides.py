"""What a cluebook draws on a square's side: wall, door, secret, open (or other)."""
import numpy as np
from scipy import ndimage

def oriented(S,X0,Y0,C,xx,yy,d):
    if d==0: cx,cy,h=X0+(xx+0.5)*C, Y0+yy*C, True
    elif d==4: cx,cy,h=X0+(xx+0.5)*C, Y0+(yy+1)*C, True
    elif d==2: cx,cy,h=X0+(xx+1)*C, Y0+(yy+0.5)*C, False
    else: cx,cy,h=X0+xx*C, Y0+(yy+0.5)*C, False
    a=int(0.5*C); b=int(0.45*C); cx=int(round(cx)); cy=int(round(cy))
    p=S[cy-b:cy+b, cx-a:cx+a] if h else S[cy-a:cy+a, cx-b:cx+b].T
    return p if p.shape==(2*b,2*a) else None

def longest_run(row):
    best=cur=0
    for v in row:
        cur=cur+1 if v else 0; best=max(best,cur)
    return best

def features(p, C):
    H,W=p.shape
    e0,e1=int(0.06*W),int(0.22*W)          # the side's two ends (clear of a crossing wall's thickness... mostly)
    ends=np.concatenate([p[:,e0:e1],p[:,W-e1:W-e0]],1).mean(1)
    lo,hi=int(H/2-0.3*C),int(H/2+0.3*C)
    r0=lo+int(np.argmax(ends[lo:hi])); e=ends[r0]
    if e<0.5:
        return dict(line=0,full=0,mid=0,a=0,b=0,pa=0,pb=0,thick=0)
    t0=t1=r0
    while t0-1>=0 and ends[t0-1]>0.5*e and r0-t0<0.2*C: t0-=1
    while t1+1<H and ends[t1+1]>0.5*e and t1-r0<0.2*C: t1+=1
    band=p[t0:t1+1].any(0)
    full=band.mean(); mid=band[int(0.38*W):int(0.62*W)].mean()
    lab,n=ndimage.label(p)
    keep=set(np.unique(lab[t0:t1+1][p[t0:t1+1]>0]))-{0}
    q=np.isin(lab,list(keep))
    m0,m1=int(0.25*W),int(0.75*W)
    above=q[max(0,t0-int(0.35*C)):t0,m0:m1]; below=q[t1+1:t1+1+int(0.35*C),m0:m1]
    a=above.mean() if above.size else 0; b=below.mean() if below.size else 0
    pa=max([longest_run(r[m0-int(0.1*W):m1+int(0.1*W)]) for r in q[max(0,t0-int(0.35*C)):max(0,t0-2)]] or [0])/C
    pb=max([longest_run(r[m0-int(0.1*W):m1+int(0.1*W)]) for r in q[t1+3:t1+1+int(0.35*C)]] or [0])/C
    return dict(line=e,full=full,mid=mid,a=a,b=b,pa=pa,pb=pb,thick=(t1-t0+1)/C)

def kind(f):
    if f['line']<0.5: return 'open'
    a,b=f['a'],f['b']
    if f['full']<0.6 and f['mid']<0.5 and max(a,b)<0.05: return 'part'
    if f['mid']<0.5 and max(a,b)>=0.05: return 'door'          # the line broken by a box (or an arch)
    if f['mid']>=0.8 and min(a,b)>=0.08 and max(f['pa'],f['pb'])<0.5: return 'secret'   # a hook across a whole line
    if min(a,b)>=0.05 and max(f['pa'],f['pb'])>=0.5: return 'door'   # a box drawn over a whole line
    if max(a,b)<0.1 and f['full']>0.85: return 'wall'
    return 'one-side'
