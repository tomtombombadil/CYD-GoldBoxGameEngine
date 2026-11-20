#!/bin/bash
# bk.sh save|load <slot> <name>: a save slot's files kept in /tmp/claude-0/splay/bak/<name> (or put back as <slot>)
G=/tmp/claude-0/splay/card/SECRET; B=/tmp/claude-0/splay/bak/$3
if [ "$1" = save ]; then rm -rf $B; mkdir -p $B; cp $G/SAVGAM$2.DAT $G/CHRDAT$2* $B/; ls $B | wc -l
else rm -f $G/SAVGAM$2.DAT $G/CHRDAT$2*; for f in $B/*; do n=$(basename $f); n=$(echo $n | sed -E "s/^(SAVGAM|CHRDAT)./\1$2/"); cp $f $G/$n; done; echo restored $3 as $2; fi
