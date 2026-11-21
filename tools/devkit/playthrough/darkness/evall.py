# evall.py <dis file>: every ON GOTO / GOSUB (op 25 / 26) in a block's disassembly - each entry's address and first texts
import re,sys
dis=open(sys.argv[1],errors='replace').read().splitlines()
idx={}
for i,l in enumerate(dis):
    m=re.match(r'([0-9A-F]{4}) ',l)
    if m: idx.setdefault(m.group(1),i)
for l in dis:
    m=re.match(r'([0-9A-F]{4}) op 2[56]: 1:([0-9A-F]{4}) 0:([0-9A-F]{4})(.*)',l)
    if not m: continue
    addrs=re.findall(r'1:([0-9A-F]{4})',m.group(4))
    print('dispatch %s on %s, %d (%d shown)'%(m.group(1),m.group(2),int(m.group(3),16),len(addrs)))
    for k,a in enumerate(addrs):
        t=[]
        for l2 in dis[idx.get(a,len(dis)):idx.get(a,len(dis))+50]:
            if '"' in l2 and (' op 12' in l2 or ' op 11' in l2): t.append(l2.split('"')[1][:44])
            if len(t)>=2: break
        print('  %02X %s %s'%(k,a,' | '.join(t)))
