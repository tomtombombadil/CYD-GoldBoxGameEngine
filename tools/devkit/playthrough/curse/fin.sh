#!/bin/bash
# fin.sh <slot> <tokenfile> <bakname> [seed]: a won fight's treasure shared and left, saved, healed (fix.sh),
# the fighters' weapons readied again (rdy.sh), backed up as <bakname>
P=/tmp/claude-0/cplay; S=$1; T="$(cat $2)"
[ -n "$4" ] && export SEED=$4
TAILN=0 $P/bin/p.sh "$T" SAVE=$S > /dev/null
# (no treasure: the fight's last page waits - a tap goes on; treasure: share and leave it)
if grep -q "? screen 5 " <(grep "? screen" $P/run.log | tail -1); then T="$T/S/E/N,"; else T="$T."; fi
[ -n "$4" ] && export SEED=$4
for i in $(seq 1 10); do TAILN=0 $P/bin/p.sh "$T" SAVE=$S > /dev/null
  st=$(grep '? screen' $P/run.log | tail -1); echo "$st" | grep -q "Press button" && { T="$T."; continue; }; break; done
TAILN=0 $P/bin/p.sh "$T" SAVE=$S SAVENOW=$S > /dev/null; unset SEED
echo "$st" | cut -c1-90
$P/bin/fix.sh $S 1 > /dev/null
$P/bin/rdy.sh $S "0:Plate Mail,Shield,Drow Long Sword;1:Plate Mail,Shield,Drow Long Sword;2:Plate Mail,Shield,Drow Long Sword,Ioun Stone Deep Red;4:Plate Mail,Shield,Flail" > /dev/null
$P/bin/bk.sh save $S $3 > /dev/null
TAILN=0 $P/bin/p.sh '' SAVE=$S PARTYDUMP=1 > /dev/null; grep -E "^  [0-6]: " $P/run.log | sed -E 's/^  [0-6]: (\S+) +AC (-?[0-9]+) HP ([0-9]+\/[0-9]+).*/\1 \2 \3/' | tr '\n' ';'; echo; grep loaded $P/run.log | head -1
