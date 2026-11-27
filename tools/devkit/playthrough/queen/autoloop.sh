#!/bin/bash
# autoloop.sh <slot> <tokens> [n]: auto.sh again and again from its own tokens (SEED fixed - default 1) until a
# fight is won or lost and nothing waits, or n rounds; the texts after
P=/tmp/claude-0/qplay; S=$1; T="$2"
for i in $(seq 1 ${3:-12}); do
  SEED=${SEED:-1} PTIME=${PTIME:-2400} AT=${AT:-1000} TN=0 $P/bin/auto.sh "$T" SAVE=$S ALLTEXT=1 AUTOTAKE=/T AUTOMONEY=1 $EXTRA > /dev/null
  N=$(cat $P/tok.txt); [ "$N" = "$T" ] && break; T="$N"
  grep -q "won:\|lost" $P/run.log && grep '? screen' $P/run.log | tail -1 | grep -q "wait 0 " && break
done
echo "$T" > $P/tok.txt
grep -E "TEXT|JOURNAL|won:|lost" $P/run.log | tail -${TXTN:-10} | cut -c1-150; grep '? screen' $P/run.log | tail -1 | cut -c1-110
