#!/bin/bash
# goto.sh <slot> <x,y> [extra env]: walk there (walkf, NOFLEE); a fight on the way is fought (fight.sh, PRIO etc from the
# environment) and the walk goes on; any other menu: the walk stops there (tok.txt has the tokens to it), its text printed
P=/tmp/claude-0/dplay; S=$1; G=$2; shift 2
for k in $(seq 1 8); do
  o=$(NOFLEE=1 $P/bin/walkf.sh $S "$PRE" $G "$@" | tail -2)
  if echo "$o" | grep -q "screen 24 "; then
    # (the fight through adv.sh - the script's after-fight part runs, its flags set; fight.sh's FIGHT=1
    # mode left them unset and the same fight came back at every step)
    r=$(TXTN=3 ${P}/bin/adv.sh $S x "$(cat $P/tok.txt)" ${GOANS:-N} 2>&1 | grep -E "fight\]|saved now" | tail -2 | tr '\n' ' ')
    echo "  fight on the way: ${r:0:140}"
    echo "$r" | grep -q "saved now" || { echo "  (not saved after the fight - stopped)"; break; }
    continue
  fi
  echo "$o" | head -1 | cut -c1-120
  # (saved only when the walk ends at the exploring menu - a save under a question skips the question's event)
  if echo "$o" | grep -q "STOP"; then TAILN=0 $P/bin/p.sh "$(cat $P/tok.txt)" SAVE=$S ALLTEXT=1 "$@" >/dev/null; echo "  (not saved: a menu waits - tok.txt)"
  else TAILN=0 $P/bin/p.sh "$(cat $P/tok.txt)" SAVE=$S ALLTEXT=1 SAVENOW=$S "$@" >/dev/null; grep "saved now" $P/run.log | tail -1 | cut -c1-70; fi
  grep -E "TEXT|JOURNAL" $P/run.log | tail -${TXTN:-8} | cut -c1-110
  break
done
