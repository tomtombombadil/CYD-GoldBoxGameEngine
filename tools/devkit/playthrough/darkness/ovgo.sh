#!/bin/bash
# ovgo.sh <slot> <x,y> [answer]: walk Darkness's overland to x,y (ovpath.py again after every stop - a place's
# message, an encounter), then adv.sh there (its "DO YOU ENTER?" answered <answer>)
P=/tmp/claude-0/dplay; S=$1
for i in 1 2 3 4 5 6 7 8; do
  TAILN=0 $P/bin/p.sh "" SAVE=$S PEEK=0025,0026 > /dev/null
  at=$(grep "  PEEK 002" $P/run.log | python3 -c "import sys;print(*[int(l.split()[3],16) for l in sys.stdin],sep=chr(10))" | tr '\n' ',' | sed 's/,$//')
  grep -q "script 17" <(grep "loaded SAV" $P/run.log) || { echo "not on the overland"; exit 0; }
  [ "$at" = "$2" ] && { echo "at $2"; exit 0; }
  path=$(S=$S python3 $P/py/ovpath.py $2 ${OVBLOCK:-3,4}) || { echo "no way"; exit 1; }
  TXTN=${TXTN:-12} $P/bin/adv.sh $S x "$path" ${3:-Y} | tail -${TXTN:-12}
done
