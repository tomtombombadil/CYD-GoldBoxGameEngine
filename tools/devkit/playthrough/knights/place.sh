#!/bin/bash
# place.sh X,Y TAG [PICK] : overland to X,Y, enter, list its events, walk each event square (walkans), log to TAG.out
P=/tmp/claude-0/knplay; cd $P
timeout 3500 bash bin/kovgo.sh C $1 > $2.go 2>&1; tail -2 $2.go
bash bin/bk.sh save C kn_$2_0 >/dev/null
bash bin/events.sh C >/dev/null 2>&1; cp ev_dis.txt ev_$2.txt
TAILN=0 bash bin/p.sh "" SAVE=C MAPFLAGS=1 >/dev/null; python3 bin/evsq.py > $2.sq
grep "loaded" run.log | head -1
cat $2.sq | tr '\n' ' '; echo
for g in $(awk '{for(i=2;i<=NF;i++) print $i}' $2.sq); do
  r=$(EXTRA="QASPELLS=1" PICK="${3:-Talk,Attack}" WN=10 TXTN=30 timeout 1500 bash bin/walkans.sh C $g 2>&1 | grep -E "TEXT|fight\]|JOURNAL|no way" | cut -c1-120 | tail -6)
  echo "== $g"; echo "$r"
  bash bin/fix.sh C >/dev/null 2>&1
done > $2.out
