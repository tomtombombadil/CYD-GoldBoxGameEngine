#!/bin/bash
# davik2.sh <slot> <tokens> [steps]: as davik.sh, but with no pointing the party follows the left-hand wall
# (L F, F, R F, B F - the first that moves) until Davik points again. Davik's question answered DTARGET.
P=/tmp/claude-0/qplay; S=$1; T="$2"
run() { TAILN=0 $P/bin/p.sh "$1" SAVE=$S SEED=${SEED:-1} ALLTEXT=1 > /dev/null; }
posof() { grep '  now' $P/run.log | tail -1 | awk '{print $2}'; }
ans() { st=$(grep '? screen' $P/run.log | tail -1)
  if echo "$st" | grep -q "T S R G F L B K P O E X"; then T="$T/${DTARGET:-S}.."; run "$T"; st=$(grep '? screen' $P/run.log | tail -1); fi; }
run "$T"; ans
next=""
for i in $(seq 1 ${3:-60}); do
  pos0=$(posof)
  if [ -n "$next" ]; then run "$T$next.."; T="$T$next.."; ans
  else for c in $([ -n "$STRAIGHT" ] && echo F LF RF BF || echo LF F RF BF); do run "$T$c.."; [ "$(posof)" != "$pos0" ] && { T="$T$c.."; break; }; done; ans; fi
  pos=$(grep '  now' $P/run.log | tail -1 | awk '{print $2,$3}')
  nt=$(grep -c TEXT $P/run.log); t=$(grep TEXT $P/run.log | tail -n +$(( ${ot:-0} + 1 )) | tr '\n' ' '); ot=$nt
  echo "$pos | $(echo "$t" | cut -c1-110)"
  for k in 1 2 3 4 5 6; do echo "$st" | grep -q "wait 1[3]* \|wait 6 " || break; T="$T.."; run "$T"; ans; done
  if echo "$st" | grep -q "screen 24"; then echo "$T" > $P/tok_fw.txt; $P/bin/qwin.sh $S $P/tok_fw.txt > /dev/null; T=$(cat $P/tok_fw.txt); run "$T"; ans; echo "  (fight won)"; fi
  echo "$st" | grep -q "wait 0 " || { echo "stop: $(echo $st | cut -c1-100)"; break; }
  [ -n "$STOPRE" ] && echo "$t" | grep -q "$STOPRE" && break
  case "$t" in *"DAVIK POINTS"*) t="DAVIK POINTS ${t##*DAVIK POINTS}";; *) t="";; esac; case "$t" in *"POINTS "*"AHEAD"*) next=F;; *"POINTS "*"TO THE RIGHT"*) next=RF;; *"POINTS "*"TO THE LEFT"*) next=LF;; *"POINTS "*"WAY YOU CAME"*) next=BF;; *) next="";; esac; [ -n "$next" ] && STRAIGHT=1
done
echo "$T" > $P/tok_d.txt
