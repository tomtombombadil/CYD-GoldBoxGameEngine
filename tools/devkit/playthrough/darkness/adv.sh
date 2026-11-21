#!/bin/bash
# adv.sh <slot> x <tokens> [Y|N]: Darkness kit - the playsim's own auto tokens (AUTOTOK: texts, pauses, fights on
# Quick - "/Q" at each fighter's menu, "Continue Battle" No) from <tokens>; Yes / No answered <answer>, a treasure
# taken (takeall.py); saved at the Move menu (env EXTRA for every run). Prints the texts and journal entries.
P=/tmp/claude-0/dplay; S=$1; T="$3"; Q=${4:-Y}
[ -z "$T" ] && T="."
for i in $(seq 1 ${ITER:-30}); do
  PTIME=${PTIME:-2400} AT=1000 TN=0 $P/bin/auto.sh "$T" SAVE=$S ALLTEXT=1 $EXTRA > /dev/null
  T=$(cat $P/tok.txt); st=$(grep '? screen' $P/run.log | tail -1)
  if echo "$st" | grep -q "wait 13 \|wait 1 "; then T="$T."
  elif echo "$st" | grep -q "Yes No"; then T="$T/$Q"
  elif echo "$st" | grep -q "WHO WILL\|Select\]"; then T="$T%${WHO:-2}/S"
  elif echo "$st" | grep -q "screen 5 .*Take"; then T="$(python3 $P/bin/takeall.py $S "$T" "" 2>/dev/null)/E"
  elif echo "$st" | grep -q "screen 5 \|screen 25 "; then T="$T/E"
  elif echo "$st" | grep -q "Aim Use"; then :
  else break; fi
done
grep -E "TEXT|JOURNAL|won:|lost" $P/run.log | tail -${TXTN:-10} | cut -c1-120
echo "$st" | cut -c1-110
echo "$st" | grep -q "|Move Area" && { TAILN=0 $P/bin/p.sh "$T" SAVE=$S SAVENOW=$S $EXTRA > /dev/null; grep "saved now" $P/run.log; }
echo "$T" > $P/tt.txt
