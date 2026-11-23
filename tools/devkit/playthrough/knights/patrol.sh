#!/bin/bash
# patrol.sh <slot>: one Hap drow patrol (RUNAT=812D, Combat) on Quick, the treasure left, healed + rested
# (fix.sh), saved back to the slot; prints the counter 4C47 and the fight's line
P=/tmp/claude-0/knplay; S=$1
echo '/C' > $P/ptok.txt
RUNAT=812D N=80 timeout 1500 $P/bin/qend.sh $S $P/ptok.txt > /dev/null
grep -E "^\[fight\]" $P/run.log | tail -1
TAILN=0 $P/bin/p.sh "$(cat $P/ptok.txt)/E/N,.." SAVE=$S RUNAT=812D SAVENOW=$S PEEKEND=4C47 > /dev/null
grep -E "PEEKEND|\? screen" $P/run.log | tail -2
$P/bin/fix.sh $S 1 | head -6
