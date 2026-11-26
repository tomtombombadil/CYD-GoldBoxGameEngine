#!/bin/bash
# fix.sh <slot>: Darkness kit - the Cheats' Healer: Heal (the list's 6th) on every member below full (unconscious
# too), Raise Dead (8th) first for the dead (status 6); then Exit, saved. Prints the party.
P=/tmp/claude-0/qplay; S=$1
TAILN=0 $P/bin/p.sh "" SAVE=$S CAMPRUN='!' > /dev/null
T='|B"0'; k=0
while read -r nm hp st; do
  cur=${hp%/*}; mx=${hp#*/}
  [ "$st" = 6 ] && T="$T%$k/Hvvvvvvv/H/E"
  [ "$cur" != "$mx" -o "$st" != 0 ] && T="$T%$k/Hvvvvv/H/E"
  k=$((k + 1))
done < <(grep -a " HP .* st " $P/run.log | head -8 | sed -E 's/^ *([A-Z ]+) HP ([0-9]+\/[0-9]+) st ([0-9]+).*/\1 \2 \3/' | awk '{print $1, $(NF-1), $NF}')
T="$T/E"
TAILN=0 $P/bin/p.sh "$T" SAVE=$S SAVENOW=$S > /dev/null; grep "saved now" $P/run.log
TAILN=0 $P/bin/p.sh "" SAVE=$S CAMPRUN='!' > /dev/null; grep -a " HP .* st " $P/run.log | head -8 | cut -c1-60
