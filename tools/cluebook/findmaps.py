import sys, numpy as np, subprocess, json
sys.path.insert(0,'.')
from cluemap import render, grids
from locate import *
pdf, geo = sys.argv[1], sys.argv[2]
DPI=100
scales=[float(s)*DPI/150 for s in sys.argv[3].split(',')]
pages=[int(p) for p in sys.argv[4].split(',')] if len(sys.argv)>4 and sys.argv[4] else None
blocks=geo_blocks(geo)
n=int(subprocess.run(['pdfinfo',pdf],capture_output=True,text=True).stdout.split('Pages:')[1].split()[0])
tm={}
results=[]
for p in (pages or range(1,n+1)):
    im=render(pdf,p,DPI); L=page_lines(im)
    if L.mean()<0.02: continue
    for c in scales:
        for bk in blocks:
            key=(bk['name'],c)
            if key not in tm:
                H,V=wall_sides(bk); t,s=template(H,V,c,(int(17*c)+4,int(17*c)+4))
                tm[key]=(t,s,t.sum(),s.sum())
            t,s,ts,ss=tm[key]
            sc=correlate(L,t)/ts - correlate(L,s)/max(ss,1)
            i=np.unravel_index(np.argmax(sc),sc.shape)
            results.append((float(sc[i]),p,bk['name'],c,int(i[1]),int(i[0])))
    best=sorted([r for r in results if r[1]==p],reverse=True)[:3]
    print(p, [(round(a,2),b,cc,x,y) for a,_,b,cc,x,y in best], flush=True)
json.dump(results,open(sys.argv[5] if len(sys.argv)>5 else './res.json','w'))
