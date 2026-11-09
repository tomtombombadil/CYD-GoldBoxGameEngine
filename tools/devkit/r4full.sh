#!/bin/bash
# r4full.sh <binary> <tag> games... - full behave rules4, 3 at a time
mkdir -p ${LOGS:-/tmp/claude-0/ag9}
R=$(cd "$(dirname "$0")/../.." && pwd)
bin=$1; tag=$2; shift 2
for s in "$@"; do
  ( TAG=$tag timeout 2400 bash $R/tools/devkit/run.sh $s $bin behave rules4 > ${LOGS:-/tmp/claude-0/ag9}/${tag}_$s.log 2>&1
    echo "$s: $(grep '== behaviour' ${LOGS:-/tmp/claude-0/ag9}/${tag}_$s.log) $(grep -c '!! RULE' ${LOGS:-/tmp/claude-0/ag9}/${tag}_$s.log) rule breaks" ) &
  while [ "$(jobs -r | wc -l)" -ge 3 ]; do wait -n; done
done
wait
