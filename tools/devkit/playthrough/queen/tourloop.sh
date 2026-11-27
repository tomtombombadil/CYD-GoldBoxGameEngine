#!/bin/bash
# tourloop.sh SLOT RAW N: regtour.sh again and again (visited.txt rebuilt from tour.log by vis.sh each time) -
# for a map whose teleporters move the party between regions (the Dark Queen's Tombs); a round per region
P=/tmp/claude-0/qplay; cd $P
for i in $(seq 1 $3); do
  bash bin/vis.sh tour.log
  bash bin/regtour.sh $1 $2 || { echo "round $i: nothing left"; break; }
  echo "round $i: $(grep -c '^== ' tour.log) steps, $(grep 'loaded SAV' run.log | tail -1 | cut -c8-70)"
done
