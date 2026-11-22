#!/bin/bash
# buyn.sh <slot> "m:g m:g ...": Champions kit - one Merchant-cheat visit: the coins pooled, member m buys goods
# index g (GOODSLOG's numbers; PS lines a page, default 17) for each pair, then Share; saved. Prints the notes.
P=/tmp/claude-0/chplay; S=$1; PS=${PS:-17}
t="\"1%0/P"
for p in $2; do m=${p%%:*}; g=${p##*:}; pg=$((g / PS)); ln=$((g % PS))
  t="$t%$m/B"; for i in $(seq 1 $pg); do t="$t/N"; done; for i in $(seq 1 $ln); do t="${t}v"; done; t="$t/B./E"
done
t="$t/S/E"
TAILN=0 $P/bin/p.sh "$t" SAVE=$S SAVENOW=$S > /dev/null
grep -o "\[note\] [A-Za-z ,.]*" $P/run.log | sort | uniq -c
grep -q "saved now ($S): ok" $P/run.log && echo "  saved" || { echo "  not saved:"; grep "? screen" $P/run.log | tail -1; }
