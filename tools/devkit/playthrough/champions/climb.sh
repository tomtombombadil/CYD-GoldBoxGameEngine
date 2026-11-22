#!/bin/bash
# climb.sh <slot> <x,y> <dir> <floors>: floor.sh again and again - each floor's "STAIRS GOING UP TO THE <dir>"
# square (the tour's) is the next start; a backup per floor (bak/floor_<n>)
P=/tmp/claude-0/chplay; S=$1; Q=$2; D=$3
for n in $(seq 1 $4); do
  echo "##### floor $n from $Q $D"
  o=$($P/bin/floor.sh $S $Q $D 2>&1); echo "$o"
  $P/bin/bk.sh save $S floor_$n > /dev/null
  nx=$(echo "$o" | awk '/^=== /{q=$2} /UP TO THE/{getline l; split(l,a," "); print q, a[2]; exit}')
  [ -z "$nx" ] && { echo "no stairs up found"; break; }
  Q=${nx% *}; w=${nx#* }; D=${w:0:1}
done
