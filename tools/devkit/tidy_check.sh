#!/bin/bash
# tidy_check.sh <game>... - Clean Up Game Files against what each game really
# reads: the PC platform's FILELOG over an events sweep (every script block),
# the demo, 12 fuzzed fights and the music render, then tidy_check.py: no file
# read may be on tidy_run's removal list for the same game folder.
# Needs bin/tsim (build.sh tsim "" tools/playsim/sim.cpp), bin/tsw (build.sh
# tsw "-DCYD_TEST_HOOKS" tools/playsim/sweep.cpp) and the GOG folders in
# /tmp/claude-0/games (run.sh). Logs: ${LOGS:-/tmp/claude-0/clean}/fl.
R=$(cd "$(dirname "$0")/../.." && pwd); B=/tmp/claude-0/ag7/bin; L=${LOGS:-/tmp/claude-0/clean}/fl
mkdir -p $L
g++ -std=c++17 -O1 -Itools/host -Isrc -o $B/tidy_run $R/tools/devkit/src/tidy_run.cpp $R/src/game/tidy.cpp \
    $R/src/engine/games.cpp $R/tools/host/*.cpp $R/src/engine/dax.cpp || exit 1
G=$(dirname $L)/gog; rm -rf $G; mkdir -p $G
for g in "$@"; do
  cp -r /tmp/claude-0/games/$g/* $G/
  rm -f $L/$g.*.log
  FILELOG=$L/$g.ev.log TAG=cv$g timeout 3000 bash $R/tools/devkit/run.sh $g $B/tsw events > $L/$g.ev.out 2>&1
  FILELOG=$L/$g.demo.log TAG=cv$g DEMO=1 timeout 600 bash $R/tools/devkit/run.sh $g $B/tsim sim > $L/$g.demo.out 2>&1
  FILELOG=$L/$g.fz.log TAG=cv$g timeout 900 bash $R/tools/devkit/run.sh $g $B/tsw fightfuzz 3 12 > $L/$g.fz.out 2>&1
  FILELOG=$L/$g.mus.log TAG=cv$g MAKEMUSIC=1 CACHE=/GOLDBOX/_CYD timeout 900 bash $R/tools/devkit/run.sh $g $B/tsim sim > $L/$g.mus.out 2>&1
done
$B/tidy_run $G > $(dirname $L)/dry_gog.txt
python3 $R/tools/devkit/tidy_check.py $(dirname $L) "$@"
