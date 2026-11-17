#!/bin/bash
# the tower's exit -> the overland -> Essembra -> the Standing Stone -> Hillsfar -> Yulash's gate
P=/tmp/claude-0/cplay; cd $P
T="LF/W,"; for i in $(seq 1 6); do TAILN=0 bin/p.sh "$T" SAVE=D > /dev/null; grep '? screen' run.log | tail -1 | grep -q "Press button" && T="$T." || break; done
T="$T/D,"; for i in $(seq 1 6); do TAILN=0 bin/p.sh "$T" SAVE=D > /dev/null; grep '? screen' run.log | tail -1 | grep -q "Press button" && T="$T." || break; done
TAILN=0 bin/p.sh "$T" SAVE=D SAVENOW=D > /dev/null; bin/bk.sh save D over2 > /dev/null; echo "overland: $(grep '^   now' run.log | tail -1)"
PRE="" bin/journey.sh D 0; bin/bk.sh save D ess2 > /dev/null; echo "essembra"
PRE="" bin/journey.sh D 1; bin/bk.sh save D stone2 > /dev/null; echo "stone"
T="......./T,"; for i in $(seq 1 10); do TAILN=0 bin/p.sh "$T" SAVE=D > /dev/null; grep '? screen' run.log | tail -1 | grep -q "Press button" && T="$T." || break; done
PRE="$T" bin/journey.sh D 2; bin/bk.sh save D hill2 > /dev/null; echo "hillsfar"
TAILN=0 bin/p.sh '' SAVE=D ITEMDUMP=1 > /dev/null; echo "drow readied: $(grep -c '\* Drow Long Sword' run.log)"
