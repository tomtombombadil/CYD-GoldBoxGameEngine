#!/bin/bash
# sellall.sh <slot>: every member's unreadied items sold at the Merchant cheat but the kept kinds (lucky papers,
# arrows, wands, rods, special items, bows, slings, rings, darts); then every member's platinum set to 500 (coins
# weigh: a loaded member is refused every Take) and the party healed. Run loot.py first (it identifies).
P=/tmp/claude-0/tplay; S=$1
TAILN=0 $P/bin/p.sh "|B" SAVE=$S ITEMDUMP=1 >/dev/null 2>&1
L=$(python3 - <<'PY'
import re,collections
keep=('lucky','Arrows','Wand','rod','Seahelm','trident','Composite','Sling','Ring','Darts','Potion','Scroll')
cur=None; out=collections.OrderedDict()
for l in open('/tmp/claude-0/tplay/run.log',errors='replace'):
    m=re.match(r'  member (\d) ',l)
    if m: cur=m.group(1); continue
    m=re.match(r'      (\S.*?)\s*$',l)
    if m and cur is not None:
        n=m.group(1)
        if any(k in n for k in keep) and 'Staff Sling' not in n: continue
        out[(cur,n)]=out.get((cur,n),0)+1
print(','.join(f'{c}:{n}:{k}' for (c,n),k in out.items()))
PY
)
[ -n "$L" ] && SELLN="$L" python3 $P/bin/loot.py $S 0,1,2,3,4,5,6 2>&1 | grep -c sold
TAILN=0 $P/bin/p.sh "|B" SAVE=$S SAVENOW=$S SETMONEY=4:500 >/dev/null 2>&1
$P/bin/fix.sh $S 1 >/dev/null 2>&1
