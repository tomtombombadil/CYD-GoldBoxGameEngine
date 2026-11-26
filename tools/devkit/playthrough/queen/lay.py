import sys
def lay(f):
    b=open(f,'rb').read(); n=b[0x400+13]; p=0x40F; out=[]
    for i in range(n):
        r=b[p:p+0x19E]; nm=r[0x68:0x77].split(b'\0')[0]; st=p
        p+=0x19E+17*r[0xC9]; more=any(r[4:8]); ne=0
        while more:
            e=b[p:p+9]; p+=9; ne+=1; more=any(e[5:9])
        out.append((nm,st,r[0xC9],ne))
    return len(b),p,out
for f in sys.argv[1:]: print(f,lay(f))
