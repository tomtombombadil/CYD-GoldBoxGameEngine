#!/bin/bash
# look.sh <slot> <tokens> <n> [answer]: Look n times where the party stands (Yes - or the answer - to each "take it?");
# the journal entries / finds printed; tokens in tok.txt
P=/tmp/claude-0/tplay; S=$1; T=$2; n=$3; A=${4:-/Y}
for i in $(seq $n); do
  NOAF=1 TN=1 $P/bin/auto.sh "$T" SAVE=$S ALLTEXT=1 > /dev/null; T=$(cat $P/tok.txt)
  st=$(grep -E "^  \? screen" $P/run.log | tail -1)
  if echo "$st" | grep -q "|Yes No"; then T="$T$A"; else T="$T/L."; fi
done
NOAF=1 TN=1 $P/bin/auto.sh "$T" SAVE=$S ALLTEXT=1 > /dev/null
grep -E "JOURNAL|YOU FIND" $P/run.log | grep -v "SO FILLED\|NOTHING" | uniq | cut -c1-160
echo "$T" > $P/tok.txt
