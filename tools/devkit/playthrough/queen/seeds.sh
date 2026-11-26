#!/bin/bash
# seeds.sh <slot> <tokens> <seeds...>: a fight tried on each seed (auto.sh, Quick, QASPELLS=1) - won? how many dead
P=/tmp/claude-0/qplay; S=$1; T="$2"; shift 2
for sd in "$@"; do
  PTIME=${PTIME:-2400} AT=1500 TN=0 $P/bin/auto.sh "$T" SAVE=$S SEED=$sd QASPELLS=1 PAGELOG=1 ALLTEXT=1 $EXTRA > /dev/null
  echo "seed $sd: $(grep -m1 'won:' $P/run.log | cut -c1-90) killed $(grep -c 'is killed\|is Gone\|swallow' $P/run.log | head -1) $(grep '? screen' $P/run.log | tail -1 | cut -c1-40)"
done
