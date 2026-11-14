# cmpdro.py <dro> <log>: the log's notes found in the DOSBox capture; their pitch, channel, timing and the
# instrument registers (only those the capture saw written after its start dump) compared at every note-on
import sys
from dro import read_dro, read_log
from collections import Counter
def snaps(ws, skip_dump):
    reg=[0]*256; seen=set(); out=[]
    for t,r,v in ws:
        old=reg[r]; reg[r]=v
        if not (skip_dump and t==0): seen.add(r)
        if 0xB0<=r<=0xB8 and (v&0x20) and not (old&0x20): out.append((t,r-0xB0,(v&0x1F)<<8|reg[0xA0+r-0xB0],list(reg),set(seen)))
    return out
A=snaps(read_dro(sys.argv[1]),True); B=snaps(read_log(sys.argv[2]),False)
sa=[(c,f) for t,c,f,_,_ in A]; sb=[(c,f) for t,c,f,_,_ in B]
print(len(sa),'captured notes,',len(sb),'rendered notes')
best=(0,0,0)
for o in range(len(sa)):
    for s in range(min(len(sb),200)):
        if sa[o]!=sb[s]: continue
        k=0
        while s+k<len(sb) and o+k<len(sa) and sa[o+k]==sb[s+k]: k+=1
        if k>best[0]: best=(k,o,s)
k,o,s=best
c=Counter(); n=0; dt=0
for i in range(k):
    ra,seen=A[o+i][3],A[o+i][4]; rb=B[s+i][3]
    dt=max(dt,abs((A[o+i][0]-A[o][0])-(B[s+i][0]-B[s][0])))
    for r in seen:
        if 0xB0<=r<=0xB8: continue
        n+=1
        if ra[r]!=rb[r]: c[r]+=1
print('identical run: %d notes (capture note %d = rendered note %d, %.1f s); timing max diff %.0f ms; registers compared %d, differing %d'%(k,o,s,(B[s+k-1][0]-B[s][0])/1000,dt,n,sum(c.values())),c.most_common(5))
