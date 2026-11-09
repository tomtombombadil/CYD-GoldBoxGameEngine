#!/bin/bash
# beh.sh <binary> <mode> <ONLY> <tag> games... - behave <mode> ONLY=... on each game (one at a time)
mkdir -p ${LOGS:-/tmp/claude-0/ag9}
R=$(cd "$(dirname "$0")/../.." && pwd)
bin=$1; mode=$2; only=$3; tag=$4; shift 4
for s in "$@"; do
  if [ -n "$only" ]; then export ONLY=$only; else unset ONLY; fi
  TAG=$tag timeout 2400 bash $R/tools/devkit/run.sh $s $bin behave $mode > ${LOGS:-/tmp/claude-0/ag9}/${tag}_$s.log 2>&1
  echo "$s: $(grep '== behaviour' ${LOGS:-/tmp/claude-0/ag9}/${tag}_$s.log)"
  grep "FAIL" ${LOGS:-/tmp/claude-0/ag9}/${tag}_$s.log | head -5 | cut -c1-300
done
