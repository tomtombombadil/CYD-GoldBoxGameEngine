#!/bin/bash
# go2.sh <slot> <x,y> [env]: Champions kit - goto.sh (doors: MAPLOCKED / MAPSECRET), each event on the way played by
# adv.sh (answer GOANS2, default Y; PICK from the environment), post.sh after a fight; until the square is reached
P=/tmp/claude-0/knplay; S=$1; G=$2; shift 2
for k in $(seq 1 ${GON:-10}); do
  o=$(timeout 2400 $P/bin/goto.sh $S $G MAPLOCKED=1 MAPSECRET=1 ${GOENV} "$@" 2>&1)
  echo "$o" | grep -E "fight on the way|JOURNAL|TEXT" | cut -c1-110
  echo "$o" | grep -q "fight on the way" && $P/bin/post.sh $S > /dev/null
  if echo "$o" | grep -q "not saved: a menu waits"; then
    a=$(TXTN=30 ITER=14 timeout 2400 $P/bin/adv.sh $S x "$(cat $P/tok.txt)" ${GOANS2:-Y} 2>&1)
    echo "$a" | grep -E "TEXT|JOURNAL|fight\]" | cut -c1-110
    echo "$a" | grep -q "fight\]" && $P/bin/post.sh $S > /dev/null
    if ! echo "$a" | grep -q "saved now" && grep -E "^  \? screen 1 " $P/run.log | tail -1 | grep -q "screen 1 "; then
      # (the party fell in a story fight on Auto: once more with WINFIGHT - every foe down, the after-fight part runs)
      echo "  LOST on Auto - WINFIGHT"
      a=$(EXTRA="$EXTRA WINFIGHT=1" TXTN=30 ITER=14 timeout 2400 $P/bin/adv.sh $S x "$(cat $P/tok.txt)" ${GOANS2:-Y} 2>&1)
      echo "$a" | grep -E "TEXT|JOURNAL|fight\]" | cut -c1-110
    fi
    echo "$a" | grep -q "saved now" || { echo "  STOPPED: $(echo "$a" | tail -2 | head -1 | cut -c1-120)"; break; }
    continue
  fi
  echo "$o" | grep -q "AT GOAL" && { echo "  at $G"; break; }
  echo "$o" | grep -q "saved now" && ! echo "$o" | grep -q "fight on the way" && { echo "  at $G (saved)"; break; }
done
find $P/card/out -name '*.ppm' -delete
TAILN=0 $P/bin/p.sh "" SAVE=$S > /dev/null; grep "   now" $P/run.log | tail -1
