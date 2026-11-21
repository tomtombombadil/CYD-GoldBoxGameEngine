#!/usr/bin/env python3
# sell2.py [extra names,comma]: Merchant tokens from the last ITEMDUMP - every unreadied plain item (and the named
# ones) sold from the TOP of each member's list down (the list closes up, so indexes stay small - no paging)
import re,sys
extra=set(x for x in (sys.argv[1] if len(sys.argv)>1 else '').split(',') if x)
keep=re.compile(r'Arrows|Bolts|Sling|Bow|Potion|Scroll|Wand|Ring|Bracers|Cloak|Gauntlets|Necklace|Helm|Boots|Girdle|rod|Rod|Staff|Dust')
mem=[];cur=None
for l in open('/tmp/claude-0/tplay/run.log',errors='replace'):
    m=re.match(r'  member (\d+) ',l)
    if m: cur=[]; mem.append((int(m.group(1)),cur)); continue
    m=re.match(r'    ([* ]) (.*)',l)
    if m and cur is not None: cur.append((m.group(1)=='*',m.group(2).strip()))
    elif cur is not None and not l.startswith('    '): cur=None
out='"1'
for mi,its in mem:
    its=list(its); k=0
    while k<len(its):
        rd,nm=its[k]
        sell = (not rd) and (nm in extra or ('+' not in nm and ' of ' not in nm and 'AC ' not in nm and not keep.search(nm)))
        if sell: out+='%%%d/V/I%s/S/Y/E/E'%(mi,'v'*k); del its[k]
        else: k+=1
print(out+'/E')
