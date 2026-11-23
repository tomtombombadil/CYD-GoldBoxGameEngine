#!/bin/bash
# try.sh <from> <x,y> <to> [tries]: hop.sh, kept only if no level was drained and nobody is dead / gone after
# (fix.sh's healing aside); else again with the fight dice SEED=1.. (Tom: save often, reload on a drain)
P=/tmp/claude-0/knplay; A=$1; G=$2; B=$3; N=${4:-6}
lv() { TAILN=0 $P/bin/p.sh "|B" SAVE=$1 PARTYDUMP=1 > /dev/null; grep -E "^  [0-7]: " $P/run.log | sed -E 's/.*levels //' | tr ' ' '\n' | awk '{s+=$1} END {print s+0}'; }
base=$(lv $A); cnt=$(TAILN=0 $P/bin/p.sh "|B" SAVE=$A PARTYDUMP=1 > /dev/null; grep -cE "^  [0-7]: " $P/run.log)
for s in "" $(seq 1 $N); do
  if [ -n "$s" ]; then export SEED=$s; fi
  out=$(TN=${TN:-8} $P/bin/hop.sh $A $G $B 2>&1)
  if echo "$out" | grep -q "NOT SAVED"; then echo "  seed ${s:-0}: stopped"; echo "$out" | tail -3; continue; fi
  $P/bin/fix.sh $B 1 > /dev/null
  now=$(lv $B); c2=$(grep -cE "^  [0-7]: " $P/run.log)
  [ -n "$RELEVEL" ] && [ "$now" -lt "$base" ] && { echo "  seed ${s:-0}: drained ($base -> $now), relevelled: $($P/bin/relevel.sh $B)"; now=$(lv $B); }
  if [ "$now" -ge "$base" ] && [ "$c2" -ge "$cnt" ]; then echo "  seed ${s:-0}: ok (levels $base -> $now)"; echo "$out" | grep -E "TEXT|MENU|fight\]|JOURNAL" | grep -v "TEXT (clear):  *$" | tail -${TN:-8} | cut -c1-200; exit 0; fi
  echo "  seed ${s:-0}: levels $base -> $now, members $cnt -> $c2: again"
done
exit 1
