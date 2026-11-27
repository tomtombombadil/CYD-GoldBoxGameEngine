#!/bin/bash
# steps.sh SLOT N : walk back and forth (B F) N rounds of 40 steps where the party stands, an encounter
# fought (Attack, WINFIGHT) - for the Dark Queen's step counters (the lighthouse's 0x13D / 0x13E: the elf
# woman's hint after 7 x 200 steps). Saves after each round.
P=/tmp/claude-0/qplay; S=$1
for i in $(seq 1 $2); do
  T=$(python3 -c "print('${STEPS:-BF}'*20)")
  for k in 1 2 3 4 5 6; do
    TAILN=0 $P/bin/p.sh "$T" SAVE=$S WINFIGHT=1 > /dev/null
    st=$(grep -E "^  \? screen" $P/run.log | tail -1)
    echo "$st" | grep -q "Cast View Encamp" && break
    if echo "$st" | grep -q "Attack"; then T="$T/A"; elif echo "$st" | grep -q "Yes No"; then T="$T/N"; elif echo "$st" | grep -q "wait 13\|wait 1 "; then T="$T."; else echo "?? $st"; break; fi
  done
  TAILN=0 $P/bin/p.sh "$T" SAVE=$S WINFIGHT=1 SAVENOW=$S PEEKEND=13D,13E > /dev/null
  echo "$i: $(grep 'saved now' $P/run.log | cut -c3-60) $(grep PEEKEND $P/run.log | head -2 | awk '{print $4}' | tr '\n' ' ')"
  grep -q "13E = 0007\|013E = 0007" $P/run.log && break
done
