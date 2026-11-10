#!/bin/bash
# verify18.sh - round 18's whole verification batch (rules work, 2026-11-10): the oracles,
# rebuilds (REBUILD=1), rules4 all nine, saveload all nine, the budgets, qa_verify (fuzz 3 seeds x 40
# with R13, defaults vs the switch off). Summary in $V/summary.txt (default /tmp/claude-0/ag18/v).
# Usage: [V=dir] [TAG=r18] verify18.sh
R=$(cd "$(dirname "$0")/../.." && pwd)
D=$R/tools/devkit
V=${V:-/tmp/claude-0/ag18/v}
T=${TAG:-r18}
mkdir -p "$V"
S=$V/summary.txt
: > "$S"
echo "== oracles $(date)" >> "$S"
O=$D/oracles
G=${GAMES:-/tmp/claude-0/games}
( cd "$O" && python3 guard.py && python3 aistep.py ) >> "$S" 2>&1
python3 "$O/qhooks.py" "$G/queen/The Dark Queen of Krynn/DQK.EXE" >> "$S" 2>&1
for g in poolrad curse secret darkness champions knights gateway treasures; do
  ovr=$(ls "$G/$g"/*/GAME.OVR "$G/$g"/*/*/GAME.OVR 2>/dev/null | head -1)
  img=$(ls /tmp/claude-0/prog/${g}_start.bin /tmp/claude-0/prog/${g}_game.bin 2>/dev/null | head -1)
  python3 "$O/finder.py" "$ovr" "$img" $g >> "$S" 2>&1
  python3 "$O/turnfind.py" "$ovr" "$img" $g >> "$S" 2>&1
done
for g in poolrad curse secret darkness champions knights gateway treasures; do
  ovr=$(ls "$G/$g"/*/GAME.OVR "$G/$g"/*/*/GAME.OVR 2>/dev/null | head -1)
  python3 "$O/onhit.py" "$ovr" $g >> "$S" 2>&1
done
python3 "$O/onhit.py" "$G/queen/The Dark Queen of Krynn/DQK.EXE" queen >> "$S" 2>&1
echo "== build $(date)" >> "$S"
( cd "$R" && REBUILD=1 bash "$D/build.sh" ${T}r4 "-fsanitize=address,undefined -DCYD_TEST_HOOKS -DBEHAVE_MATRIX -DBEHAVE_ROUND4" tools/playsim/sweep.cpp ) > "$V/build_r4.log" 2>&1
( cd "$R" && LDEXTRA="-Wl,--wrap=malloc,--wrap=free,--wrap=calloc,--wrap=realloc" REBUILD=1 bash "$D/build.sh" ${T}simx "-fno-omit-frame-pointer" tools/playsim/sim.cpp tools/playsim/heapprof.cpp ) > "$V/build_sim.log" 2>&1
tail -1 "$V/build_r4.log" >> "$S"; tail -1 "$V/build_sim.log" >> "$S"
BIN=/tmp/claude-0/ag7/bin/${T}r4
echo "== r4all $(date)" >> "$S"
BIN=$BIN LOGS=$V TAGP=$T bash "$D/r4all.sh" >> "$S" 2>&1
echo "== saveload $(date)" >> "$S"
LOGS=$V/sl TAGP=$T bash "$D/saveload.sh" $BIN >> "$S" 2>&1
echo "== budgetx $(date)" >> "$S"
SIMBIN=/tmp/claude-0/ag7/bin/${T}simx LOGS=$V TAGP=$T bash "$D/budgetx.sh" >> "$S" 2>&1
echo "== qa_verify $(date)" >> "$S"
LOGS=$V/qv TAGP=$T bash "$D/qa_verify.sh" $BIN >> "$S" 2>&1
echo "== done $(date)" >> "$S"
