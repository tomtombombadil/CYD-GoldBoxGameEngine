#!/bin/bash
# tour.sh <slot> <x,y x,y ...>: walkans.sh to each square in turn (EXTRA / PICK / NOBASE from the environment);
# a target missed twice is passed over; each reached one saved as bak/tour_<n>; the texts to tour.log;
# STOPRE=pattern ends the tour when a text matches it
P=/tmp/claude-0/qplay; S=$1; shift; n=0
for g in "$@"; do
  for t in 1 2; do
    timeout 3000 bash $P/bin/walkans.sh $S $g > $P/tour_step.txt 2>&1
    echo "== $g try $t: $(tail -1 $P/w.out | cut -c1-40)" >> $P/tour.log
    grep -v "saved now\|? screen" $P/tour_step.txt >> $P/tour.log
    grep -q "now $g " $P/w.out && break
  done
  n=$((n+1)); bash $P/bin/bk.sh save $S tour_$n > /dev/null
  [ -n "$STOPRE" ] && grep -qi "$STOPRE" $P/tour_step.txt && { echo "== STOP at $g" >> $P/tour.log; break; }
done
echo "== tour done" >> $P/tour.log
