#!/bin/bash
# demos.sh <playsim binary> <game>... - each game's demo in the playsim (the
# version line's Demo, run until the title comes back - sim.cpp DEMO=1), its
# pages logged (PAGELOG); one line a game: pages and how it ended. A demo that
# works ends "screen 27 ... title 1" (PoolRad has no demo). Logs in
# ${LOGS:-/tmp/claude-0/ag9}/demo_<game>.log. Extra env: DEMOFST=1 (the fight's
# states), PARTYDUMP=1 (each member's weapon dice, class flags, control at the
# fight's start), DEMOSHOTS=1 (a snapshot every 3 s into the run folder's out/).
mkdir -p ${LOGS:-/tmp/claude-0/ag9}
R=$(cd "$(dirname "$0")/../.." && pwd)
bin=$1; shift
for g in "$@"; do
  ( TAG=${TAGP}demo DEMO=1 PAGELOG=1 timeout 900 bash $R/tools/devkit/run.sh $g $bin sim > ${LOGS:-/tmp/claude-0/ag9}/demo_$g.log 2>&1 ) &
  while [ "$(jobs -r | wc -l)" -ge 3 ]; do wait -n; done
done
wait
for g in "$@"; do
  L=${LOGS:-/tmp/claude-0/ag9}/demo_$g.log
  echo "$g: $(grep -c '^\[page' $L) pages | $(grep 'demo over' $L) $(grep -c 'error\|not there\|no monster' $L) errors"
done
