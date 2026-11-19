#!/bin/bash
# trade.sh <slot> "from:Item Name:to;..." - each item (the first unreadied one of that name) handed from member
# <from> to member <to> through View - Items - Trade; saved back. (RDYPRE / RDYPOST as rdy.sh for camp)
P=/tmp/claude-0/cplay; S=$1
IFS=';' read -ra G <<< "$2"
for g in "${G[@]}"; do
  f=${g%%:*}; r=${g#*:}; nm=${r%:*}; to=${r##*:}
  TAILN=0 $P/bin/p.sh "" SAVE=$S ITEMDUMP=1 >/dev/null
  k=$(awk -v m="$f" -v n="$nm" '/^  member /{c=($2==m); i=-1; next} c && /^    [* ] /{i++; s=$0; sub(/^    [* ] /,"",s); if (s==n && substr($0,5,1)==" ") {print i; exit}}' $P/run.log)
  [ -z "$k" ] && { echo "  $f: no unreadied '$nm'"; continue; }
  d=$((to - f)); mv=""; if [ $d -gt 0 ]; then mv=$(printf 'v%.0s' $(seq 1 $d)); elif [ $d -lt 0 ]; then mv=$(printf '^%.0s' $(seq 1 $((-d)))); fi
  dn=""; [ $k -gt 0 ] && dn=$(printf 'v%.0s' $(seq 1 $k))
  TAILN=0 $P/bin/p.sh "${RDYPRE}%$f/V/I,$dn/T,$mv/S,/E/E${RDYPOST}" SAVE=$S SAVENOW=$S >/dev/null
  # (a refused trade - the taker overloaded or full - says so: the item still the giver's)
  TAILN=0 $P/bin/p.sh "" SAVE=$S ITEMDUMP=1 >/dev/null
  k2=$(awk -v m="$f" -v n="$nm" '/^  member /{c=($2==m); i=-1; next} c && /^    [* ] /{i++; s=$0; sub(/^    [* ] /,"",s); if (s==n && substr($0,5,1)==" ") {print i; exit}}' $P/run.log)
  if [ "$k2" = "$k" ]; then echo "  $f -> $to: $nm REFUSED (still member $f's)"; else echo "  $f -> $to: $nm"; fi
done
