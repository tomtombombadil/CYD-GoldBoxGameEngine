#!/bin/bash
# portals.sh <slot> [rounds]: Treasures' Crossroads (script 50) after its centre - goes to each flagged square
# in reach (0x15 - 0x29, 0x78 - 0x7F), tries the four ways off it; a way that moves the party somewhere else
# (a portal) or starts a fight is taken and saved, and the search goes on from there.
P=/tmp/claude-0/tplay; cd $P; S=$1; N=${2:-20}; done_=" "
for r in $(seq 1 $N); do
  MAPREACH=1 TAILN=0 bin/p.sh "|BLR" SAVE=$S >/dev/null 2>&1
  here=$(grep -a '   now' run.log | tail -1 | sed -E 's/.*now ([0-9]+,[0-9]+) .*/\1/')
  cand=$(grep -a "MAPREACH [0-9]" run.log | tr ' ' '\n' | grep -E "=(1[5-9A-F]|2[0-9]|7[89A-F])$" | cut -d= -f1)
  goal=""; for c in $cand; do case "$done_" in *" $c "*) ;; *) goal=$c; break;; esac; done
  [ -z "$goal" ] && { echo "round $r at $here: nothing new in reach"; break; }
  done_="$done_$goal "
  if [ "$goal" != "$here" ]; then AUTOPICK="Attack,Take,Go On" TL=6 bin/tourx.sh "$goal" >/dev/null 2>&1; fi
  TAILN=0 bin/p.sh "|BLR" SAVE=$S >/dev/null 2>&1; at=$(grep -a '   now' run.log | tail -1 | sed -E 's/.*now ([0-9]+,[0-9]+) .*/\1/')
  echo "round $r: goal $goal, at $at"
  [ "$at" != "$goal" ] && { echo "   (carried elsewhere on the way)"; continue; }
  for t in "F" "RF" "BF" "LF"; do
    AT=300 QASPELLS=1 AUTOLOOT=1 AUTOTAKE=/S/T/I TAKELEAVE=1 bin/auto.sh "|B$t" SAVE=$S >/dev/null 2>&1
    np=$(grep -a '   now' run.log | tail -1 | sed -E 's/.*now ([0-9]+,[0-9]+) .*/\1/'); f=$(grep -ac "a fight:" run.log)
    dx=$(( ${np%,*} - ${at%,*} )); dy=$(( ${np#*,} - ${at#*,} )); d=$(( (dx<0?-dx:dx) + (dy<0?-dy:dy) ))
    if [ $d -gt 1 ] || [ $f -gt 0 ] || grep -aq "script 51 of" run.log; then
      echo "   $t: -> $np fights $f $(grep -aE 'TEXT' run.log | grep -v 'CLEAR SK' | tail -1 | cut -c1-90)"
      TAILN=0 bin/p.sh "$(cat tok.txt)" SAVE=$S SAVENOW=$S >/dev/null 2>&1
      grep -aE "TEXT|a fight:|won:|LOAD MONSTER|LOOT take|script .* loaded" run.log | uniq | cut -c1-400 >> transcript.txt
      grep -aq "script 51 of" run.log && { echo OUT; exit 0; }
      bin/fix.sh $S 1 >/dev/null 2>&1; break
    fi
  done
done
