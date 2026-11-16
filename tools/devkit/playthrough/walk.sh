#!/bin/bash
# walk.sh <slot> <tokens> <x,y> [env]: go.sh with NOAF, answering encounter menus: a friendly text (FRIENDLY= regex,
# default BENEDICTION|SMILE|WAVE) -> Wait, else Combat; stops at other menus. Tokens in tok.txt, slot in slot.txt.
P=/tmp/claude-0/play; S=$1; T=$2; G=$3; shift 3
for i in $(seq 1 ${WALKN:-12}); do
  NOAF=1 bash $P/bin/go.sh $S "$T" $G "$@" > $P/walk.out 2>&1
  S=$(cat $P/slot.txt); T=$(cat $P/tok.txt)
  st=$(grep -E "^STOP" $P/walk.out | tail -1)
  [ -z "$st" ] && { tail -2 $P/walk.out; exit 0; }
  if echo "$st" | grep -qE "Combat Wait Flee"; then
    if grep -E "TEXT" $P/walk.out | tail -3 | grep -qE "${FRIENDLY:-BENEDICTION|SMILE|WAVE}"; then T="$T/W"; echo "  (friendly: Wait)";
    else T="$T/C"; echo "  (Combat)"; fi
    TAILN=0 AT=${AT:-400} AUTOFIGHT=1 $P/bin/auto.sh "$T" SAVE=$S "$@" > /dev/null; T=$(cat $P/tok.txt); echo "$T" > $P/tok.txt
    continue
  fi
  echo "$st"; grep -E "TEXT|MENU" $P/walk.out | tail -4 | cut -c1-200; exit 1
done
