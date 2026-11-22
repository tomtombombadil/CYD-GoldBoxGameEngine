#!/bin/bash
# auto.sh <tokens> [env...]: the play folder run with AUTOTOK=400 AUTOFIGHT=1; prints the texts and the
# final token string (in /tmp/claude-0/tplay/tok.txt, the '?' removed) and the state
P=/tmp/claude-0/tplay
TAILN=0 $P/bin/p.sh "$1" AUTOTOK=${AT:-400} AUTOFIGHT=$([ -n "$NOAF" ] && echo 2 || echo 1) "${@:2}" > /dev/null
lostf(){ [ "$(grep -c "a fight:" $P/run.log)" -gt "$(grep -c "^\[fight\] won" $P/run.log)" ] && ! grep -q "^AUTO: out of tokens" $P/run.log; }
# (a fight lost - the party gone, the load menu up: again on other dice, the seed kept for the replays)
if [ -z "$NORETRY" ] && lostf; then
  for sd in 1 2 3 4 5 6 7 8 9 10 11 12; do echo $sd > $P/seed.txt
    TAILN=0 $P/bin/p.sh "$1" AUTOTOK=${AT:-400} AUTOFIGHT=$([ -n "$NOAF" ] && echo 2 || echo 1) "${@:2}" > /dev/null
    lostf || { echo "  (fight lost; won on seed $sd)"; break; }
    [ $sd = 12 ] && { rm -f $P/seed.txt; echo "  (FIGHT LOST on every seed)"; }
  done
fi
grep -E "TEXT|MENU prompt|saved now|JOURNAL|LIST" $P/run.log | uniq | cut -c1-230 | tail -${TN:-25}
t=$(grep AUTOTOKENS $P/run.log | tail -1 | sed 's/^  AUTOTOKENS //'); [ -z "$t" ] && t="$1"
t=$(echo "$t" | sed 's/?//'); echo "$t" > $P/tok.txt
grep -E "^  \? screen|^   now" $P/run.log | tail -2 | cut -c1-200
