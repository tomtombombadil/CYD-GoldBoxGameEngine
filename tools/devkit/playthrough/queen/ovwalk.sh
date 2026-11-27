#!/bin/bash
# ovwalk.sh <slot> <path tokens>: the Dark Queen's overland - the path a step at a time (ovpath.py's tokens);
# a fight won (qwin.sh), a press pressed, a Yes / No answered OVANS (default N - "do you enter?" is the last
# step's: Y there); stops at any other menu. Tokens to tok_ov.txt.
P=/tmp/claude-0/qplay; S=$1; PATHT=$2; T=""; export WINFIGHT=1 SEED=${SEED:-1}
run() { TAILN=0 $P/bin/p.sh "$1" SAVE=$S ALLTEXT=1 > /dev/null; st=$(grep '? screen' $P/run.log | tail -1); }
n=${#PATHT}
for ((i=0;i<n;i++)); do
  T="$T${PATHT:$i:1}"; run "$T"
  for k in $(seq 1 12); do
    case "$st" in
      *"screen 24"*|*"screen 5 menu [|View Take"*) echo "$T" > $P/tok_fw.txt; $P/bin/qwin.sh $S $P/tok_fw.txt > /dev/null; T=$(cat $P/tok_fw.txt); run "$T"; echo "  fight at step $i";;
      *"wait 13"*|*"wait 1 "*) T="$T."; run "$T";;
      *"|Yes No]"*) if [ $i = $((n-1)) ]; then break; fi; T="$T/${OVANS:-N}"; run "$T"; echo "  answered: $(grep TEXT $P/run.log | tail -1 | cut -c1-80)";;
      *"wait 0 "*) break;;
      *) echo "STOP at step $i: $st"; echo "$T" > $P/tok_ov.txt; exit 1;;
    esac
  done
done
echo "$T" > $P/tok_ov.txt; grep TEXT $P/run.log | tail -3 | cut -c1-150; echo "$st" | cut -c1-100
