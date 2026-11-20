#!/usr/bin/env python3
# shop.py id|sell: tokens for the Merchant cheat from the last ITEMDUMP in run.log
#  id   - every unreadied item without a plus in its name is identified
#  sell - every unreadied item still without a plus is sold (from the bottom up), arrows kept
import re,sys
mode=sys.argv[1]; keep=re.compile(r'Arrows|Bolts|Sling|Bow|Potion|Scroll|Wand|Ring|Bracers|Cloak|Gauntlets|Necklace|Helm|Boots|Girdle')
mem=[]; cur=None
for l in open('/tmp/claude-0/tplay/run.log',errors='replace'):
    m=re.match(r'  member (\d+) ',l)
    if m: cur=[]; mem.append((int(m.group(1)),cur)); continue
    m=re.match(r'    ([* ]) (.*)',l)
    if m and cur is not None: cur.append((m.group(1)=='*',m.group(2).strip()))
    elif cur is not None and not l.startswith('    '): cur=None
out='"1'
for mi,its in mem:
    idx=list(range(len(its)))
    if mode=='sell': idx=idx[::-1]
    for k in idx:
        rd,nm=its[k]
        if rd or '+' in nm: continue
        if mode=='sell' and keep.search(nm): continue
        if mode=='id' and re.search(r'Arrows',nm): continue
        out+='%%%d/V/I%s/%s/Y/E/E'%(mi,'v'*k,'I' if mode=='id' else 'S')
print(out+'/E')
