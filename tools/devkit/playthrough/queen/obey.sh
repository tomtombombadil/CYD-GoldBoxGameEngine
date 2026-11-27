#!/bin/bash
# obey.sh <slot> <tokens> [steps]: a walk where the script says which way is allowed (the Dark Queen's palace
# escort: "NOT THAT WAY ... GO THIS WAY ANYWAY?") - each step F, else R F, L F, B F; a question that isn't the
# refusal stops it (STOPRE also); "." for texts. Tokens to tok_o.txt, the texts printed
P=/tmp/claude-0/qplay; S=$1; T="$2"
run() { TAILN=0 $P/bin/p.sh "$1" SAVE=$S SEED=${SEED:-1} ALLTEXT=1 $EXTRA > /dev/null; }
for i in $(seq 1 ${3:-40}); do
  ok=""
  for m in F RF LF BF; do
    run "$T$m.."
    st=$(grep '? screen' $P/run.log | tail -1)
    if grep -q "NOT THAT WAY\|OBEY YOUR" <(grep TEXT $P/run.log | tail -6); then continue; fi
    T="$T$m.."; ok=1
    while echo "$st" | grep -q "wait 13 \|wait 1 \|wait 11 "; do T="$T."; run "$T"; st=$(grep '? screen' $P/run.log | tail -1); done
    break
  done
  [ -z "$ok" ] && { echo "!! no way allowed"; break; }
  echo "$(grep '  now' $P/run.log | tail -1 | cut -c1-24) $(grep TEXT $P/run.log | tail -1 | cut -c1-100)"
  echo "$st" | grep -q "wait 0 " || { echo "stop: $st" | cut -c1-120; break; }
  [ -n "$STOPRE" ] && grep TEXT $P/run.log | tail -8 | grep -q "$STOPRE" && break
done
echo "$T" > $P/tok_o.txt
