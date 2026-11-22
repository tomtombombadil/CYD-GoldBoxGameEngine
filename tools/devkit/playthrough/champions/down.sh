#!/bin/bash
# down.sh <slot> <x,y>: Champions kit - go2 to x,y, then the step (each direction tried) that takes the party far
# away (a stair), taken and saved; prints where it leads
P=/tmp/claude-0/chplay; S=$1; Q=$2
$P/bin/go2.sh $S $Q > /dev/null 2>&1
for t in F LF RF LLF; do
  TAILN=0 $P/bin/p.sh "$t" SAVE=$S > /dev/null
  w=$(grep "   now" $P/run.log | tail -1 | awk '{print $2}')
  dx=$(( ${w%,*} - ${Q%,*} )); dy=$(( ${w#*,} - ${Q#*,} ))
  if [ $(( dx*dx + dy*dy )) -gt 2 ]; then
    TXTN=6 ITER=10 timeout 2400 $P/bin/adv.sh $S x "$t" ${DOWNANS:-Y} 2>&1 | grep -E "TEXT|fight\]|saved" | grep -v "clear):  $" | tail -4
    echo "  $Q --$t--> $w"; exit 0
  fi
done
echo "  no stair at $Q"; exit 1
