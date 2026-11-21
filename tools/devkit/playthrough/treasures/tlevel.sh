#!/bin/bash
# tlevel.sh <slot>: Treasures - every member trained at the Cheats' Trainer a level a visit, with the experience
# they have, until none can train (a magic-user's new spell: the list's first); prints the levels.
P=/tmp/claude-0/tplay; S=$1
for r in 1 2 3 4 5 6 7 8; do
  any=0
  for i in 0 1 2 3 4 5 6; do
    TAILN=0 $P/bin/p.sh "|B\"3%$i|T" SAVE=$S ${X:+SETEXP=$i:$X} > /dev/null
    grep -aq "screen 11 menu \[|Yes No\]" $P/run.log || continue
    TAILN=0 $P/bin/p.sh "|B\"3%$i|T/Y/L..|B" SAVE=$S ${X:+SETEXP=$i:$X} SAVENOW=$S > /dev/null
    grep -aq "training: Congrat" $P/run.log && any=1
    grep -a "training:\|\[note\]" $P/run.log | head -3 | cut -c1-100
  done
  [ $any = 0 ] && break
done
TAILN=0 $P/bin/p.sh "|B" SAVE=$S PARTYDUMP=1 > /dev/null
grep -aE "^  [0-6]: " $P/run.log | cut -c1-150
