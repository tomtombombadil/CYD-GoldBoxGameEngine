#!/bin/bash
# round 10's verification batch (the scripts it calls are tools/devkit's)
R=$(cd "$(dirname "$0")/../.." && pwd)
export LOGS=${LOGS:-/tmp/claude-0/ag10}; mkdir -p $LOGS
cd $R
REBUILD=1 bash tools/devkit/build.sh eco10 "-fsanitize=address,undefined -DCYD_TEST_HOOKS -DBEHAVE_ECONOMY" tools/playsim/sweep.cpp > $LOGS/b_eco.log 2>&1
REBUILD=1 LDEXTRA="-Wl,--wrap=malloc,--wrap=free,--wrap=calloc,--wrap=realloc" bash tools/devkit/build.sh sim10 "-fno-omit-frame-pointer" tools/playsim/sim.cpp tools/playsim/heapprof.cpp > $LOGS/b_sim.log 2>&1
echo "== rules4"
bash tools/devkit/r4full.sh /tmp/claude-0/ag7/bin/r410 r4g poolrad curse secret darkness champions knights queen gateway treasures
echo "== economy"
bash tools/devkit/beh.sh /tmp/claude-0/ag7/bin/eco10 economy "" ecoall poolrad curse secret darkness champions knights queen gateway treasures
echo "== budgets"
SIMBIN=/tmp/claude-0/ag7/bin/sim10 bash tools/devkit/budgetx.sh
echo "== fuzz"
bash tools/devkit/fuzz.sh /tmp/claude-0/ag7/bin/r410 champions 30 1 2 3
bash tools/devkit/fuzz.sh /tmp/claude-0/ag7/bin/r410 queen 30 13 4 5
bash tools/devkit/fuzz.sh /tmp/claude-0/ag7/bin/r410 knights 30 1 2
echo "== saveload"
for g in champions knights queen; do TAG=sl timeout 1200 bash tools/devkit/run.sh $g /tmp/claude-0/ag7/bin/r410 saveload > $LOGS/sl_$g.log 2>&1; echo "$g: $(tail -1 $LOGS/sl_$g.log)"; done
echo "== done"
