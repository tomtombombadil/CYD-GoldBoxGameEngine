#!/bin/bash
# rdy.sh <slot> "m:Item,Item#2,...;m:..." - member m's readied items made exactly these (Name#n: the n-th of that
# name; arrows / darts / potions etc. left as they are unless named); saved back, the readied lists printed
P=/tmp/claude-0/dplay; S=$1
TAILN=0 $P/bin/p.sh "" SAVE=$S ITEMDUMP=1 >/dev/null
T=$(python3 - "$P/run.log" "$2" <<'PY'
import re,sys,os
lines=open(sys.argv[1]).read().split('\n')
mem={}; cur=None
for l in lines:
    m=re.match(r'  member (\d) ',l)
    if m: cur=int(m.group(1)); mem[cur]=[]; continue
    m=re.match(r'    ([* ]) (.*)',l)
    if m and cur is not None: mem[cur].append([m.group(1)=='*',m.group(2).strip()])
keep=re.compile(os.environ.get("RDYKEEP") or r"Arrows|Quarrels|Darts|Potion|Scroll|Dust|Stone")
out=''
for grp in [g for g in sys.argv[2].split(';') if g.strip()]:
    w,names=grp.strip().split(':',1); w=int(w); it=mem[w]
    want=set()
    for nm in names.split(','):
        n=1
        if '#' in nm: nm,n=nm.split('#'); n=int(n)
        c=0
        for k,(r,x) in enumerate(it):
            if x==nm:
                c+=1
                if c==n: want.add(k)
    pos=0; t=''
    def go(k):
        global pos,t
        t+=('v'*(k-pos)) if k>pos else ('^'*(pos-k)); pos=k
    for k,(r,x) in enumerate(it):
        if r and k not in want and not keep.search(x): go(k); t+='/R'
    for k in sorted(want):
        if not it[k][0]: go(k); t+='/R'
    if t: out+='%%%d/V/I,%s/E/E' % (w,t)
print(out)
PY
)
echo "rdy: $T" >&2
[ -n "$T" ] && TAILN=0 $P/bin/p.sh "${RDYPRE}$T${RDYPOST}" SAVE=$S SAVENOW=$S >/dev/null   # (RDYPRE=/C RDYPOST=/E: the overland, View is in Camp)
TAILN=0 $P/bin/p.sh "" SAVE=$S ITEMDUMP=1 >/dev/null
awk '/^  member/{printf "\n%s %s %s:", $3,$4,$5} /^    \*/{sub(/^    \* /,""); printf " %s;", $0}' $P/run.log; echo
