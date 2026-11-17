#!/bin/bash
# goal.sh <slot> <x,y> <bakname>: walk there (walkf.sh); a fight on the way is played by drv2.py (seeds
# "" 2 3 4) and finished by fin.sh (backed up as <bakname>_f), then the walk goes on; stops at any other
# menu (tokens in tok.txt) or the goal (saved in the slot, backed up as <bakname>)
P=/tmp/claude-0/cplay; S=$1; G=$2; B=$3
$P/bin/bk.sh save $S ${B}_start > /dev/null
for k in 1 2 3 4 5; do
  r=$(timeout 2400 $P/bin/walkf.sh $S "" $G ALLTEXT=1 | tail -2 | head -1)
  T=$(cat $P/tok.txt)
  if echo "$r" | grep -q "screen 24"; then
    won=""
    for s in "" 2 3 4; do $P/bin/bk.sh load $S ${B}_start > /dev/null
      o=$(timeout 3000 python3 $P/bin/drv3.py $S "$T" $s 2>/dev/null | grep -E "^(WON|OUT)" | head -1)
      echo "  fight (seed '$s'): ${o:0:70}"
      if echo "$o" | grep -q WON; then cp $P/ftok.txt $P/ft_${B}_$k.txt; $P/bin/fin.sh $S $P/ft_${B}_$k.txt ${B}_start "$s" > /dev/null; won=1; break; fi
    done
    [ -z "$won" ] && { echo "LOST every seed"; $P/bin/bk.sh load $S ${B}_start > /dev/null; exit 1; }
    continue
  fi
  echo "$r" | grep -q "AT GOAL" && { TAILN=0 $P/bin/p.sh "$T" SAVE=$S SAVENOW=$S > /dev/null; $P/bin/bk.sh save $S $B > /dev/null; echo "AT GOAL $G"; exit 0; }
  echo "$r"; exit 2
done
