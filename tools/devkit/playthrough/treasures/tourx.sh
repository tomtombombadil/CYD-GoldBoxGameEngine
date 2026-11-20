#!/bin/bash
# tourx.sh "x,y x,y ...": go.sh to each square in turn from slot J, a log line each
P=/tmp/claude-0/tplay
export AUTOLOOT=${AUTOLOOT-1} AUTOTAKE=${AUTOTAKE-/S/T/I}
for g in $1; do
  echo "== to $g" | tee -a $P/transcript.txt
  AUTOPICK="${AUTOPICK:-Attack,Fight,Take,Yes}" $P/bin/go.sh J "|B" $g 2>&1 | grep -v "^  TEXT:  *%" | tail -${TL:-7} | cut -c1-200
  if [ "$(cat $P/tok.txt)" != "|B" ]; then TAILN=0 $P/bin/p.sh "$(cat $P/tok.txt)" SAVE=$(cat $P/slot.txt) SAVENOW=J >/dev/null; grep -aE "TEXT|MENU prompt|^\[fight\] won|a fight:|JOURNAL|LOAD MONSTER|script .* loaded|LOOT take" $P/run.log | uniq | cut -c1-400 >> $P/transcript.txt; fi
  TAILN=0 $P/bin/p.sh "|B" SAVE=J PARTYDUMP=1 >/dev/null; grep "loaded SAVGAMJ" $P/run.log | cut -c1-80; grep -E "^  [0-7]: " $P/run.log | awk '{printf "%s %s/%s  ",$2,$6,""}' ; echo
done
