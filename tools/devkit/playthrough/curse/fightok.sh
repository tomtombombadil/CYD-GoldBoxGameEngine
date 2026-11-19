#!/bin/bash
# fightok.sh <slot> <start bak> <tokens file> <bakname> <seeds...>: drv3 on each seed from the start; a win is
# finished (fin.sh) and kept only when no member is Gone (status 8 - disintegrated, beyond any cure); else the next seed
P=/tmp/claude-0/cplay; S=$1; B=$2; TF=$3; N=$4; shift 4
for s in "$@"; do
  $P/bin/bk.sh load $S $B > /dev/null
  o=$(timeout 3000 python3 $P/bin/drv3.py $S "$(cat $TF)" $s 2>&1 | tail -2 | tr '\n' ' ')
  echo "seed $s: ${o:0:120}"
  echo "$o" | grep -q "^WON" || continue
  cp $P/ftok.txt $P/ft_$N.txt
  $P/bin/fin.sh $S $P/ft_$N.txt ${N}_won $s > /dev/null 2>&1
  TAILN=0 $P/bin/p.sh "" SAVE=$S CAMPRUN='!' > /dev/null
  if grep -a " HP .* st 8 " $P/run.log; then echo "  someone Gone - next seed"; continue; fi
  echo "KEPT seed $s"; exit 0
done
exit 1
