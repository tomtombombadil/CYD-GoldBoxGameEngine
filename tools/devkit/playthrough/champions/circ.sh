#!/bin/bash
# circ.sh <slot> <n>: Champions kit - Gargath Keep's circular stairs: Look at the stair square, UP, then tour
# the new floor's event squares (doors passed); n times; a backup each (bak/circ_<n>)
P=/tmp/claude-0/chplay; S=$1
for n in $(seq 1 $2); do
  echo "##### up $n"
  PICK="Up" TXTN=12 ITER=14 timeout 2400 $P/bin/adv.sh $S x "/L" N 2>&1 | grep -E "TEXT|fight\]|JOURNAL" | grep -v "clear):  $"
  $P/bin/post.sh $S > /dev/null
  TAILN=0 $P/bin/p.sh "," SAVE=$S MAPREACH=1 MAPLOCKED=1 MAPSECRET=1 > /dev/null
  here=$(grep "   now" $P/run.log | tail -1 | awk '{print $2}')
  sq=$(grep "MAPREACH [0-9]" $P/run.log | head -1 | tr ' ' '\n' | grep = | cut -d= -f1 | grep -v "^$here$" | tr '\n' ' ')
  echo "floor at $here: $sq"
  [ -n "$sq" ] && $P/bin/tourc.sh $S $sq 2>&1 | grep -E "^===|STOPPED|LOST|fight\]|JOURNAL|  at |TEXT" | grep -v "clear):  $" | awk '!s[$0]++' | cut -c1-120
  [ -n "$sq" ] && $P/bin/go2.sh $S $here > /dev/null 2>&1
  $P/bin/bk.sh save $S circ_$n > /dev/null
done
