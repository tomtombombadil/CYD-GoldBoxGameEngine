#!/bin/bash
# buy1.sh <slot> <m> <page> <line> : member m buys the Merchant cheat's goods on page (0-based, Next presses) line
# (0-based), paying from the pooled funds (Pool first); saved back; prints the note shown and the bought line
P=/tmp/claude-0/dplay; S=$1; m=$2
t="\"1%0/P%$m/B"; for i in $(seq 1 $3); do t="$t/N"; done; for i in $(seq 1 $4); do t="${t}v"; done; t="$t/B"
TAILN=0 $P/bin/p.sh "$t" SAVE=$S > /dev/null
grep -o "\[note\] [A-Za-z ,.]*" $P/run.log | tail -1
TAILN=0 $P/bin/p.sh "$t./E/E" SAVE=$S SAVENOW=$S > /dev/null; grep -q "saved now ($S): ok" $P/run.log && echo "  saved" || { echo "  not saved:"; grep "? screen" $P/run.log | tail -1; }
