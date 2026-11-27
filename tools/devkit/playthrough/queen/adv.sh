#!/bin/bash
# adv.sh <slot> x <tokens> [Y|N]: Darkness kit - the playsim's own auto tokens (AUTOTOK: texts, pauses, fights on
# Quick - "/Q" at each fighter's menu, "Continue Battle" No) from <tokens>; Yes / No answered <answer>, a treasure
# taken (takeall.py); saved at the Move menu (env EXTRA for every run). Prints the texts and journal entries.
P=/tmp/claude-0/qplay
# (a fight's treasure taken by the selected member unless EXTRA says otherwise - AUTOTAKE takes tokens)
case "$EXTRA" in *AUTOTAKE=*) ;; *) EXTRA="$EXTRA AUTOTAKE=/T/I$(printf "/T%.0s" $(seq 1 30))";; esac; S=$1; T="$3"; Q=${4:-Y}
# (the treasure's money too - gems, jewelry, coins: the playsim's AUTOMONEY; 2026-11-23, every treasure's money had
# been left behind)
case "$EXTRA" in *AUTOMONEY=*) ;; *) EXTRA="$EXTRA AUTOMONEY=1";; esac
[ -z "$T" ] && T="."
for i in $(seq 1 ${ITER:-30}); do
  PTIME=${PTIME:-2400} AT=1000 TN=0 $P/bin/auto.sh "$T" SAVE=$S ALLTEXT=1 $EXTRA > /dev/null
  T=$(cat $P/tok.txt); st=$(grep '? screen' $P/run.log | tail -1)
  if echo "$st" | grep -q "wait 13 \|wait 1 \|Press <enter>.*wait 2 "; then T="$T."
  elif echo "$st" | grep -q "Yes No\|No Yes"; then T="$T/$Q"
  elif echo "$st" | grep -q "WHO WILL\|Select\]"; then T="$T%${WHO:-2}/S"
  elif echo "$st" | grep -q "screen 5 .*Take"; then T="$(python3 $P/bin/takeall.py $S "$T" "" 2>/dev/null)/E"
  elif echo "$st" | grep -q "wait 3 "; then
    # a list menu (Champions' "WHAT DO YOU DO?" lists): the first PICK word a line starts with, else line LISTPICK (0)
    l=$(grep "LIST prompt" $P/run.log | tail -1 | grep -o "\[[^]]*\]" | tr -d '[]')
    k=${LISTPICK:-0}; n=0; found=""
    for c in $(echo "$PICK" | tr ',' ' '); do n=0; while IFS= read -r line; do echo "$line" | grep -qi "^$c" && { k=$n; found=1; break; }; n=$((n+1)); done <<< "$l"; [ -n "$found" ] && break; done
    T="$T@$k"
  elif echo "$st" | grep -q "screen 24 .*wait 11 "; then T="$T."
  # (a script's PROGRAM 9 camp - Tas's "DO YOU REST?": Exit, and the story goes on; 2026-11-23, a save there
  # left the party at the citadel's default square with the story skipped)
  elif echo "$st" | grep -q "screen 7 .*wait 14 "; then T="$T/E"
  # (a treasure menu the auto run left: whatever is still there is said, never left silently - 2026-11-27,
  # the Dark Queen's menu had no Take and every treasure was exited unseen)
  elif echo "$st" | grep -q "screen 5 \|screen 25 "; then echo "!! treasure left: $st" | tee -a $P/story.log >&2; T="$T/E"
  elif echo "$st" | grep -q "|Move Aim\|Aim Use"; then :
  elif [ -n "$PICK" ] && w=$(echo "$st" | sed -E 's/.*menu \[[^|]*\|([^]]*)\].*/\1/') && k=$(for c in $(echo "$PICK" | tr ',' ' '); do echo "$w" | grep -qw "$c" && { echo "$c"; break; }; done) && [ -n "$k" ]; then T="$T/${k:0:1}"
  else break; fi
done
grep -E "TEXT|JOURNAL|won:|lost" $P/run.log | tail -${TXTN:-10} | cut -c1-120
echo "$st" | cut -c1-110
# (left in the camp's menu - a treasure's Exit then an E: out of it, and saved)
[ -z "$NOCAMPEXIT" ] && echo "$st" | grep -q "screen 7 menu \[|Save View" && { T="$T/E"; st="  ? screen 0 menu [|Move Area"; }
echo "$st" | grep -q "screen 0 menu \[|Move Area\|screen 0 menu \[|Move Encamp\|screen 0 menu \[|Area Cast View" && { TAILN=0 $P/bin/p.sh "$T" SAVE=$S SAVENOW=$S ALLTEXT=1 $EXTRA > /dev/null; grep "saved now" $P/run.log; grep -E "TEXT|JOURNAL|\[fight\]" $P/run.log >> $P/story.log; }
echo "$T" > $P/tt.txt
