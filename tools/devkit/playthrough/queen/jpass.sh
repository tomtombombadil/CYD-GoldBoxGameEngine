#!/bin/bash
# jpass.sh <slot> <PASS>: Hawkbluff (script 46, map 47) - the jeweler's pass: steps up and down the street until
# its timer (var A2, one a step) is out, to 4,15, S into the shop, "No", PASS, Yes, <PASS>, Yes; saved at 4,15
P=/tmp/claude-0/qplay; S=$1
TAILN=0 $P/bin/p.sh "." SAVE=$S MAPPATH=9,15 > /dev/null; G=$(grep MAPPATH $P/run.log | sed 's/.*: //' | sed 's/./&./g')
W="$G"; for i in 1 2 3 4 5 6; do W="${W}B.F.F.F.F.F.B.F.F.F.F.F."; done
TAILN=0 $P/bin/p.sh "$W" SAVE=$S SEED=1 MAPPATH=4,15 > /dev/null; B=$(grep MAPPATH $P/run.log | sed 's/.*: //' | sed 's/./&./g')
T="$W$B"; TAILN=0 $P/bin/p.sh "$T" SAVE=$S SEED=1 > /dev/null
d=$(grep '  now' $P/run.log | tail -1 | awk '{print $3}'); case $d in N) T="${T}B.";; E) T="${T}R.";; W) T="${T}L.";; esac
QMAX=10 TXTN=8 $P/bin/qtalk.sh $S "${T}F.." /Y "=$2;" /Y | grep -E "TEXT" | tail -5
TAILN=0 $P/bin/p.sh "$(cat $P/tok_t.txt).........." SAVE=$S SEED=1 SAVENOW=$S > /dev/null; grep "saved now" $P/run.log
