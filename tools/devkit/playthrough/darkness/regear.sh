#!/bin/bash
# regear.sh <slot>: Darkness - the party's standard gear bought again at the Merchant cheat (40000 platinum put
# in member 0's purse, pooled, shared, then every purse emptied) and readied; for the gates that destroy the
# party's magic items (Moander, Limbo's return, Bane's land) when nothing was stored. RDYPRE / RDYPOST: camp.
P=/tmp/claude-0/dplay; S=$1
TAILN=0 $P/bin/p.sh "" SAVE=$S RECSET="0:103=40,0:104=9C" SAVESLOT=$S >/dev/null
it(){ m=$1; p=$2; k=$3; t="%$m/B"; for i in $(seq 1 $p); do t="$t/N"; done; for i in $(seq 1 $k); do t="${t}v"; done; echo -n "$t/B./E"; }
T="\"1%0/P$(it 0 2 7)$(it 0 2 4)$(it 0 2 9)$(it 0 2 15)$(it 1 2 7)$(it 1 2 4)$(it 1 2 9)$(it 1 3 0)$(it 2 2 3)$(it 2 2 15)$(it 2 2 4)$(it 2 2 9)$(it 5 2 7)$(it 5 2 4)$(it 5 2 9)$(it 5 2 15)$(it 3 1 11)$(it 3 1 12)$(it 3 0 3)$(it 4 1 11)$(it 4 1 12)$(it 4 3 0)/S/E"
TAILN=0 $P/bin/p.sh "$T" SAVE=$S SAVENOW=$S >/dev/null; echo "bought $(grep -c '\[note\].*buys' $P/run.log)"; grep "saved now" $P/run.log
R=""; for m in 0 1 2 3 4 5; do R="$R$m:103=0,$m:104=0,"; done; TAILN=0 $P/bin/p.sh "" SAVE=$S RECSET="${R%,}" SAVESLOT=$S >/dev/null
RDYKEEP="Potion|Scroll" $P/bin/rdy.sh $S "0:Plate Mail +2,Shield +2,Helm +2,Long Sword +2;1:Plate Mail +2,Shield +2,Helm +2,Mace +2;2:Elfin Chain +2,Shield +2,Helm +2,Long Sword +2;3:Bracers AC 4,Cloak Of Prot +2,Dagger +4;4:Bracers AC 4,Cloak Of Prot +2,Mace +2,Ring Of Prot from evil;5:Plate Mail +2,Shield +2,Helm +2,Long Sword +2" 2>&1 | tail -6
