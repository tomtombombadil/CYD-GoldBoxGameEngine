#!/bin/bash
# macefight.sh: the altar's fight played turn by turn with Tom's tactics - the archer (KESTREL) shoots MACE,
# MIRA Magic Missiles him, SELENE / BRAN cast Silence 15' on him (once each); the rest Quick. A caster
# who can't see MACE yet does Done/Guard; the archer then shoots the nearest. Tokens in fight.tok.
P=/tmp/claude-0/play; S=$(cat $P/altar.slot); T=${START_T:-$(cat $P/altar.tok)}
last=""
for i in $(seq 1 ${STEPS:-60}); do
  TAILN=0 $P/bin/p.sh "$T" SAVE=$S MAPLOCKED=1 "$@" > /dev/null
  who=$(grep "^  ? turn" $P/run.log | tail -1 | awk '{print $3}')
  st=$(grep -E "^  \? screen" $P/run.log | tail -1)
  if ! echo "$st" | grep -q "screen 2[0-9]\|screen 19"; then echo "out of the fight: ${st:0:90}"; break; fi
  if echo "$st" | grep -q "Choose Spell"; then T="$T/E"; continue; fi
  if echo "$st" | grep -q "Aim:"; then echo "aim ($who)"; T="$T#MACE;/T~/T~"; continue; fi
  if echo "$st" | grep -q "Continue Battle"; then T="$T/N"; continue; fi
  if ! echo "$st" | grep -q "Move View Aim"; then T="$T."; continue; fi
  mace_gone=$(grep "^\[page" $P/run.log | grep -ciE "MACE (goes down|is killed|is dying|flees)")
  again=0; [ "$who" = "$last" ] && again=1
  cast=0; echo "$st" | grep -q "Cast" && cast=1
  case "$who" in
    KESTREL) [ $mace_gone -gt 0 ] && a="/Q" || a="/A#MACE;/T~/T~" ;;
    MIRA) if [ $mace_gone -gt 0 ]; then a="/Q"; elif [ $cast = 1 ]; then a="/C/C"; else a="/D/G"; fi ;;
    SELENE) if [ $mace_gone -gt 0 ]; then a="/Q"; elif [ $cast = 1 ] && [ "${sil_s:-0}" -lt 1 ]; then a="/Cvvvvvvvvv/C"; else a="/D/G"; fi ;;
    BRAN) if [ $mace_gone -gt 0 ]; then a="/Q"; elif [ $cast = 1 ] && [ "${sil_b:-0}" -lt 1 ]; then a="/Cvvvvv/C"; else a="/D/G"; fi ;;
    *) a="/Q" ;;
  esac
  # a silence counts once it was aimed at MACE
  [ "$who" = "$last" ] || true
  echo "turn $who -> $a"
  T="$T$a"; last=$who
  echo "$T" > $P/fight.tok
  [ "$who" = SELENE ] && [ "${a:0:3}" = "/Cv" ] && sil_s=1
  [ "$who" = BRAN ] && [ "${a:0:3}" = "/Cv" ] && sil_b=1
done
grep "^\[page\|aim MACE\|AI spell" $P/run.log | grep -iE "MACE|silenc|Casting|lost a spell|Casts" | cut -c1-100 | tail -30
