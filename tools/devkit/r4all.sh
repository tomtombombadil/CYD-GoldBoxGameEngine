#!/bin/bash
# behave rules4 on every game (2 at a time); logs in $LOGS (default /tmp/claude-0/ag7/logs)/r4_<game>.log
# Usage: [BIN=bin/r4] [LOGS=dir] [TAGP=x] r4all.sh [games...]   (BIN: build.sh r4 ... - README; TAGP: parallel agents)
L=${LOGS:-/tmp/claude-0/ag7/logs}; mkdir -p $L
R=$(cd "$(dirname "$0")/../.." && pwd)
B=${BIN:-/tmp/claude-0/ag7/bin/r4}
games=${*:-"poolrad curse secret darkness champions knights queen gateway treasures"}
for s in $games; do
  ( TAG=${TAGP}r4 timeout 1800 bash "$R/tools/devkit/run.sh" $s "$B" behave rules4 > $L/r4_$s.log 2>&1
    echo "$s: $(grep '== behaviour' $L/r4_$s.log) $(grep '== .* problems' $L/r4_$s.log)" ) &
  while [ "$(jobs -r | wc -l)" -ge 2 ]; do wait -n; done
done
wait
