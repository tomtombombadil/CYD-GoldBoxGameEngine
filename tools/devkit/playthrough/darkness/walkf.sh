#!/bin/bash
# walkf.sh <slot> <tokens> <x,y>: walk to x,y (MAPPATH a step at a time), fleeing each encounter menu
# that offers Flee; stops at any other menu or at the goal. Tokens in tok.txt.
P=/tmp/claude-0/dplay; S=$1; T=$2; G=$3; shift 3
for i in $(seq 1 60); do
  TAILN=0 $P/bin/p.sh "$T" SAVE=$S MAPPATH=$G "$@" > /dev/null
  st=$(grep -E "^  \? screen" $P/run.log | tail -1)
  if [ -z "$NOFLEE" ] && echo "$st" | grep -q "Flee"; then T="$T/F,"; echo "  (fled at $(grep '^   now' $P/run.log | tail -1 | cut -c10-16))"; continue; fi
  if echo "$st" | grep -q "Press button"; then T="$T."; continue; fi
  # (a wizard-locked door - Bash all but hopeless: Knock when someone has it memorized)
  if echo "$st" | grep -q "Locked"; then if echo "$st" | grep -q "Knock"; then T="$T/K"; else T="$T/B,"; fi; continue; fi
  # (screen 24 under the exploring words: a fight or a script's treasure starting - 2026-11-23, walked on and
  # saved there, Champions' Huerzyd statue treasure was lost)
  if ! echo "$st" | grep -q "Cast View Encamp" || echo "$st" | grep -q "screen 24 "; then echo "STOP: $st"; break; fi
  path=$(grep -E "  MAPPATH " $P/run.log | tail -1 | sed 's/.*: //')
  if [ -z "$path" ] || [ "$path" = "no way" ]; then echo "AT GOAL or no way ($path)"; break; fi
  T="$T${path:0:1}"
done
echo "$T" > $P/tok.txt; grep '^   now' $P/run.log | tail -1
