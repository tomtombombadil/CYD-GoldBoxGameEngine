"""Reads a cluebook page: the grids drawn on it, and each grid's square sides (wall / open)."""
import numpy as np, subprocess, os, sys
from PIL import Image

def render(pdf, page, dpi=150):
    out=f'./r_{os.getpid()}'
    subprocess.run(['pdftoppm','-r',str(dpi),'-f',str(page),'-l',str(page),'-png',pdf,out],capture_output=True)
    import glob
    f=sorted(glob.glob(out+'*.png'))[-1]; im=np.asarray(Image.open(f).convert('RGB')).astype(int); os.remove(f)
    return im

def ink(im):
    r,g,b=im[...,0],im[...,1],im[...,2]
    mx=im.max(2); mn=im.min(2); gray=(r+g+b)/3
    paper=np.median(gray)
    return (gray < paper-70) | ((mx-mn > 70) & (gray < paper-25))

def runs_mask(m, L, axis):
    """pixels in straight runs of at least L along axis (1: horizontal, 0: vertical)"""
    a = m if axis==1 else m.T
    out=np.zeros_like(a)
    for y in range(a.shape[0]):
        row=a[y]; d=np.diff(np.concatenate(([0],row.astype(int),[0])))
        st=np.where(d==1)[0]; en=np.where(d==-1)[0]
        for s,e in zip(st,en):
            if e-s>=L: out[y,s:e]=1
    return out if axis==1 else out.T

def period(profile, lo, hi):
    p=profile-profile.mean(); best=None
    ac=[(np.dot(p[:-k],p[k:])/(len(p)-k), k) for k in range(lo,hi)]
    # the first strong peak (not a multiple)
    vals=np.array([a for a,_ in ac]); ks=[k for _,k in ac]
    i=int(np.argmax(vals)); return ks[i]

def comb(profile, c, length):
    """best offset of a comb of spacing c (sub-pixel c) and its strength per tooth"""
    best=(-1,0)
    for o10 in range(0,int(c*10)):
        o=o10/10; teeth=np.arange(o,len(profile),c).astype(int)
        s=profile[teeth].sum()
        if s>best[0]: best=(s,o)
    return best[1]

import re
def words(pdf, page):
    out=subprocess.run(['pdftotext','-bbox','-f',str(page),'-l',str(page),pdf,'-'],capture_output=True,text=True).stdout
    return [(float(a),float(b),float(c),float(d),w) for a,b,c,d,w in
            re.findall(r'<word xMin="([\d.]+)" yMin="([\d.]+)" xMax="([\d.]+)" yMax="([\d.]+)">([^<]*)</word>',out)]

def fit(pairs):
    """least squares pos = a + (k+0.5)*c over (k,pos)"""
    k=np.array([p[0]+0.5 for p in pairs]); v=np.array([p[1] for p in pairs])
    A=np.vstack([np.ones_like(k),k]).T; (a,c),res,_,_=np.linalg.lstsq(A,v,rcond=None)
    err=np.abs(A@np.array([a,c])-v).max() if len(v) else 0
    return a,c,err

def grids(pdf, page, scale):
    """the grids on the page from their edge numbers: (x0, y0, c, n across, n down) in pixels at scale px/pt"""
    ws=[(0.5*(a+c)*scale,0.5*(b+d)*scale,int(w)) for a,b,c,d,w in words(pdf,page) if re.fullmatch(r'\d{1,2}',w) and int(w)<=31]
    found=[]
    used=set()
    # top axes: numbers on one line (y within 0.3 of a line height) rising left to right, evenly spaced
    ws_sorted=sorted(ws,key=lambda t:t[1])
    rows=[]
    for x,y,n in ws_sorted:
        for r in rows:
            if abs(r[0][1]-y)<6*scale: r.append((x,y,n)); break
        else: rows.append([(x,y,n)])
    for r in rows:
        r=sorted(r)
        # longest run with n rising by one and even spacing
        best=[]
        for i in range(len(r)):
            run=[r[i]]
            for j in range(i+1,len(r)):
                if r[j][2]==run[-1][2]+1 or (r[j][2]>run[-1][2] and r[j][2]-run[-1][2]<=3): run.append(r[j])
            if len(run)>len(best): best=run
        if len(best)<6 or best[0][2]>2: continue
        a,c,err=fit([(n,x) for x,y,n in best])
        if c<8 or err>0.25*c: continue
        ytop=np.median([y for _,y,_ in best])
        # left axis: numbers in a column left of a, below ytop
        col=[(x,y,n) for x,y,n in ws if abs(x-(a-0.6*c))<0.7*c and y>ytop and y<ytop+(34)*c]
        col=sorted(col,key=lambda t:t[1])
        if len(col)>=3:
            b,c2,err2=fit([(n,y) for x,y,n in col])
            if err2>0.3*c: b=None
        else: b=None
        found.append({'x0':a,'c':c,'y0':b,'ytop':ytop,'across':best[-1][2]+1,'down':(max(n for _,_,n in col)+1 if col else None),'top':len(best),'left':len(col)})
    return found
