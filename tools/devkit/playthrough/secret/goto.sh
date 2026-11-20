#!/bin/bash
# goto.sh <slot> <x,y> [extra env]: walk there (walkf, NOFLEE); a fight on the way is fought (fight.sh, PRIO etc from the
# environment) and the walk goes on; any other menu: the walk stops there (tok.txt has the tokens to it), its text printed
P=/tmp/claude-0/splay; S=$1; G=$2; shift 2
for k in $(seq 1 8); do
  o=$(NOFLEE=1 $P/bin/walkf.sh $S "$PRE" $G "$@" | tail -2)
  if echo "$o" | grep -q "screen 24 "; then
    n=f$(date +%s); r=$(${P}/bin/fight.sh $S "$(cat $P/tok.txt)" $n 2>&1 | tail -2 | head -1)
    echo "  fight on the way: ${r:0:110}"
    TAILN=0 $P/bin/p.sh '' SAVE=$S SETMONEY=4:60 SAVENOW=$S >/dev/null; continue
  fi
  echo "$o" | head -1 | cut -c1-120
  # (saved only when the walk ends at the exploring menu - a save under a question skips the question's event)
  if echo "$o" | grep -q "STOP"; then TAILN=0 $P/bin/p.sh "$(cat $P/tok.txt)" SAVE=$S ALLTEXT=1 "$@" >/dev/null; echo "  (not saved: a menu waits - tok.txt)"
  else TAILN=0 $P/bin/p.sh "$(cat $P/tok.txt)" SAVE=$S ALLTEXT=1 SAVENOW=$S "$@" >/dev/null; grep "saved now" $P/run.log | tail -1 | cut -c1-70; fi
  grep -E "TEXT|JOURNAL" $P/run.log | tail -${TXTN:-8} | cut -c1-110
  break
done
