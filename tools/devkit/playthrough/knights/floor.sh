#!/bin/bash
# floor.sh <slot> <stairs x,y> <N|E|S|W>: Champions kit - go2 to the stairs, take them (adv.sh with the step),
# then tourc.sh every event square the new floor reaches (doors locked / secret passed). Prints the texts.
P=/tmp/claude-0/knplay; S=$1; Q=$2; ST=$3
$P/bin/go2.sh $S $Q > /dev/null 2>&1
TAILN=0 $P/bin/p.sh "," SAVE=$S > /dev/null
f=$(grep "   now" $P/run.log | tail -1 | awk '{print $3}')
d=$(( ( $(echo NESW | grep -bo "$ST" | cut -d: -f1) - $(echo NESW | grep -bo "${f:0:1}" | cut -d: -f1) + 4 ) % 4 ))
case $d in 0) T=F;; 1) T=RF;; 2) T=RRF;; 3) T=LF;; esac
TXTN=10 ITER=14 timeout 2400 $P/bin/adv.sh $S x "$T" Y 2>&1 | grep -E "TEXT|fight\]|JOURNAL" | grep -v "clear):  $"
$P/bin/post.sh $S > /dev/null
TAILN=0 $P/bin/p.sh "," SAVE=$S MAPREACH=1 MAPLOCKED=1 MAPSECRET=1 > /dev/null
here=$(grep "   now" $P/run.log | tail -1 | awk '{print $2}')
sq=$(grep "MAPREACH [0-9]" $P/run.log | head -1 | tr ' ' '\n' | grep = | cut -d= -f1 | grep -v "^$here$" | tr '\n' ' ')
echo "floor at $here: $sq"
$P/bin/tourc.sh $S $sq 2>&1 | grep -E "^===|STOPPED|LOST|fight\]|JOURNAL|  at |TEXT" | grep -v "clear):  $" | awk '!s[$0]++' | cut -c1-120
