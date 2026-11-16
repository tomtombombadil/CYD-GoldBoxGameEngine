#!/bin/bash
# auto.sh <tokens> [env...]: the play folder run with AUTOTOK=400 AUTOFIGHT=1; prints the texts and the
# final token string (in /tmp/claude-0/play/tok.txt, the '?' removed) and the state
P=/tmp/claude-0/play
TAILN=0 $P/bin/p.sh "$1" AUTOTOK=${AT:-400} AUTOFIGHT=1 "${@:2}" > /dev/null
grep -E "TEXT|MENU prompt|saved now|JOURNAL|LIST" $P/run.log | uniq | cut -c1-230 | tail -${TN:-25}
t=$(grep AUTOTOKENS $P/run.log | tail -1 | sed 's/^  AUTOTOKENS //'); [ -z "$t" ] && t="$1"
t=$(echo "$t" | sed 's/?//'); echo "$t" > $P/tok.txt
grep -E "^  \? screen|^   now" $P/run.log | tail -2 | cut -c1-200
