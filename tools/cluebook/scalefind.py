import sys, numpy as np, subprocess
sys.path.insert(0,'.')
from cluemap import render
from locate import *
pdf, geo = sys.argv[1], sys.argv[2]
blocks=geo_blocks(geo)
n=int(subprocess.run(['pdfinfo',pdf],capture_output=True,text=True).stdout.split('Pages:')[1].split()[0])
dens=[]
pls={}
for p in range(1,n+1):
    L=page_lines(render(pdf,p,100)); pls[p]=L; dens.append((L.mean(),p))
top=[p for _,p in sorted(dens,reverse=True)[:4]]
best=[]
for c in np.arange(15,30.1,0.5):
    s=0
    for p in top:
        m=0
        for bk in blocks:
            H,V=wall_sides(bk); t,sv=template(H,V,c,(int(17*c)+4,int(17*c)+4))
            sc=correlate(pls[p],t)/t.sum()-correlate(pls[p],sv)/max(sv.sum(),1)
            m=max(m,sc.max())
        s=max(s,m)
    best.append((s,c)); print(f"{c:.1f} {s:.2f}",flush=True)
print("best", max(best))
