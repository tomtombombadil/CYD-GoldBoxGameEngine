#!/bin/bash
# gostep.sh <slot> <x,y> [answer] [first tokens]: walk to a square beside x,y (goto.sh), face it, step in, then adv.sh
# (texts / questions / treasure / fights) - the event as a player meets it
P=/tmp/claude-0/knplay; S=$1; X=${2%,*}; Y=${2#*,}; Q=${3:-Y}; PRE="$4"
for n in "$X,$((Y+1)) N" "$((X-1)),$Y E" "$X,$((Y-1)) S" "$((X+1)),$Y W"; do
  set -- $n; nx=${1%,*}; ny=${1#*,}; want=$2
  [ $nx -lt 0 -o $ny -lt 0 -o $nx -gt 15 -o $ny -gt 15 ] && continue
  timeout 1500 $P/bin/goto.sh $S $1 MAPLOCKED=1 MAPSECRET=1 $GOENV > /tmp/claude-0/knplay/gostep.log 2>&1
  TAILN=0 $P/bin/p.sh "" SAVE=$S > /dev/null
  grep -q "loaded SAVGAM.*: area [0-9]*, $nx,$ny," $P/run.log || continue
  for t in "" L R LL; do
    TAILN=0 $P/bin/p.sh "${t}F" SAVE=$S > /dev/null
    if grep -q "   now $X,$Y $want" $P/run.log; then TXTN=${TXTN:-20} $P/bin/adv.sh $S x "${PRE}${t}F" $Q; exit 0; fi
  done
done
echo "no way into $X,$Y"
