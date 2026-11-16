#!/bin/bash
# macefight.sh: the altar's fight played turn by turn with Tom's tactics - the archer (KESTREL) shoots MACE,
# SELENE / BRAN Silence 15' on him, the mages' area spells (Sleep, Fireball) on the orcs, then Magic Missiles
# on MACE; the rest Quick. Per caster a spell queue (list positions as the fight's "Spells in Memory":
# sorted by level, one line a copy). Aims: Silence / Magic Missile at MACE, others the first offered.
# Tokens in fight.tok.
P=/tmp/claude-0/cplay; S=$(cat $P/altar.slot); T=${START_T:-$(cat $P/altar.tok)}
# queues: "position:kind ..." (kind m = aim at MACE, a = the first offered)
declare -A Q=( [MIRA]="${QMIRA:-2:a 6:a 2:a 0:m 0:m}" [SELENE]="${QSELENE:-9:m 13:a 6:a 6:a 4:m}" [BRAN]="${QBRAN:-5:m}" )
declare -A AIM
for i in $(seq 1 ${STEPS:-80}); do
  TAILN=0 $P/bin/p.sh "$T" SAVE=$S MAPLOCKED=1 "$@" > /dev/null
  who=$(grep "^  ? turn" $P/run.log | tail -1 | awk '{print $3}')
  st=$(grep -E "^  \? screen" $P/run.log | tail -1)
  if ! echo "$st" | grep -qE "screen (1|19|2[0-9]) "; then echo "out of the fight: ${st:0:90}"; break; fi
  if echo "$st" | grep -q "Choose Spell"; then T="$T/E"; continue; fi
  if echo "$st" | grep -q "Aim:"; then
    if [ "${AIM[$who]}" = m ]; then T="$T#MACE;/T~/T~"; else T="$T/T"; fi
    echo "aim ($who ${AIM[$who]})"; continue
  fi
  if echo "$st" | grep -q "Continue Battle"; then T="$T/N"; continue; fi
  if ! echo "$st" | grep -q "Move View Aim"; then T="$T."; continue; fi
  if [ -z "$who" ]; then echo "no one's turn: ${st:0:90}"; break; fi
  mace_gone=$(grep "^\[page" $P/run.log | grep -ciE "MACE (goes down|is killed|is dying|flees)")
  cast=0; echo "$st" | grep -q "Cast" && cast=1
  a="/Q"
  case "$who" in
    KESTREL) [ $mace_gone -gt 0 ] || a="/A#MACE;/T~/T~" ;;
    MIRA|SELENE|BRAN)
      q=(${Q[$who]})
      if [ $cast = 1 ] && [ ${#q[@]} -gt 0 ]; then
        it=${q[0]}; pos=${it%%:*}; kind=${it##*:}
        if [ $kind = m ] && [ $mace_gone -gt 0 ]; then a="/Q"; else
          a="/C$(printf 'v%.0s' $(seq 1 $pos 2>/dev/null))/C"; [ $pos = 0 ] && a="/C/C"
          AIM[$who]=$kind; Q[$who]="${q[*]:1}"
        fi
      elif [ $cast = 0 ] && [ $mace_gone = 0 ]; then a="/D/G"; fi ;;
  esac
  echo "turn $who -> $a"
  T="$T$a"
  echo "$T" > $P/fight.tok
done
grep "^\[page" $P/run.log | grep -iE "MACE|silenc|Casting|lost a spell|Casts|asleep|Fire" | grep -v "ORC is silenced" | cut -c1-90 | tail -30
