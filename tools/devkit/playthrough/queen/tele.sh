#!/bin/bash
# tele.sh SLOT x,y: walk (five tries - a fight on the way ends a walk) to a teleporter square (its question answered No on the way), then Look and Yes - saved
P=/tmp/claude-0/qplay; cd $P
for k in 1 2 3 4 5; do
EXTRA="WINFIGHT=1 $EXTRA" PICK=${PICK:-Pay,Gold,Step} NOANS="LEAVE THE TOMB\|CLIMB UP\|STEP INTO\|APPROACH${NOANS:+\\|$NOANS}" WN=30 timeout 3000 bash bin/walkans.sh $1 $2 > /dev/null 2>&1
grep -q "now $2 " w.out && break; done
grep -q "now $2 " w.out || { echo "not at $2: $(tail -1 w.out)"; exit 1; }
TAILN=0 bin/p.sh "/L/Y" SAVE=$1 ALLTEXT=1 WINFIGHT=1 SAVENOW=$1 >/dev/null; grep "saved now" run.log
