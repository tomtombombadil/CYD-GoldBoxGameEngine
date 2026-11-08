#!/bin/bash
# behave rules4 on every game (2 at a time); logs in /tmp/claude-0/ag7/logs/r4_<game>.log
# Usage: r4all.sh [games...]
L=/tmp/claude-0/ag7/logs; mkdir -p $L
games=${*:-"poolrad curse secret darkness champions knights queen gateway treasures"}
for s in $games; do
  ( TAG=r4 timeout 1800 bash /tmp/claude-0/ag7/run.sh $s /tmp/claude-0/ag7/bin/r4 behave rules4 > $L/r4_$s.log 2>&1
    echo "$s: $(grep '== behaviour' $L/r4_$s.log) $(grep '== .* problems' $L/r4_$s.log)" ) &
  while [ "$(jobs -r | wc -l)" -ge 2 ]; do wait -n; done
done
wait
