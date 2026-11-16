#!/bin/bash
# cbuy.sh "m:p.k,p.k m:..." -> Curse shop tokens: member m buys page p (1-3) line k (0-based)
out=''
for grp in $1; do
  m=${grp%%:*}; out="$out%$m"
  for it in $(echo ${grp#*:} | tr ',' ' '); do
    p=${it%%.*}; k=${it#*.}; t="/B"
    for i in $(seq 2 $p); do t="$t/N"; done
    for i in $(seq 1 $k); do t="${t}v"; done
    out="$out$t/B/E"
  done
done
echo "$out"
