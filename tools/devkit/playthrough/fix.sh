#!/bin/bash
# fix.sh <slot> [rest]: the Cheats tab's Healer on every hurt member (0 HP: Raise Dead first, then Cure
# Critical Wounds until full), then (rest=1) the Sanctuary's rest; saved back to the slot. Prints the party.
P=/tmp/claude-0/play; S=$1
TAILN=0 $P/bin/p.sh "|B" SAVE=$S PARTYDUMP=1 > /dev/null
T='|B"0'
while read -r i hp mx; do
  [ "$hp" = "$mx" ] && continue
  [ "$hp" = 0 ] && T="$T%$i/Hvvvvvv/H/N/E"
  n=$(( (mx - hp) / 14 + 1 )); [ $n -gt 6 ] && n=6
  for k in $(seq $n); do T="$T%$i/Hvvvv/H/E"; done
done < <(grep -E "^  [0-7]: " $P/run.log | head -8 | sed -E 's/^  ([0-7]): .* HP ([0-9-]+)\/([0-9]+).*/\1 \2 \3/')
T="$T/E"
# (MEM="m:picks m:picks": the casters' Memorize picks in the camp before the rest)
for g in $MEM; do T="$T%${g%%:*}/E/M/M${g#*:}/E/E/Y/E/E"; done
[ -n "$2" ] && T="$T\"2/R/R$(printf '_%.0s' $(seq 40))/E"
TAILN=0 $P/bin/p.sh "$T" SAVE=$S SAVENOW=$S > /dev/null
TAILN=0 $P/bin/p.sh "|B" SAVE=$S PARTYDUMP=1 > /dev/null
grep -E "^  [0-7]: " $P/run.log | head -8 | cut -c1-48
grep "mem:" $P/run.log | grep -E " [0-9A-F]{2}@" | sed "s/^  member //"
