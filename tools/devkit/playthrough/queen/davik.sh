#!/bin/bash
# davik.sh <slot> <tokens> [steps]: Hawkbluff's temple - walk where Davik points ("DAVIK POINTS AHEAD / TO THE
# RIGHT / TO THE LEFT / THE WAY YOU CAME"); a blocked step turns right; stops at a question (wait 2 / 5 / 12) or
# STOPRE in the texts. Tokens to tok_d.txt
P=/tmp/claude-0/qplay; S=$1; T="$2"
run() { TAILN=0 $P/bin/p.sh "$1" SAVE=$S SEED=${SEED:-1} ALLTEXT=1 > /dev/null; }
next=F
for i in $(seq 1 ${3:-60}); do
  pos0=$(grep '  now' $P/run.log 2>/dev/null | tail -1 | awk '{print $2,$3}')
  run "$T$next.."
  st=$(grep '? screen' $P/run.log | tail -1)
  pos=$(grep '  now' $P/run.log | tail -1 | awk '{print $2,$3}')
  T="$T$next.."
  t=$(grep TEXT $P/run.log | tail -3 | tr '\n' ' ')
  echo "$pos | $(echo "$t" | cut -c1-110)"
  # (Davik's question again: the target letter, DTARGET)
  if echo "$st" | grep -q "T S R G F L B K P O E X"; then T="$T/${DTARGET:-S}.."; run "$T"; st=$(grep '? screen' $P/run.log | tail -1); fi
  echo "$st" | grep -q "wait 0 " || { echo "stop: $(echo $st | cut -c1-100)"; break; }
  [ -n "$STOPRE" ] && echo "$t" | grep -q "$STOPRE" && break
  case "$t" in *"POINTS "*"AHEAD"*) next=F;; *"TO THE RIGHT"*) next=RF;; *"TO THE LEFT"*) next=LF;; *"WAY YOU CAME"*) next=BF;; *) [ "$pos" = "$pos0" ] && next=RF || next=F;; esac
done
echo "$T" > $P/tok_d.txt
