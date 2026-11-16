#!/bin/bash
# gbuy.sh "m:g,g m:g" -> Gateway Merchant tokens: member m buys good g (GOODSLOG's number; 22 a page)
out='"1'
for grp in $1; do
  m=${grp%%:*}; out="$out%$m"
  for g in $(echo ${grp#*:} | tr ',' ' '); do
    t="/B"; for i in $(seq 1 $((g/22))); do t="$t/N"; done
    for k in $(seq 1 $((g%22))); do t="${t}v"; done
    out="$out$t/B/E"
  done
done
echo "$out/E"
