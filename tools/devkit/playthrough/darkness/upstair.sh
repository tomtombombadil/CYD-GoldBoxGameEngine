#!/bin/bash
# upstair.sh <slot>: Darkness's Red Tower - from where the party stands, every wall-type-2 side (stairs, table 989F)
# reached (MAPWRAP) is tried; the first that says "THE PATH GOES UP" is taken (Go on? Yes) and saved. Prints the level.
P=/tmp/claude-0/dplay; S=$1
$P/bin/bk.sh save $S _up_pre >/dev/null
sides=""
for d in 0 2 4 6; do TAILN=0 $P/bin/p.sh "" SAVE=$S MAPTYPES=$d >/dev/null
  sides="$sides $(sed -n '/MAPTYPES/,+16p' $P/run.log | sed 1d | python3 -c "
import sys
n={'0':'N','2':'E','4':'S','6':'W'}['$d']
for l in sys.stdin:
  if ':' not in l: continue
  y,r=l.split(':')
  for x,c in enumerate(r.split()):
    if c=='2': print('%d,%d:%s'%(x,int(y),n))
")"; done
for sd in $sides; do sq=${sd%:*}; dir=${sd#*:}
  $P/bin/bk.sh load $S _up_pre >/dev/null
  GOENV=MAPWRAP=1 WAYANS=${WAYANS:-N} timeout 1500 $P/bin/gostepy.sh $S $sq N >/dev/null 2>&1
  TAILN=0 $P/bin/p.sh "" SAVE=$S >/dev/null
  at=$(grep -o "loaded SAV[^,]*, [0-9]*,[0-9]*" $P/run.log | grep -o "[0-9]*,[0-9]*$"); [ "$at" != "$sq" ] && continue
  f=$(bash $P/bin/face.sh $S $dir); TAILN=0 $P/bin/p.sh "${f}F" SAVE=$S ALLTEXT=1 >/dev/null
  if grep -q "PATH GOES ${WANT:-UP}" $P/run.log; then
    TXTN=6 $P/bin/adv.sh $S x "${f}F" Y | tail -6; echo "took $sd"; exit 0; fi
done
$P/bin/bk.sh load $S _up_pre >/dev/null; echo "no ${WANT:-UP} stairs reached"; exit 1
