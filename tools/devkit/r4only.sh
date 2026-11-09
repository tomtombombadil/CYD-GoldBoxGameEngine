#!/bin/bash
# r4only.sh <binary> <ONLY> <tag> games...  - behave rules4 ONLY=<ONLY> on each game, one summary line each
mkdir -p ${LOGS:-/tmp/claude-0/ag9}
R=$(cd "$(dirname "$0")/../.." && pwd)
bin=$1; only=$2; tag=$3; shift 3
for s in "$@"; do
  ONLY=$only TAG=$tag timeout 900 bash $R/tools/devkit/run.sh $s $bin behave rules4 > ${LOGS:-/tmp/claude-0/ag9}/${tag}_$s.log 2>&1
  echo "$s: $(grep -c '^ *ok\|PASS' ${LOGS:-/tmp/claude-0/ag9}/${tag}_$s.log) | $(grep '== behaviour' ${LOGS:-/tmp/claude-0/ag9}/${tag}_$s.log) $(grep 'FAIL\|!!' ${LOGS:-/tmp/claude-0/ag9}/${tag}_$s.log | head -3)"
done
