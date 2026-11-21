#!/bin/bash
# glevel.sh <slot> <exp>: Gateway - every own member (rows 0-5) given <exp> (SETEXP) and trained at the
# Cheats' Trainer a level a visit until none can train; prints the levels.
P=/tmp/claude-0/tplay; S=$1; X=$2
for r in 1 2 3 4 5 6 7 8; do
  any=0
  for i in 0 1 2 3 4 5; do
    TAILN=0 $P/bin/p.sh "|B\"3%$i|T" SAVE=$S SETEXP=$i:$X > /dev/null
    grep -q "Do you wish to train" $P/run.log || continue
    any=1
    TAILN=0 $P/bin/p.sh "|B\"3%$i|T/Y/L..|B" SAVE=$S SETEXP=$i:$X SAVENOW=$S > /dev/null
  done
  [ $any = 0 ] && break
done
TAILN=0 $P/bin/p.sh "|B" SAVE=$S PARTYDUMP=1 > /dev/null
grep -aE "^  [0-6]: " $P/run.log | cut -c1-150
