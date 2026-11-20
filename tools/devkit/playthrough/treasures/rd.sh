#!/bin/bash
# rd.sh <slot>: the tanks' (0 1 2 4) melee weapon and shield readied, launchers put away; saved back
P=/tmp/claude-0/tplay; S=$1
TAILN=0 $P/bin/p.sh "" SAVE=$S ITEMDUMP=1 >/dev/null
T=$(python3 - "$P/run.log" <<'PY'
import re,sys
lines=open(sys.argv[1]).read().split('\n')
mem={}; cur=None
for l in lines:
    m=re.match(r'  member (\d) ',l)
    if m: cur=int(m.group(1)); mem[cur]=[]; continue
    m=re.match(r'    ([* ]) (.*)',l)
    if m and cur is not None: mem[cur].append((m.group(1)=='*',m.group(2)))
out=''
for w in (0,1,2,4):
    it=mem.get(w,[]); pos=0; t=''
    def go(k):
        global pos,t
        t+=('v'*(k-pos)) if k>pos else ('^'*(pos-k)); pos=k
    def find(f):
        for k,(r,n) in enumerate(it):
            if f(n): return k,r
        return None,None
    for k,(r,n) in enumerate(it):
        if r and re.search(r'Bow|Sling|Crossbow',n): go(k); t+='/R'
    melee=lambda n: re.search(r'Sword|Mace|Axe|Hammer|Flail',n) is not None
    if not any(r and melee(n) for r,n in it):
        k,r=find(melee)
        if k is not None: go(k); t+='/R'
    k,r=find(lambda n: n.startswith('Shield'))
    if k is not None and not r: go(k); t+='/R'
    if t: out+='%%%d/V/I,%s/E/E' % (w,t)
print(out)
PY
)
echo "rd: $T" >&2; [ -n "$T" ] && TAILN=0 $P/bin/p.sh "$T" SAVE=$S SAVENOW=$S >/dev/null
TAILN=0 $P/bin/p.sh "" SAVE=$S ITEMDUMP=1 >/dev/null; grep -E "^  member" $P/run.log | cut -c10-34 | tr '\n' ';'; echo
