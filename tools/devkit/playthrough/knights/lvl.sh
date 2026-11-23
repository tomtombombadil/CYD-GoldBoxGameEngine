#!/bin/bash
# lvl.sh <slot> <xp>: Champions kit - every member given <xp> (SETEXP) and trained by the Trainer cheat one level a
# visit until none may train (a new spell: the list's first); saved. Prints the levels.
P=/tmp/claude-0/knplay; S=$1; X=$2
for r in $(seq 1 12); do
  any=0
  for i in 0 1 2 3 4 5; do
    TAILN=0 $P/bin/p.sh "\"3%$i|T" SAVE=$S SETEXP=$i:$X > /dev/null
    grep -qi "do you wish to train" $P/run.log || continue
    any=1
    TAILN=0 $P/bin/p.sh "\"3%$i|T/Y/L..|B" SAVE=$S SETEXP=$i:$X SAVENOW=$S > /dev/null
    grep -q "saved now" $P/run.log || { echo "member $i: not saved"; grep -E "^  \? screen" $P/run.log | tail -1; }
  done
  [ $any = 0 ] && break
done
TAILN=0 $P/bin/p.sh "" SAVE=$S > /dev/null
grep -E "^  [0-5]: " $P/run.log | cut -c1-110
