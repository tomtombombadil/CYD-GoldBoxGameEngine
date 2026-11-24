#!/bin/bash
# cross.sh <slot> <script>: from the edge square the party stands on, the step (any facing) that loads
# that script; saved there. The transcript gets its lines.
P=/tmp/claude-0/tplay; cd $P
for t in "|BF" "|BLF" "|BRF" "|BRRF"; do
  TAILN=0 bin/auto.sh "$t" SAVE=$1 >/dev/null
  if grep -aq "script $2 of" run.log; then
    TAILN=0 bin/auto.sh "$t" SAVE=$1 SAVENOW=$1 >/dev/null
    grep -aE "TEXT|JOURNAL|a fight|won|script .* loaded" run.log | uniq | cut -c1-400 >> transcript.txt
    echo "crossed ($t): $(grep -a 'saved now' run.log | tail -1 | cut -c1-60)"; exit 0
  fi
done
echo "NO CROSSING to script $2"; exit 1
