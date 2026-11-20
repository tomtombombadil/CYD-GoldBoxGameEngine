#!/bin/bash
# endfight.sh <slot> <tokenfile>: the rest of a fight's turns passed (Guard, else Quick) until it ends
# (Continue Battle answered No); the tokens written back to the file
P=/tmp/claude-0/splay; S=$(cat $2)
for k in $(seq 1 ${N:-60}); do
  r=$(TAILN=0 $P/bin/p.sh "$S" SAVE=$1 >/dev/null; grep "? screen" $P/run.log | tail -1)
  if echo "$r" | grep -q "Continue Battle"; then S="$S/N,"; continue; fi
  echo "$r" | grep -qE "screen 24 " || break
  if echo "$r" | grep -q "Guard"; then S="$S/G,"
  elif echo "$r" | grep -q "Delay"; then S="$S/E/Q,"
  elif echo "$r" | grep -q "Done"; then S="$S/D,"
  elif echo "$r" | grep -q "Aim:"; then S="$S/E,"
  else S="$S.,"; fi
done
echo "$S" > $2; echo "$r"; grep -E "^\[fight\]" $P/run.log | tail -2
