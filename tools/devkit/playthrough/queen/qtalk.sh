#!/bin/bash
# qtalk.sh SLOT "BASE tokens" ANSWER... : a script's conversation answered in order - before each answer, presses
# (".") until a menu or a typed question waits (wait 2 / 3 / 5 / 12); an answer "/X" (menu word), "=TEXT;"
# (typed), "@n" (list line) or "%n/S" (who). Env: SEED (default 1), EXTRA. Prints the texts; the tokens to tok_t.txt
P=/tmp/claude-0/qplay; S=$1; T="$2"; shift 2
run() { TAILN=0 $P/bin/p.sh "$1" SAVE=$S ALLTEXT=1 SEED=${SEED:-1} $EXTRA > /dev/null; grep '? screen' $P/run.log | tail -1; }
for a in "$@" END; do
  for k in $(seq 1 ${QMAX:-25}); do
    st=$(run "$T")
    echo "$st" | grep -qE "wait (2|3|5|12) " && break
    echo "$st" | grep -q "wait 0 " && [ "$a" = END ] && break
    T="$T."
  done
  [ "$a" = END ] && break
  T="$T$a"
done
echo "$T" > $P/tok_t.txt
grep -E "TEXT|JOURNAL" $P/run.log | tail -${TXTN:-12} | cut -c1-170
echo "$st" | cut -c1-120
