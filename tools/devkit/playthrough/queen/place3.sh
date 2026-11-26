#!/bin/bash
# place3.sh TAG [PICK]: walk each event square of TAG.sq (already made) with walkans, log to TAG.out
P=/tmp/claude-0/qplay; cd $P
for g in $(awk '{for(i=2;i<=NF;i++) print $i}' $1.sq); do
  r=$(EXTRA="QASPELLS=1" PICK="${2:-Talk,Attack}" NOANS="${NOANS:-LEAVE\|CLIMB OUT}" WN=10 TXTN=30 timeout 1500 bash bin/walkans.sh ${SLOT:-B} $g 2>&1 | grep -E "TEXT|fight\]|JOURNAL|no way" | cut -c1-120 | tail -6)
  echo "== $g"; echo "$r"
  bash bin/fix.sh ${SLOT:-B} >/dev/null 2>&1
done > $1.out
