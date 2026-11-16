#!/bin/bash
# fightdrv.sh: a fight played turn by turn with Tom's tactics against a caster (TGT= its name prefix, default MACE):
# the archer (KESTREL) shoots it, SELENE / BRAN Silence 15' on it, MIRA Magic Missiles; the rest Quick. Per caster a
# spell queue QMIRA / QSELENE / QBRAN ("position:kind ...", the fight's Spells in Memory list order; kind m = aim at
# TGT, a = the first offered). FSLOT= the save, START_T= the tokens to the fight. Tokens in fight.tok.
P=/tmp/claude-0/cplay; S=${FSLOT:-$(cat $P/altar.slot)}; T=${START_T:-$(cat $P/altar.tok)}; TGT=${TGT:-MACE}
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
    if [ "$lastaim" = "$who$i" ]; then T="$T/E"; elif [ "${AIM[$who]}" = m ]; then T="$T#${TGT};/T~/T~"; else T="$T/T"; fi
    lastaim="$who$((i+1))"
    echo "aim ($who ${AIM[$who]})"; continue
  fi
  if echo "$st" | grep -q "Continue Battle"; then T="$T/N"; continue; fi
  if ! echo "$st" | grep -q "Move View Aim"; then T="$T."; continue; fi
  if [ -z "$who" ]; then echo "no one's turn: ${st:0:90}"; break; fi
  mace_gone=$(grep "^\[page" $P/run.log | grep -ciE "${TGT}[A-Z0-9 ]* (goes down|is killed|is dying|flees)")
  cast=0; echo "$st" | grep -q "Cast" && cast=1
  a="/Q"
  case "$who" in
    KESTREL) [ $mace_gone -ge ${TGTN:-1} ] || a="/A#${TGT};/T~/E/Q~" ;;
    MIRA|SELENE|BRAN)
      q=(${Q[$who]})
      if [ $cast = 1 ] && [ ${#q[@]} -gt 0 ]; then
        it=${q[0]}; pos=${it%%:*}; kind=${it##*:}
        if [ $kind = m ] && [ $mace_gone -ge ${TGTN:-1} ]; then a="/Q"; else
          a="/C$(printf 'v%.0s' $(seq 1 $pos 2>/dev/null))/C"; [ $pos = 0 ] && a="/C/C"
          AIM[$who]=$kind; Q[$who]="${q[*]:1}"
        fi
      elif [ $cast = 0 ] && [ $mace_gone -lt ${TGTN:-1} ]; then a="/D/G"; fi ;;
  esac
  echo "turn $who -> $a"
  T="$T$a"
  echo "$T" > $P/fight.tok
done
grep "^\[page" $P/run.log | grep -iE "MACE|silenc|Casting|lost a spell|Casts|asleep|Fire" | grep -v "ORC is silenced" | cut -c1-90 | tail -30
