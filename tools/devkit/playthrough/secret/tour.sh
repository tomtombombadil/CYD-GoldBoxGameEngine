#!/bin/bash
# tour.sh <slot A> <slot B> x,y ... : hop from A to each square in turn (saving alternately in B / A),
# the party fixed (Healer + Sanctuary rest) after each; the last slot is printed
P=/tmp/claude-0/splay; a=$1; b=$2; shift 2
for g in "$@"; do
  echo "=== $g"
  set -o pipefail; TN=${TN:-12} $P/bin/hop.sh $a $g $b | grep -v "^  ? screen" || { echo "STOPPED at $g (slot $a kept)"; exit 1; }
  $P/bin/fix.sh $b 1 | awk '{printf "%s %s %s | ", $2, $5, $6} END {print ""}'
  t=$a; a=$b; b=$t
done
echo "LAST SLOT $a"
