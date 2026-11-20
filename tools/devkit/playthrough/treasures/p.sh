#!/bin/bash
# p.sh <tokens> [extra env...] - run the playsim on the play folder from the party menu (PARTYMENU) or SAVE=x; print the state; last snapshot to /tmp/claude-0/tplay/last.png
P=/tmp/claude-0/tplay/card
[ -z "${MEMSET+x}" ] && [ -f /tmp/claude-0/tplay/memset.txt ] && export MEMSET=$(cat /tmp/claude-0/tplay/memset.txt)   # (the casters spells, MEMSET= empty: none)
cd $P && rm -f out/*.ppm
env GAME=${GAMEN:-9} MANUAL=1 HEARD=1 "${@:2}" timeout ${PTIME:-900} ${SIM:-/tmp/claude-0/ag7/bin/splay} $P/TREASURE "$1?" > /tmp/claude-0/tplay/run.log 2>&1
grep -E "^  \? screen|^   now|MENU prompt|TEXT|LIST" /tmp/claude-0/tplay/run.log | tail -${TAILN:-6} | cut -c1-220
f=$(ls $P/out/*.ppm 2>/dev/null | sort -V | tail -1); [ -n "$f" ] && convert $f -scale 200% /tmp/claude-0/tplay/last.png
