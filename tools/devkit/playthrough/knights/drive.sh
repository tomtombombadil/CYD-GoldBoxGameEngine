#!/bin/bash
# drive.sh <slot> <tokens>: the tokens a step at a time from the save; after each step a press waited for gets ".",
# a Yes / No "/N" (YES pattern: "/Y"), a menu with Leave "/L" (else its first word); prints each step's square,
# 0x4C0F / 0x4C15 and the last text; the whole token string to tok_d.txt
P=/tmp/claude-0/knplay; S=$1; T=""; IN=$2
for i in $(seq 0 $((${#IN} - 1))); do
  T="$T${IN:$i:1}"
  for k in 1 2 3 4 5; do
    TAILN=0 $P/bin/p.sh "$T" SAVE=$S ALLTEXT=1 PEEKEND=C04B,C04C,4C0F,4C15 > /dev/null
    st=$(grep '? screen' $P/run.log | tail -1); tx=$(grep TEXT $P/run.log | tail -1)
    if echo "$st" | grep -q "wait 13 \|wait 1 "; then T="$T."
    elif echo "$st" | grep -q "Yes No"; then [ -n "$YES" ] && echo "$tx" | grep -q "$YES" && T="$T/Y" || T="$T/N"
    elif echo "$st" | grep -q "menu \[|[A-Z][a-z]* .*Leave"; then T="$T/L"
    else break; fi
  done
  echo "${IN:0:$((i+1))}: $(grep PEEKEND $P/run.log | head -4 | awk '{print strtonum("0x"$4)}' 2>/dev/null | tr '\n' ' ')$(grep PEEKEND $P/run.log | head -4 | awk '{print $4}' | tr '\n' ' ') $(echo "$tx" | cut -c3-70)"
done
echo "$T" > $P/tok_d.txt
