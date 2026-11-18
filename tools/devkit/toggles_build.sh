#!/bin/bash
# The toggle checks' binaries: one a game (tg<short>) - the game's own behave
# parts (behave.sh's flags) + the common suites. (One binary with every part
# doesn't do: BEHAVE_TREASURES turns Darkness's part into Treasures', and other
# parts' hooks change the common suites' fights.)
D=$(cd "$(dirname "$0")" && pwd)
H="-DCYD_TEST_HOOKS -DBEHAVE_MATRIX -DBEHAVE_ROUND4 -DBEHAVE_LEVELS -DBEHAVE_ECONOMY -DBEHAVE_FEATURES -DBEHAVE_TOGGLES"
declare -A FLAGS=(
  [curse]="-DBEHAVE_CLASSES -DBEHAVE_ITEMS -DBEHAVE_SPELLS -DBEHAVE_MONSTERS -DBEHAVE_EVENTS -DBEHAVE_SAVES"
  [poolrad]="-DBEHAVE_POOL -DBEHAVE_SPELLS" [secret]="-DBEHAVE_SECRET" [darkness]="-DBEHAVE_DARKNESS"
  [treasures]="-DBEHAVE_TREASURES" [champions]="-DBEHAVE_CHAMPIONS" [knights]="-DBEHAVE_DEATHKNIGHTS"
  [gateway]="-DBEHAVE_GATEWAY" [queen]="-DBEHAVE_QUEEN")
for g in ${*:-poolrad curse secret darkness champions knights queen gateway treasures}; do
  bash "$D/build.sh" tg$g "$H ${FLAGS[$g]}" tools/playsim/sweep.cpp > /tmp/claude-0/tg_build_$g.log 2>&1
  tail -1 /tmp/claude-0/tg_build_$g.log
done
