#!/bin/bash
# saveload.sh <sweep binary> [games...] - sweep saveload (the save / load round trip) on each game, a line each
L=${LOGS:-/tmp/claude-0/agq/sl}
mkdir -p "$L"
R=$(cd "$(dirname "$0")/../.." && pwd)
bin=$1; shift
for g in ${*:-poolrad curse secret darkness gateway treasures champions knights queen}; do
  TAG=${TAGP}sl timeout 900 bash "$R/tools/devkit/run.sh" "$g" "$bin" saveload > "$L/sl_$g.log" 2>&1
  echo "$g: $(tail -1 "$L/sl_$g.log") $(grep -c 'ERROR: AddressSanitizer\|runtime error' "$L/sl_$g.log") sanitizer errors"
done
