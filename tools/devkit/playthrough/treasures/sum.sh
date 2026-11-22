#!/bin/bash
# sum.sh <first square>: the transcript from the last "== to <square>" on, each text once
P=/tmp/claude-0/tplay
n=$(grep -an "== to $1\$" $P/transcript.txt | tail -1 | cut -d: -f1)
tail -n +$n $P/transcript.txt | grep -a "== to\|a fight\|won\|JOURNAL\|TEXT (clear)\|MENU" | grep -v "DURK\|SIGN\|BRASS PLATE\|ARCHWAY" | awk '!s[$0]++ || /== to/' | cut -c1-${W:-460}
