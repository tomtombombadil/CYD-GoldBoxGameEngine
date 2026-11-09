#!/bin/bash
# qa_verify.sh <sweep binary (CYD_TEST_HOOKS)> [games...] - Quick Actions' checks (Tom, 2026-11-09):
#   1. fightfuzz with random grids (QAFUZZ=1: each member's 8 switches at random, half the fights all
#      on Quick; fight_check R13) - 3 seeds x 40 fights a game: rule breaks, sanitizer errors
#   2. the defaults against the switch off (QAOFF=1): the same seed's fightfuzz action logs
#      (FCACTLOG=1, every action) must be identical - 2 seeds x 25 fights a game
# Logs in $LOGS (default /tmp/claude-0/agq/qv); TAGP=qv run folders.
L=${LOGS:-/tmp/claude-0/agq/qv}
mkdir -p "$L"
R=$(cd "$(dirname "$0")/../.." && pwd)
bin=$1; shift
games=${*:-poolrad curse secret darkness gateway treasures champions knights queen}
for g in $games; do
  QAFUZZ=1 TAGP=qv LOGS=$L bash "$R/tools/devkit/fuzz.sh" "$bin" "$g" 40 1 2 3
  echo "$g R13: $(grep -h 'fight checks' $L/fz_${g}_*.log | sed 's/.*ticks, //' | tr '\n' ' ')"
done
for g in $games; do
  for s in 4 5; do
    for v in on off; do
      if [ $v = off ]; then export QAOFF=1; else unset QAOFF; fi
      FCACTLOG=1 FCACTN=100000000 TAG=qv$v$s timeout 2400 bash "$R/tools/devkit/run.sh" "$g" "$bin" fightfuzz $s 25 \
        2>/dev/null | grep '^  \[act ' > "$L/acts_${g}_${s}_$v.txt"
    done
    unset QAOFF
    if cmp -s "$L/acts_${g}_${s}_on.txt" "$L/acts_${g}_${s}_off.txt"; then r=identical; else r=DIFFERENT; fi
    echo "$g seed $s: defaults vs off - $(wc -l < $L/acts_${g}_${s}_on.txt) actions, $r"
  done
done
