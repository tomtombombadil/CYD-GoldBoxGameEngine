#!/bin/bash
# leave.sh X,Y : walk to the place's entry square, find the step that asks to leave, answer Yes, decline the
# overland's "enter?" and save on the overland
P=/tmp/claude-0/knplay; cd $P
for k in 1 2 3 4 5 6 7 8; do
  TAILN=0 bash bin/p.sh "" SAVE=C >/dev/null; grep -q "now ${1} " run.log && break
  EXTRA="QASPELLS=1" PICK="${PICK:-Disarm,Attack}" WN=10 timeout 1500 bash bin/walkans.sh C $1 > w.out 2>&1; bash bin/fix.sh C >/dev/null 2>&1
done; T=""
for s in F RF LF RRF; do
  for a in "/N" "/Y./N" "/Y/N"; do
    TAILN=0 bash bin/p.sh "$T$s$a" SAVE=C ALLTEXT=1 >/dev/null
    if grep -q "Area 1, script 17" run.log && grep -E "screen" run.log | tail -1 | grep -q "Move Encamp"; then
      TAILN=0 bash bin/p.sh "$T$s$a" SAVE=C SAVENOW=C >/dev/null; grep "saved now" run.log; exit 0; fi
  done
done
echo "no way out found"; grep -E "screen|TEXT" run.log | tail -3
