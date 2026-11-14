#!/usr/bin/env python3
# xmi_lengths.py <file.XMI ...>: each sequence's length (120 ticks a second), notes, loop controllers (CC 116/117)
import struct,sys
def chunks(b,p,end):
    while p+8<=end:
        cid=b[p:p+4]; ln=struct.unpack('>I',b[p+4:p+8])[0]
        yield cid,p+8,ln
        p+=8+ln+(ln&1)
def evnt(b,p,ln):
    end=p+ln; t=0; loops=0; notes=0
    while p<end:
        c=b[p]
        if c<0x80:
            t+=c; p+=1; continue
        st=c; p+=1; hi=st>>4
        if st==0xFF:
            typ=b[p]; p+=1; l=0
            while True:
                x=b[p]; p+=1; l=(l<<7)|(x&0x7F)
                if x<0x80: break
            p+=l
            if typ==0x2F: break
        elif st==0xF0:
            l=0
            while True:
                x=b[p]; p+=1; l=(l<<7)|(x&0x7F)
                if x<0x80: break
            p+=l
        elif hi==0x9:
            p+=2; notes+=1
            while b[p]&0x80: p+=1
            p+=1
        elif hi in (0x8,0xA,0xB,0xE):
            if hi==0xB and b[p] in (116,117): loops+=1
            p+=2
        elif hi in (0xC,0xD): p+=1
    return t,loops,notes
for f in sys.argv[1:]:
    b=open(f,'rb').read()
    out=[]
    def walk(p,end):
        for cid,q,ln in chunks(b,p,end):
            if cid in (b'FORM',b'CAT '): walk(q+4,q+ln)
            elif cid==b'EVNT':
                t,l,n=evnt(b,q,ln); out.append('%.0fs notes %d loopCC %d'%(t/120,n,l))
    walk(0,len(b))
    print(f.split('/')[-1],out)
