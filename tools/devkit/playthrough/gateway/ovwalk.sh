#!/bin/bash
# ovwalk.sh <slot> <x,y> [allowed flags]: Gateway's overland on foot (or by boat: flags A) from the slot to x,y -
# ovgo.py's plan a few steps at a time, encounters attacked (auto.sh), the plan made anew after each; stops at
# a Yes / No (a town's "Enter ...?"). Tokens in tok.txt.
P=/tmp/claude-0/gplay; S=$1; G=$2; FL=${3:-0,2,3,4,5,7,8,9,B,C,E,F}
TAILN=0 $P/bin/p.sh "|B" SAVE=$S MAPFLAGS=1 > /dev/null; cp $P/run.log $P/ovmap.log
T="|B/M"
for i in $(seq 1 60); do
  TAILN=0 $P/bin/p.sh "$T" SAVE=$S > /dev/null
  now=$(grep -aE "^ +now [0-9]+," $P/run.log | tail -1 | sed -E 's/^ +now ([0-9]+,[0-9]+) (.).*/\1 \2/')
  st=$(grep -aE "^  \? screen" $P/run.log | tail -1)
  if echo "$st" | grep -q "Attack Flee"; then AT=500 $P/bin/auto.sh "$T/A" SAVE=$S > /dev/null; T="$(cat $P/tok.txt)/M"; echo "  (a fight at $now)"; continue; fi
  if echo "$st" | grep -q "|Yes No"; then echo "ASKED at $now: $(grep -a 'TEXT (clear)' $P/run.log | tail -1 | cut -c1-80)"; break; fi
  [ -z "$now" ] && now="$START"
  plan=$(python3 $P/bin/ovgo.py $P/ovmap.log $G $FL "$now") || { echo "no way from $now"; break; }
  [ -z "$plan" ] && { echo "AT GOAL $now"; break; }
  T="$T$(echo "$plan" | sed -E 's/^(([LRB]?F){1,3}).*/\1/')"
done
echo "$T" > $P/tok.txt; echo "$T" | cut -c1-150
