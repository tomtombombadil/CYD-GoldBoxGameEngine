#!/bin/bash
# t.sh <action> - append the action to ftok.txt (FSLOT the save), run, print new pages, HPs, whose turn
P=/tmp/claude-0/cplay
S="$(cat $P/ftok.txt)$1"; echo "$S" > $P/ftok.txt
FSLOT=${FSLOT:-D} $P/bin/st.sh "$S" > /dev/null
n=${PAGES:-6}; grep -E "^\[page|aim .*not" $P/run.log | tail -$n | sed "s/^\[page [0-9]*\] //" | cut -c1-60 | tr '\n' '|'; echo
grep -o "[0-9]* [0-9a-z] [A-Z -]* at [0-9,]* team [01] hp [0-9]*" $P/run.log | tail -${NF:-11} | awk '{print $2"@"$(NF-4)":"$NF}' | tr '\n' ' '; echo
grep -E "^  \? (screen|turn)" $P/run.log | tail -2 | sed 's/wait.*//' | tr '\n' ' '; echo
