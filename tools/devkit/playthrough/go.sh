#!/bin/bash
# go.sh <save> <tokens> <x,y> [env...]: from the save + tokens, walk to x,y a step at a time
# (MAPPATH each step), events played by auto.sh; after a fight the party is fixed (fix.sh on slot
# J, the walk going on from there); stops at a choice or the goal. Final tokens in tok.txt, the slot
# they start from in slot.txt.
P=/tmp/claude-0/play
S=$1; T=$2; G=$3; shift 3; export MAPLOCKED=1
fights=0
for i in $(seq 1 80); do
  TAILN=0 $P/bin/p.sh "$T" SAVE=$S MAPPATH=$G "$@" > /dev/null
  path=$(grep "MAPPATH" $P/run.log | tail -1 | sed 's/.*: //')
  st=$(grep -E "^  \? screen" $P/run.log | tail -1)
  nf=$(grep -c "^\[fight\] won" $P/run.log)
  if [ "$nf" -gt "$fights" ] && echo "$st" | grep -q "Area Cast View" && [ -z "$NOFIX" ]; then
    TAILN=0 $P/bin/p.sh "$T" SAVE=$S SAVENOW=J "$@" > /dev/null
    $P/bin/fix.sh J 1 > /dev/null
    echo "  (fixed after a fight, at $(grep -E '^   now' $P/run.log | tail -1 | cut -c10-20))"
    S=J; T="|B"; fights=0; continue
  fi
  fights=$nf
  if echo "$st" | grep -q "Locked."; then T="$T${DOORTOK:-/B}"; echo "  (locked: ${DOORTOK:-/B})"; TN=4 $P/bin/auto.sh "$T" SAVE=$S "$@" > /dev/null; T=$(cat $P/tok.txt); continue; fi
  if ! echo "$st" | grep -q "Area Cast View"; then echo "STOP: $st"; break; fi
  if [ -z "$path" ] || [ "$path" = "no way" ]; then echo "AT GOAL or no way ($path)"; break; fi
  m=${path:0:1}
  TN=6 $P/bin/auto.sh "$T$m" SAVE=$S "$@" | grep -E "TEXT|MENU|LIST" | grep -v "TEXT (clear): $" | tail -4
  T=$(cat $P/tok.txt)
done
echo "$T" > $P/tok.txt; echo "$S" > $P/slot.txt
