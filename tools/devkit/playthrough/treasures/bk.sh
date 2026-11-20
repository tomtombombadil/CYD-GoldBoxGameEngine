#!/bin/bash
# bk.sh save|load <slot> <name>: a Treasures save slot's files (SAVGAMx.PTY, CHRDATx*, VAULTx.DAT) kept in bak/<name> / put back
G=/tmp/claude-0/tplay/card/TREASURE/SAVE; B=/tmp/claude-0/tplay/bak/$3
if [ "$1" = save ]; then rm -rf $B; mkdir -p $B; cp $G/SAVGAM$2.* $G/CHRDAT$2* $G/VAULT$2.* $B/ 2>/dev/null; ls $B | wc -l
else [ -f $B/SAVGAM*.PTY ] || { echo "no backup $3"; exit 1; }; rm -f $G/SAVGAM$2.* $G/CHRDAT$2* $G/VAULT$2.*; for f in $B/*; do n=$(basename $f); n=$(echo $n | sed -E "s/^(SAVGAM|CHRDAT|VAULT)./\1$2/"); cp $f $G/$n; done; echo restored $3 as $2; fi
