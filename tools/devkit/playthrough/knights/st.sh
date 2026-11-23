#!/bin/bash
# st.sh <tokens> [env]: a fight step by step - run (SAVE=$FSLOT), then the field around the fighters
# (rows R0-R1, default all with a fighter), the fighters' places / HP, whose turn, the new pages
P=/tmp/claude-0/knplay
TAILN=0 $P/bin/p.sh "$1?[" SAVE=${FSLOT:-I} PAGELOG=1 "${@:2}" > /dev/null
grep -E "^\[page" $P/run.log | tail -${PAGES:-12} | cut -c1-100
awk '/FIELDNOW/{f=1;next} f&&/^  FIG/{f=0} f' $P/run.log | tail -25 | grep -E "[0-9a-z]" | grep -E "[0-5a-z]{1}" | sed -n '1,25p' | grep -E "[0-9][^0-9]*[a-z]|[a-z]|[0-5]" | cut -c1-60 > /tmp/claude-0/knplay/field.txt
grep -E "[a-z0-5]" $P/field.txt | grep -vE "^  [ 0-9]{2} [.#]*$"
grep "^  FIG" $P/run.log | tail -${NF:-30} | awk '$NF>0 || 1' | sed 's/^  FIG //' | tr '\n' ';' | cut -c1-600; echo
grep -E "^  \? (screen|turn)" $P/run.log | tail -2
