#!/bin/bash
# round 11's verification batch (the scripts it calls are tools/devkit's):
# the oracles it needs (tohit), rules4 / economy on all nine, budgets,
# fightfuzz where fights changed, sweep saveload on all nine
R=$(cd "$(dirname "$0")/../.." && pwd)
export LOGS=${LOGS:-/tmp/claude-0/ag11}; mkdir -p $LOGS
cd $R
python3 -I tools/devkit/oracles/tohit.py > $LOGS/oracle_tohit.log 2>&1
REBUILD=1 bash tools/devkit/build.sh r411 "-fsanitize=address,undefined -DCYD_TEST_HOOKS -DBEHAVE_MATRIX -DBEHAVE_ROUND4" tools/playsim/sweep.cpp > $LOGS/b_r4.log 2>&1
REBUILD=1 bash tools/devkit/build.sh eco11 "-fsanitize=address,undefined -DCYD_TEST_HOOKS -DBEHAVE_ECONOMY" tools/playsim/sweep.cpp > $LOGS/b_eco.log 2>&1
REBUILD=1 LDEXTRA="-Wl,--wrap=malloc,--wrap=free,--wrap=calloc,--wrap=realloc" bash tools/devkit/build.sh sim11 "-fno-omit-frame-pointer" tools/playsim/sim.cpp tools/playsim/heapprof.cpp > $LOGS/b_sim.log 2>&1
echo "== rules4"
bash tools/devkit/r4full.sh /tmp/claude-0/ag7/bin/r411 r4 poolrad curse secret darkness champions knights queen gateway treasures
echo "== economy"
bash tools/devkit/beh.sh /tmp/claude-0/ag7/bin/eco11 economy "" eco poolrad curse secret darkness champions knights queen gateway treasures
echo "== budgets"
SIMBIN=/tmp/claude-0/ag7/bin/sim11 bash tools/devkit/budgetx.sh
echo "== fuzz"
bash tools/devkit/fuzz.sh /tmp/claude-0/ag7/bin/r411 champions 30 1 2 3
bash tools/devkit/fuzz.sh /tmp/claude-0/ag7/bin/r411 queen 30 13 4 5
bash tools/devkit/fuzz.sh /tmp/claude-0/ag7/bin/r411 darkness 30 1 2
bash tools/devkit/fuzz.sh /tmp/claude-0/ag7/bin/r411 treasures 30 1 2
bash tools/devkit/fuzz.sh /tmp/claude-0/ag7/bin/r411 gateway 30 1
bash tools/devkit/fuzz.sh /tmp/claude-0/ag7/bin/r411 knights 30 1
echo "== saveload"
for g in poolrad curse secret darkness champions knights queen gateway treasures; do TAG=sl timeout 1200 bash tools/devkit/run.sh $g /tmp/claude-0/ag7/bin/r411 saveload > $LOGS/sl_$g.log 2>&1; echo "$g: $(tail -1 $LOGS/sl_$g.log)"; done
echo "== done"
