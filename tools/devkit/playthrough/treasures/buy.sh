#!/bin/bash
# buy.sh "m:i,i,i m:i,i" -> merchant tokens: member m buys goods i (list opens at index 1, top 1; pages 20,39,58,69)
out='"1'
for grp in $1; do
  m=${grp%%:*}; out="$out%$m"
  for i in $(echo ${grp#*:} | tr ',' ' '); do
    t="/B"
    if [ $i -le 19 ]; then n=$((i-1)); elif [ $i -le 38 ]; then t="$t/N"; n=$((i-20)); elif [ $i -le 57 ]; then t="$t/N/N"; n=$((i-39)); else t="$t/N/N/N"; n=$((i-58)); fi
    for k in $(seq 1 $n); do t="${t}v"; done
    out="$out$t/B/E"
  done
done
echo "$out"
