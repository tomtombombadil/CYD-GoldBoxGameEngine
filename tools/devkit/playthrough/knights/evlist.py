# evlist.py <dis file>: each "op 25: 1:7F79 0:n a0 a1 ..." dispatch (the square's event number) - event -> address -> first texts
import re,sys
dis=open(sys.argv[1],errors='replace').read().splitlines()
idx={}
for i,l in enumerate(dis):
    m=re.match(r'([0-9A-F]{4}) ',l)
    if m: idx.setdefault(m.group(1),i)
for l in dis:
    m=re.match(r'([0-9A-F]{4}) op 25: 1:7F79 0:([0-9A-F]{4})(.*)',l)
    if not m: continue
    addrs=re.findall(r'1:([0-9A-F]{4})',m.group(3))
    print('dispatch at %s, %d events (%d shown)'%(m.group(1),int(m.group(2),16),len(addrs)))
    for k,h in enumerate(addrs):
        t=[]
        if h in idx:
            for l2 in dis[idx[h]:idx[h]+40]:
                if '"' in l2: t.append(l2.split('"')[1][:50])
                if len(t)==2: break
        print('  %02X %s %s'%(k,h,' | '.join(t)))
