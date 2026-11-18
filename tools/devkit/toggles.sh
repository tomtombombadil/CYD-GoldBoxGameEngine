#!/bin/bash
# The toggle checks (Tom, 2026-11-17: "check each toggle and verify its effect
# in each game" before a release). Runs every behave suite of each game with
# TOGGLEMAP=1 (each check line ends with the switches the engine asked while it
# ran) and the switches held off by FIXOFF / ADNDOFF / ENHOFF (pass-through env,
# hex masks or "all"). A run with them all off against one with none off says
# which checks each switch decides; tools/devkit/toggles.py reads the logs.
# Usage: [BIN=bin] [OUT=dir] [SUITES="..."] toggles.sh <tag> [games...]
# The binary: build.sh tg "-DCYD_TEST_HOOKS -DBEHAVE_<every part>" tools/playsim/sweep.cpp
# (the flags: toggles.py's header). Two runs at a time (2 cores).
R=$(cd "$(dirname "$0")/../.." && pwd)
B=${BIN:-/tmp/claude-0/ag7/bin/tg}   # (per game: tg<short>, toggles_build.sh)
tag=$1; shift
OUT=${OUT:-/tmp/claude-0/tg/logs}
mkdir -p "$OUT"
games=${*:-"poolrad curse secret darkness champions knights queen gateway treasures"}
declare -A PART=([curse]="classes items spells monsters saves" [poolrad]=pool [secret]=secret
  [darkness]=darkness [treasures]=treasures [champions]=champions [knights]=deathknights [gateway]=gateway
  [queen]=queen)
SUITES=${SUITES:-"own matrix levels economy features rules4 toggles"}
for g in $games; do
  for s in $SUITES; do
    parts=$s; [ "$s" = own ] && parts=${PART[$g]}
    for p in $parts; do
      ( ITEMS_BIN=$( [ -f /tmp/claude-0/sw/items_$g.bin ] && echo /tmp/claude-0/sw/items_$g.bin || echo /tmp/claude-0/sw/items_curse.bin ) TOGGLEMAP=1 TAG=tg_${g}_$p timeout ${TMO:-3000} \
          bash "$R/tools/devkit/run.sh" $g "${BIN:-/tmp/claude-0/ag7/bin/tg$g}" behave $p > "$OUT/${tag}_${g}_$p.log" 2>&1
        echo "$tag $g $p: $(grep '^== behaviour' "$OUT/${tag}_${g}_$p.log" | tail -1)" ) &
      while [ "$(jobs -r | wc -l)" -ge ${JOBS:-2} ]; do wait -n; done
    done
  done
done
wait
