#!/bin/bash
# post.sh <slot>: Champions kit - after a fight: the standard gear readied again (gear.txt - Quick readies the bows),
# plain loot sold, the Healer cheat; prints the party
P=/tmp/claude-0/qplay; S=$1
timeout 900 $P/bin/rdy.sh $S "$(cat $P/gear.txt)" > /dev/null 2>&1
timeout 1800 $P/bin/sellplain.sh $S 2>&1 | grep -v "^  saved" | tail -3
timeout 600 $P/bin/fix.sh $S 2>&1 | tail -6 | cut -c1-40
find $P/card/out -name '*.ppm' -delete
