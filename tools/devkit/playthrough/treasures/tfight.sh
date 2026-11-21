#!/bin/bash
# tfight.sh <slot> <tokens> "WHO=pos:TARGET pos:TARGET ..." ...: a fight turn by turn - each named caster's
# spell queue (list position in "Spells in Memory" : the foe's name to aim at, or 'a' = the first offered);
# everyone else, and a caster with an empty queue, Quick. Tokens in fight.tok; ends out of the fight.
P=/tmp/claude-0/tplay; S=$1; T=$2; shift 2
declare -A Q AIM
for a in "$@"; do Q[${a%%=*}]="${a#*=}"; done
for i in $(seq 1 ${STEPS:-200}); do
  TAILN=0 $P/bin/p.sh "$T" SAVE=$S MAPLOCKED=1 > /dev/null
  who=$(grep -a "^  ? turn" $P/run.log | tail -1 | awk '{print $3}')
  st=$(grep -aE "^  \? screen" $P/run.log | tail -1)
  if ! echo "$st" | grep -qE "screen (1|5|11|19|2[0-9]) "; then echo "out of the fight: ${st:0:90}"; break; fi
  if echo "$st" | grep -q "screen 5 "; then T="$T${AUTOTAKE:-/T/I/T/T/T/T}/E"; continue; fi
  if echo "$st" | grep -q "screen 11 "; then T="$T/N"; continue; fi
  if echo "$st" | grep -q "Choose Spell"; then T="$T/E"; continue; fi
  if echo "$st" | grep -qi "aim:"; then
    k=${AIM[$who]}
    if [ -n "$k" ] && [ "$k" != a ]; then T="$T#$k;/T~/T~"; else T="$T/T"; fi
    echo "aim ($who $k)"; continue
  fi
  if echo "$st" | grep -qi "continue battle"; then T="$T/N"; continue; fi
  if ! echo "$st" | grep -q "Move"; then T="$T."; continue; fi
  if [ -z "$who" ]; then echo "no one's turn: ${st:0:90}"; break; fi
  a="/Q"
  q=(${Q[$who]})
  if echo "$st" | grep -q "Cast" && [ ${#q[@]} -gt 0 ]; then
    it=${q[0]}; pos=${it%%:*}; kind=${it##*:}
    a="/C$(printf 'v%.0s' $(seq 1 $pos 2>/dev/null))/C"; [ $pos = 0 ] && a="/C/C"
    AIM[$who]=$kind; Q[$who]="${q[*]:1}"
  fi
  echo "turn $who -> $a"
  T="$T$a"; echo "$T" > $P/fight.tok
done
echo "$T" > $P/fight.tok
grep -a "^\[page" $P/run.log | grep -aiE "stone|killed|goes down|Casts|asleep|dying" | cut -c14-90 | tail -${PG:-25}
