#!/bin/bash
# The flash check on every game (Tom, 2026-11-10: messages too quick to
# read). Usage: flashcheck.sh [seed] [actions]  (READTIME=0: the originals'
# timing - lists the places Readable Messages fixes; default: must be 0)
# Build first: tools/devkit/build.sh flash "-DCYD_TEST_FLASH" tools/playsim/sweep.cpp
D=$(cd "$(dirname "$0")" && pwd)
seed=${1:-1}; n=${2:-1500}
for g in poolrad curse secret darkness gateway treasures champions knights queen; do
  echo "== $g"
  TAG=fl$g NMODIFY=${NMODIFY-30} MCAMP=${MCAMP-4} NCHEAT=${NCHEAT-3} timeout ${TMO-1500} \
    bash "$D/run.sh" $g /tmp/claude-0/ag7/bin/flash flashcheck "$seed" "$n" 2>/dev/null | grep -E "FLASH|flashcheck"
done
