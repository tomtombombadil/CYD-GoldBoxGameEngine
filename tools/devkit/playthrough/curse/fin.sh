#!/bin/bash
# fin.sh <slot> <tokenfile> <bakname> [seed]: a won fight finished by Tom's rules (2026-11-16): every item
# taken (takeall.py), the coins shared, the treasure left; saved; the party healed and rested (fix.sh); the
# items sorted by the Merchant cheat (loot.py: plain ones sold, magic ones identified); the fighters' weapons
# readied again (rdy.sh); backed up as <bakname>
P=/tmp/claude-0/cplay; S=$1; T="$(cat $2)"; SD="$4"
[ -n "$SD" ] && export SEED=$SD
# (a page may wait first: a tap goes on until the treasure screen or the exploring menu)
for i in $(seq 1 6); do
  TAILN=0 $P/bin/p.sh "$T" SAVE=$S > /dev/null; st=$(grep '? screen' $P/run.log | tail -1)
  echo "$st" | grep -qE "screen (5|0) " && break; T="$T."
done
if echo "$st" | grep -q "screen 5 "; then
  T="$(python3 $P/bin/takeall.py $S "$T" "$SD")"
  T="$T/S/E/N,"
fi
for i in $(seq 1 10); do TAILN=0 $P/bin/p.sh "$T" SAVE=$S > /dev/null
  st=$(grep '? screen' $P/run.log | tail -1); echo "$st" | grep -q "Press button" && { T="$T."; continue; }; break; done
TAILN=0 $P/bin/p.sh "$T" SAVE=$S SAVENOW=$S > /dev/null; unset SEED
echo "$st" | cut -c1-90
$P/bin/fix.sh $S 1 > /dev/null
# (the weapons readied again BEFORE the selling: Quick readies bows, and an unreadied drow sword sells for 0)
$P/bin/rdy.sh $S "$(cat $P/gear.txt)" > /dev/null
python3 $P/bin/loot.py $S 0,1,2,3,4,5,6,7
$P/bin/rdy.sh $S "$(cat $P/gear.txt)" > /dev/null
$P/bin/bk.sh save $S $3 > /dev/null
TAILN=0 $P/bin/p.sh '' SAVE=$S PARTYDUMP=1 > /dev/null; grep -E "^  [0-6]: " $P/run.log | sed -E 's/^  [0-6]: (\S+) +AC (-?[0-9]+) HP ([0-9]+\/[0-9]+).*/\1 \2 \3/' | tr '\n' ';'; echo; grep loaded $P/run.log | head -1
