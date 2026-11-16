#!/bin/bash
# p.sh <tokens> [extra env...] - run the playsim on the play folder from the party menu (PARTYMENU) or SAVE=x; print the state; last snapshot to /tmp/claude-0/play/last.png
P=/tmp/claude-0/play/poolrad
[ -z "${MEMSET+x}" ] && [ -f /tmp/claude-0/play/memset.txt ] && export MEMSET=$(cat /tmp/claude-0/play/memset.txt)   # (the casters spells, MEMSET= empty: none)
cd $P && rm -f out/*.ppm
env GAME=1 MANUAL=1 "${@:2}" timeout ${PTIME:-900} ${SIM:-/tmp/claude-0/ag7/bin/splay} $P/POOLRAD "$1?" > /tmp/claude-0/play/run.log 2>&1
grep -E "^  \? screen|^   now|MENU prompt|TEXT|LIST" /tmp/claude-0/play/run.log | tail -${TAILN:-6} | cut -c1-220
f=$(ls $P/out/*.ppm 2>/dev/null | grep -v _end | tail -1); [ -n "$f" ] && convert $f -scale 200% /tmp/claude-0/play/last.png
