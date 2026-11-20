#!/bin/bash
# runev.sh <slot> <addr> <tokens> [answer]: a script event started by hand (RUNAT), its pages pressed on, Yes / No
# questions answered <answer> (default Y), a treasure taken (takeall), a fight fought (fight.sh); then saved.
# Prints the texts and journal entries. (env RUNSET / EXTRA passed on as EVENV)
P=/tmp/claude-0/splay; S=$1; A=$2; T="$3"; Q=${4:-Y}
export EVENV="RUNAT=$A ${RUNSET:+RUNSET=$RUNSET} $EXTRA" EVTOK="${T:0:1}"
[ -z "$T" ] && { T="."; export EVTOK="."; }
for i in $(seq 1 14); do
  TAILN=0 $P/bin/p.sh "$T" SAVE=$S ALLTEXT=1 > /dev/null; st=$(grep '? screen' $P/run.log | tail -1)
  if echo "$st" | grep -q "Press\|PRESS"; then T="$T."
  elif echo "$st" | grep -q "Yes No"; then T="$T/$Q"
  elif echo "$st" | grep -q "WHO WILL"; then T="$T%${WHO:-2}/S"
  elif echo "$st" | grep -q "screen 5 "; then T="$(python3 $P/bin/takeall.py $S "$T" "" 2>/dev/null)/E/N,"
  elif echo "$st" | grep -q "screen 24 " && ! echo "$st" | grep -qE "Aim|Quick"; then T="$T."
  elif echo "$st" | grep -q "screen 24 "; then $P/bin/fight.sh $S "$T" ev$A 2>&1 | tail -2; exit 0
  else break; fi
done
grep -E "TEXT|JOURNAL" $P/run.log | tail -${TXTN:-10} | cut -c1-120
echo "$st" | cut -c1-90
echo "$st" | grep -q "|Move" && { TAILN=0 $P/bin/p.sh "$T" SAVE=$S SAVENOW=$S > /dev/null; grep "saved now" $P/run.log; }
echo "$T" > $P/tt.txt
