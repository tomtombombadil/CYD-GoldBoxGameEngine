#!/bin/bash
# hop.sh <from slot> <x,y> <to slot>: go.sh from the slot's start, then save as <to slot>; shows the new texts
P=/tmp/claude-0/gplay
$P/bin/go.sh $1 "|B${HOPTOK}" $2 > $P/hop.log 2>&1
grep "fixed after" $P/hop.log; tail -1 $P/hop.log
S=$(cat $P/slot.txt)
if grep -q "^STOP" $P/hop.log; then echo "NOT SAVED (stopped; from slot $S)"; exit 1; fi
TAILN=0 $P/bin/p.sh "$(cat $P/tok.txt)" SAVE=$S SAVENOW=$3 MAPLOCKED=1 ALLTEXT=1 > /dev/null
grep -E "TEXT|MENU|LIST|\[fight\]|LOAD MONSTER|JOURNAL" $P/run.log | grep -v "TEXT (clear): $" | uniq | cut -c1-260 | tail -${TN:-20}
grep -E "saved now|^  \? screen" $P/run.log | tail -2
convert $P/last.png -scale 50% $P/b.png
