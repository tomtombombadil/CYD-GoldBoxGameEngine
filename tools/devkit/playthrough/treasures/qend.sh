#!/bin/bash
# qend.sh <slot> <tokenfile>: the rest of a fight on Quick (Continue Battle No), tokens written back
P=/tmp/claude-0/tplay; S=$(cat $2)
for k in $(seq 1 ${N:-60}); do
  r=$(TAILN=0 $P/bin/p.sh "$S" SAVE=$1 >/dev/null; grep "? screen" $P/run.log | tail -1)
  if echo "$r" | grep -q "Continue Battle"; then S="$S/N,"; continue; fi
  echo "$r" | grep -qE "screen 24 " || break
  if echo "$r" | grep -q "Quick"; then S="$S/Q,"; elif echo "$r" | grep -q "Aim:"; then S="$S/E,"; else S="$S.,"; fi
done
echo "$S" > $2; echo "$r"; grep -E "^\[fight\]" $P/run.log | tail -1
