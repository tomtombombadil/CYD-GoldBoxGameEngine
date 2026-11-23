#!/bin/bash
# walkans.sh <slot> <x,y> [PRE tokens]: walkf to x,y answering each question on the way by its text - No to
# buying / drinking / joining / waiting (NOANS, an extra grep pattern), else Yes; a press waited for: "."; then the
# tokens to tok_w.txt and adv.sh from them (its answers Y; PICK from the environment) - texts printed, saved
P=/tmp/claude-0/knplay; S=$1; G=$2; T="$3"
NO="${NOBASE:-BUY\|DRINK\|JOIN\|WAIT HERE}${NOANS:+\\|$NOANS}"
for i in $(seq 1 ${WN:-16}); do
  timeout 900 $P/bin/walkf.sh $S "$T" $G MAPLOCKED=1 MAPSECRET=1 ALLTEXT=1 > $P/w.out 2>&1; T=$(cat $P/tok.txt)
  l=$(grep -E "TEXT" $P/run.log | tail -1); s=$(grep '? screen' $P/run.log | tail -1)
  grep -q "AT GOAL" $P/w.out && break
  if echo "$s" | grep -q "Yes No"; then echo "$l" | grep -q "$NO" && T="$T/N" || T="$T/Y"
  elif echo "$s" | grep -q "wait 13 \|wait 1 "; then T="$T."
  else break; fi
done
echo "$T" > $P/tok_w.txt
TXTN=${TXTN:-20} ITER=${ITER:-14} timeout 3000 $P/bin/adv.sh $S x "$T" ${ANS:-Y} 2>&1 | grep -v "CUT THROUGH\|VANDALS" | tail -${TXTN:-20} | cut -c1-130
