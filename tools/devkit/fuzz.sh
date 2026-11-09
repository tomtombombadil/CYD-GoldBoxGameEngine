#!/bin/bash
# fuzz.sh <binary> <game> <fights> seeds... - fightfuzz, one line a seed (TAGP=x: a prefix for the run folders - parallel agents)
mkdir -p ${LOGS:-/tmp/claude-0/ag9}
R=$(cd "$(dirname "$0")/../.." && pwd)
bin=$1; g=$2; n=$3; shift 3
for s in "$@"; do
  ( TAG=${TAGP}fz$s timeout 2400 bash $R/tools/devkit/run.sh $g $bin fightfuzz $s $n > ${LOGS:-/tmp/claude-0/ag9}/fz_${g}_$s.log 2>&1
    echo "$g seed $s: exit $? rules $(grep -c '!! RULE' ${LOGS:-/tmp/claude-0/ag9}/fz_${g}_$s.log) asan $(grep -c 'ERROR: AddressSanitizer\|runtime error' ${LOGS:-/tmp/claude-0/ag9}/fz_${g}_$s.log) | $(tail -1 ${LOGS:-/tmp/claude-0/ag9}/fz_${g}_$s.log | cut -c1-150)" ) &
  while [ "$(jobs -r | wc -l)" -ge 3 ]; do wait -n; done
done
wait
