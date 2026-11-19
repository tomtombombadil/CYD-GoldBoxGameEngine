#!/bin/bash
# push.sh <slot> <x,y> <name>: walk there through locked / secret doors; each fight on the way played by fightok.sh
# (seeds 1-10, a win with nobody Gone), healed and saved; stops at the goal or any other menu (tokens in tok.txt)
P=/tmp/claude-0/cplay; S=$1; G=$2; N=$3
for k in $(seq 1 12); do
  $P/bin/bk.sh save $S ${N}_$k > /dev/null
  r=$(timeout 2400 $P/bin/walkf.sh $S "" $G ALLTEXT=1 MAPLOCKED=1 MAPSECRET=1 | tail -2 | head -1)
  if echo "$r" | grep -q "screen 24"; then
    cp $P/tok.txt $P/push_tok.txt
    grep -a "TEXT" $P/run.log | tail -2 | cut -c1-80
    $P/bin/fightok.sh $S ${N}_$k $P/push_tok.txt ${N}_f$k 1 2 3 4 5 6 7 8 9 10 | tail -1 || { echo "LOST"; exit 1; }
    continue
  fi
  echo "$r" | grep -q "AT GOAL" && { echo "AT GOAL $G"; exit 0; }
  echo "$r"; exit 2
done
