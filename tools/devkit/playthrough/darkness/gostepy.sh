#!/bin/bash
# gostepy.sh <slot> <x,y> [answer]: gostep.sh, but a question met on the way (a menu the walk stopped at) is answered
# with $WAYANS (default Y) by adv.sh and the walk tried again - up to 6 times
P=/tmp/claude-0/dplay
for k in 1 2 3 4 5 6; do
  o=$($P/bin/gostep.sh "$@" 2>&1); echo "$o" | grep -v "^no way" | tail -${TXTN:-12}
  echo "$o" | grep -q "^no way" || exit 0
  grep -q "STOP:" $P/gostep.log || { echo "$o" | tail -1; tail -4 $P/gostep.log; exit 1; }
  echo "  on the way: $(grep TEXT $P/gostep.log | tail -1 | cut -c1-100) -> ${WAYANS:-Y}"
  TXTN=3 $P/bin/adv.sh $1 x "$(cat $P/tok.txt)" ${WAYANS:-Y} 2>&1 | tail -3 | cut -c1-120
done
