#!/bin/bash
# sellplain.sh <slot>: every unknown item identified (loot.py IDALL), then every unreadied item still with a plain
# name (no "+", no "of", not a missile, scroll, potion or other kept kind) sold at the Merchant cheat - the packs
# kept light (Secret, 2026-11-20: full packs left the sanctum's treasure behind)
P=/tmp/claude-0/chplay; S=$1
IDALL=1 python3 $P/bin/loot.py $S 0,1,2,3,4,5 > /dev/null
TAILN=0 $P/bin/p.sh '' SAVE=$S ITEMDUMP=2 > /dev/null
L=$(python3 - "$P/run.log" <<'PY'
import re,sys,os
m=-1; cnt={}
for l in open(sys.argv[1]):
    if l.startswith('  member '): m=int(l.split()[1]); continue
    if m<0 or not l.startswith('    '): continue
    if l.startswith('    *'): continue
    if re.search(r'hidden [1-9]', l): continue   # (still unidentified - Shal's vorpal was sold as a plain long sword)
    n=re.sub(r'\s+\[type.*','',l).strip()
    if '+' in n or ' of ' in n or n.startswith(('Wand','Ring of','Bracers','Cloak','Necklace','Gauntlets','Boots','Girdle','Potion','Scroll','Mage Scroll','Cleric Scroll','Staff Sling','Composite')): continue
    if re.search(r'Arrows|Quarrels|Darts|Bolts|Sling', n): continue
    if os.path.exists('/tmp/claude-0/chplay/gear.txt') and any(n in g.split(',') for g in open('/tmp/claude-0/chplay/gear.txt').read().strip().split(';') for g in [g.split(':',1)[-1]]): continue   # (the gear the party readies - never sold: a fight's Quick leaves it unreadied)
    cnt[(m,n)]=cnt.get((m,n),0)+1
print(','.join(f'{m}:{n}:{c}' for (m,n),c in cnt.items()))
PY
)
echo "selling: $L"
[ -n "$L" ] && SELLN="$L" python3 $P/bin/loot.py $S 0,1,2,3,4,5 | tail -3
