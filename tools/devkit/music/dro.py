#!/usr/bin/env python3
# dro.py: DOSBox raw OPL captures (.dro v1 / v2; second-chip writes dropped) and mmlog logs as (ms, reg, val)
import struct,sys
def read_dro(p):
    d=open(p,'rb').read(); assert d[:8]==b'DBRAWOPL', d[:8]
    maj,mn=struct.unpack_from('<HH',d,8)
    if (maj,mn)==(0,1):
        ms,ln,hw=struct.unpack_from('<IIB',d,12); p=24 if d[20:24] else 23
        p=0x18
        t=0; out=[]
        while p<len(d):
            c=d[p]; p+=1
            if c==0: t+=d[p]+1; p+=1
            elif c==1: t+=struct.unpack_from('<H',d,p)[0]+1; p+=2
            elif c in (2,3): pass
            elif c==4: out.append((t,d[p],d[p+1])); p+=2
            else: out.append((t,c,d[p])); p+=1
        return out
    if maj==2:
        npairs,ms,hw,fmt,comp,sd,ld,cml=struct.unpack_from('<IIBBBBBB',d,12)
        cm=d[26:26+cml]; p=26+cml; t=0; out=[]
        for i in range(npairs):
            c,v=d[p],d[p+1]; p+=2
            if c==sd: t+=v+1
            elif c==ld: t+=(v+1)<<8
            elif not c&0x80: out.append((t,cm[c],v))
        return out
    raise SystemExit('dro version %d.%d'%(maj,mn))
def read_log(p):
    hz=18.2; t=0; out=[]
    for l in open(p):
        if l.startswith('HZ'): hz=float(l[3:]); continue
        if l[0]=='T': t=int(l[2:])/hz*1000; continue
        r,v=l.split(); out.append((t,int(r,16),int(v,16)))
    return out
if __name__=='__main__':
    a=read_dro(sys.argv[1]); b=read_log(sys.argv[2])
    print(len(a),'dro writes;',len(b),'log writes')
    sa=[(r,v) for t,r,v in a]; sb=[(r,v) for t,r,v in b]
    # find the log's sequence (after its init) inside the dro
    key=sb[40:60]
    for i in range(len(sa)-20):
        if sa[i:i+20]==key: print('match at dro write',i,'time %d ms'%a[i][0]); break
