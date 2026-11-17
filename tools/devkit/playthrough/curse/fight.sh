#!/bin/bash
# fight.sh <slot> <tokens to a fight> <bakname>: the fight played by drv3.py (seeds "" 2 3 4), then finished by
# fin.sh (items taken, sold / identified, the party healed); the slot saved, backed up as <bakname>
P=/tmp/claude-0/cplay; S=$1; T=$2; B=$3
$P/bin/bk.sh save $S ${B}_pre > /dev/null
for s in "" 2 3 4; do $P/bin/bk.sh load $S ${B}_pre > /dev/null
  o=$(timeout 3000 python3 $P/bin/drv3.py $S "$T" $s 2>/dev/null | grep -E "^(WON|OUT)" | head -1)
  echo "  fight (seed '$s'): ${o:0:90}"
  if echo "$o" | grep -q WON; then cp $P/ftok.txt $P/ft_$B.txt; $P/bin/fin.sh $S $P/ft_$B.txt $B "$s"; exit 0; fi
done
echo "LOST every seed"; $P/bin/bk.sh load $S ${B}_pre > /dev/null; exit 1
