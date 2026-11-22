#!/bin/bash
# drop.sh <slot> "m:Item,Item;m:..." - Champions kit: member m drops the named (unreadied) items, from the overland's
# camp (DROPPRE, default /E; DROPPOST /E); saved. Prints each member's items after.
P=/tmp/claude-0/chplay; S=$1
TAILN=0 $P/bin/p.sh "" SAVE=$S ITEMDUMP=1 >/dev/null
T=$(python3 - "$P/run.log" "$2" <<'PY'
import sys,re
log=open(sys.argv[1],errors='replace').read().splitlines()
items={}; cur=None
for l in log:
    m=re.match(r'  member (\d+) ',l)
    if m: cur=int(m.group(1)); items[cur]=[]; continue
    if cur is not None and l.startswith('    '): items[cur].append(l[6:].strip())
    elif cur is not None and not l.startswith('    '): cur=None
t=''
for part in sys.argv[2].split(';'):
    if not part: continue
    m,names=part.split(':'); m=int(m); lst=list(items[m])
    for n in names.split(','):
        k=lst.index(n); t+='%%%d/V/I'%m+'v'*k+'/D/Y/E/E'; lst.pop(k)
print(t)
PY
)
echo "drop: $T"
TAILN=0 $P/bin/p.sh "${DROPPRE-/E}$T${DROPPOST-/E}" SAVE=$S SAVENOW=$S >/dev/null; grep "saved now" $P/run.log
grep -o "\[note\] [A-Za-z ,.]*" $P/run.log | sort | uniq -c
TAILN=0 $P/bin/p.sh "" SAVE=$S ITEMDUMP=1 >/dev/null; grep -A12 "^  member" $P/run.log | grep -v "^--" | cut -c1-90
