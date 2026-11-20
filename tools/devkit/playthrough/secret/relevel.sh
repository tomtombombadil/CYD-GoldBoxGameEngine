#!/bin/bash
# relevel.sh <slot>: members whose levels a drain took retrained to the party's targets (XP granted by SETEXP,
# the Trainer one level a visit; a magic-user's new spell: the list's first). Prints the levels.
P=/tmp/claude-0/splay; S=$1
declare -A XP=([ALDRIC]=125001 [THORIN]=70001 [KESTREL]=140002 [SELENE]=80002 [BRAN]=27501 [MIRA]=40001)
for r in 1 2 3 4 5 6 7 8; do
  TAILN=0 $P/bin/p.sh "|B" SAVE=$S PARTYDUMP=1 > /dev/null
  mapfile -t L < <(grep -E "^  [0-5]: " $P/run.log | awk '{print $2}')
  any=0
  for i in "${!L[@]}"; do
    n=${L[$i]}; x=${XP[$n]}; [ -z "$x" ] && continue
    TAILN=0 $P/bin/p.sh "|B\"3%$i|T" SAVE=$S SETEXP=$i:$x > /dev/null
    grep -q "Do you wish to train" $P/run.log || continue
    any=1
    TAILN=0 $P/bin/p.sh "|B\"3%$i|T/Y/L..|B" SAVE=$S SETEXP=$i:$x SAVENOW=$S > /dev/null
  done
  [ $any = 0 ] && break
done
TAILN=0 $P/bin/p.sh "|B" SAVE=$S PARTYDUMP=1 > /dev/null
grep -E "^  [0-5]: " $P/run.log | sed -E 's/^  [0-5]: ([A-Z]+).* levels (.*)/\1 \2/' | tr '\n' '|'; echo
