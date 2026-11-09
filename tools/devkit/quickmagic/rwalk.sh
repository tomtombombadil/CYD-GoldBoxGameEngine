#!/bin/bash
# rwalk.sh <short> <tries> <len>: random walks from save A until a fight starts in the playsim
s=$1; n=$2; len=$3
for i in $(seq 1 $n); do
  t=$(python3 -c "import random;random.seed($i);print('.'+''.join(random.choice('FFFFFLR.') for _ in range($len)))")
  out=$(TAG=rw$s SAVE=A bash "$(cd "$(dirname "$0")/.." && pwd)/run.sh" $s ${SIM:-/tmp/claude-0/ag7/bin/sim} "$t" 2>&1)
  if echo "$out" | grep -q "a fight:"; then echo "SEED $i: $t"; echo "$out" | grep -B3 "a fight:" | head -8; break; fi
done
