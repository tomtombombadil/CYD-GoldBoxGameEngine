import sys
def layout(d):
    n=d[0x400+13]|d[0x400+14]<<8; p=0x40F; out=[]
    for i in range(n):
        r=d[p:p+0x19E]; ni=r[0xC9]; st=p; p+=0x19E+17*ni; effs=[]
        more=any(r[4:8])
        while more:
            e=d[p:p+9]; effs.append(e); p+=9; more=any(e[5:9])
        out.append((st,r,ni,effs))
    return n,out,p
a=open(sys.argv[1],'rb').read(); b=open(sys.argv[2],'rb').read()
na,la,ra=layout(a); nb,lb,rb=layout(b)
print('count',na,nb,'rest at',ra,rb,'sizes',len(a),len(b))
for (sa,r1,i1,e1),(sb,r2,i2,e2) in zip(la,lb):
    print(r1[0x68:0x77].split(b'\0')[0], 'items',i1,i2,'effects',len(e1),len(e2))
    diffs=[k for k in range(0x19E) if r1[k]!=r2[k]]
    print('  record differs at', ' '.join('%X:%02X/%02X'%(k,r1[k],r2[k]) for k in diffs[:60]))
