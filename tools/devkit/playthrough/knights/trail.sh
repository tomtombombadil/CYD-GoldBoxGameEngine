#!/bin/bash
# trail.sh <slot> <tokens>: each prefix of the tokens (a letter a step / turn) run from the save: the square, facing,
# 0x4C0F (Vingaard's red-door trail) and the last text - to follow a step-by-step trail
P=/tmp/claude-0/knplay; S=$1; T=$2
for i in $(seq 1 ${#T}); do
  t=${T:0:$i}
  TAILN=0 $P/bin/p.sh "$t" SAVE=$S ALLTEXT=1 PEEKEND=${PK:-4C0F,4C57} > /dev/null
  echo "$t: $(grep '   now' $P/run.log | tail -1 | cut -c4-12) $(grep PEEKEND $P/run.log | head -2 | awk '{print $4}' | tr '\n' ' ') $(grep TEXT $P/run.log | tail -1 | cut -c3-80)"
done
