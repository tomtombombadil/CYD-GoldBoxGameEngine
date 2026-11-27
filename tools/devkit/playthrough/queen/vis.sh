#!/bin/bash
# vis.sh LOGS... : visited.txt = the tour targets really reached in those logs, + the avoid list (avoid.txt)
cat "$@" | grep "^== " | awk '{t=$2; for(i=1;i<=NF;i++) if($i=="now"){n=$(i+1)}; if(t==n) print t}' | sort -u > visited.txt
[ -f avoid.txt ] && tr ';' '\n' < avoid.txt >> visited.txt
