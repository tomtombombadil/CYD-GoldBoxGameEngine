#!/bin/bash
# stepwalk.sh <slot> <x,y> [answer] : walk one step at a time (MAPPATH again after each: the Heart's beat pushes
# the party about), questions on the way answered N (WAYANS), the last step's <answer>; up to $MAXSTEPS (60)
P=/tmp/claude-0/dplay; S=$1; tx=${2%,*}; ty=${2#*,}
for i in $(seq 1 ${MAXSTEPS:-60}); do
  TAILN=0 $P/bin/p.sh "" SAVE=$S MAPPATH=$2 MAPLOCKED=1 MAPSECRET=1 $GOENV >/dev/null
  at=$(grep -o "loaded SAV[^,]*, [0-9]*,[0-9]*" $P/run.log | grep -o "[0-9]*,[0-9]*$")
  [ "$at" = "$2" ] && { echo "at $2 after $((i-1)) steps"; exit 0; }
  p=$(grep MAPPATH $P/run.log | sed 's/.*: //'); [ "$p" = "no way" ] && { echo "no way from $at"; exit 1; }
  step=${p%%F*}F; rest=${p#*F}
  ans=${WAYANS:-N}; [ -z "$rest" ] && ans=${3:-Y}
  o=$(TXTN=${TXTN:-4} $P/bin/adv.sh $S x "$step" $ans 2>&1); echo "$o" | grep -E "TEXT|fight\]" | grep -vE "LUB|DUB|ARTERY|LINKING|SLIME" | cut -c1-130
done
echo "not there after ${MAXSTEPS:-60} steps (at $at)"; exit 1
