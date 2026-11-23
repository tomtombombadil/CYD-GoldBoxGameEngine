#!/bin/bash
# kovgo.sh <slot> <x,y> [PRE]: Death Knights' overland - kovpath.py's walk from the save, played by adv.sh (an
# encounter met on the way: PICK Attack, Quick with the casters' spells; post.sh after a fight), again from each
# stop until the party stands on x,y; its "DO YOU ENTER?" answered ENTER (default Y). PRE: tokens before the walk
# (a question the save waits at - "/N")
P=/tmp/claude-0/knplay; S=$1; G=$2; PRE="$3"
for i in $(seq 1 ${KN:-8}); do
  T=$(python3 $P/bin/kovpath.py $G $S) || { echo "no way"; exit 1; }
  a=$(PICK=${PICK:-Attack} EXTRA="$EXTRA QASPELLS=1" TXTN=30 ITER=16 timeout 3000 $P/bin/adv.sh $S x "$PRE$T" ${ENTER:-Y} 2>&1)
  echo "$a" | grep -E "fight\]|JOURNAL" | cut -c1-110
  echo "$a" | grep -q "fight\]" && $P/bin/post.sh $S > /dev/null
  PRE=""
  TAILN=0 $P/bin/p.sh "" SAVE=$S PEEK=4BC3,4BC4 > /dev/null
  at=$(grep "  PEEK 4BC" $P/run.log | python3 -c 'import sys;print(*[int(l.split()[3],16) for l in sys.stdin],sep=chr(10))' | tr '\n' ',' | sed 's/,$//')
  grep -q "loaded SAV.*script 17" $P/run.log || { echo "$a" | grep TEXT | tail -${TXTN:-12} | cut -c1-120; echo "entered ($at)"; exit 0; }
  [ "$at" = "$G" ] && { echo "at $G"; exit 0; }
  echo "$a" | grep -q "saved now" || { echo "  STOPPED: $(echo "$a" | tail -2 | head -1 | cut -c1-110)"; exit 1; }
done
