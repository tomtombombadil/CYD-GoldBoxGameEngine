#!/bin/bash
# p.sh <tokens> [extra env...] - run the playsim on the play folder from the party menu (PARTYMENU) or SAVE=x; print the state; last snapshot to /tmp/claude-0/qplay/last.png
P=/tmp/claude-0/qplay/card
[ -z "${MEMSET+x}" ] && [ -f /tmp/claude-0/qplay/memset.txt ] && export MEMSET=$(cat /tmp/claude-0/qplay/memset.txt)   # (the casters spells, MEMSET= empty: none)
cd $P && rm -f out/*.ppm
# EVENV="RUNAT=... RUNSET=... TELE=..." (a script event started by hand) only for runs whose tokens start with EVTOK
EX=(); [ -n "$EVENV" ] && [ -n "$EVTOK" ] && [[ "$1" == "$EVTOK"* ]] && read -ra EX <<< "$EVENV"
env GAME=${GAMEN:-7} MANUAL=1 "${EX[@]}" "${@:2}" timeout ${PTIME:-900} ${SIM:-/tmp/claude-0/ag7/bin/splay} "$P/The Dark Queen of Krynn" "$1?" > ${RUNLOG:-/tmp/claude-0/qplay/run.log} 2>&1
grep -E "^  \? screen|^   now|MENU prompt|TEXT|LIST" ${RUNLOG:-/tmp/claude-0/qplay/run.log} | tail -${TAILN:-6} | cut -c1-220
f=$(ls $P/out/*.ppm 2>/dev/null | sort -V | tail -1); [ -n "$f" ] && convert $f -scale 200% /tmp/claude-0/qplay/last.png
