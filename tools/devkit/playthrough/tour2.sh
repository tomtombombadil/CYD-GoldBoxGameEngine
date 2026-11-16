#!/bin/bash
# tour2.sh <a> <b> x,y ...: try.sh (RELEVEL) to each square in turn, alternating the slots; stops at a failure
P=/tmp/claude-0/play; a=$1; b=$2; shift 2
for g in "$@"; do
  echo "=== $g"
  RELEVEL=1 $P/bin/try.sh $a $g $b ${TRIES:-4} 2>&1 | grep -v "^STOP\|fixed after\|NOT SAVED" || { echo "FAILED at $g (slot $a)"; echo $a > $P/lastslot.txt; exit 1; }
  t=$a; a=$b; b=$t
done
echo $a > $P/lastslot.txt; echo "LAST SLOT $a"
