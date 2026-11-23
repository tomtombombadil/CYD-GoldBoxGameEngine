#!/bin/bash
# tourc.sh <slot> x,y ...: go2.sh to each square in turn; a backup after each (bak/t_<x>_<y>)
P=/tmp/claude-0/knplay; S=$1; shift
for q in "$@"; do echo "=== $q"; $P/bin/go2.sh $S $q 2>&1 | tail -40; $P/bin/bk.sh save $S t_${q/,/_} > /dev/null; done
