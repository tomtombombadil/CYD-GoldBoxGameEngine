#!/bin/bash
# fight.sh <slot> <tokens to a fight> <bakname>: Darkness kit - the fight played with everyone on Quick (playsim FIGHT=1,
# seeds "" 2 3), its treasure taken by the playsim's own step, the slot saved (SAVENOW), backed up as <bakname>
P=/tmp/claude-0/knplay; S=$1; T=$2; B=$3
$P/bin/bk.sh save $S ${B}_pre > /dev/null
for s in "" 2 3; do $P/bin/bk.sh load $S ${B}_pre > /dev/null
  PTIME=2400 TAILN=0 $P/bin/p.sh "$T" SAVE=$S FIGHT=1 AUTOTAKE="/T/I$(printf "/T%.0s" $(seq 1 30))" ${s:+SEED=$s} SAVENOW=$S $EXTRA > /dev/null
  r=$(grep "fight over" $P/run.log | tail -1)
  echo "  fight (seed '$s'): ${r:0:90}"
  # (won = the engine's "[fight] won" line: "result" is Curse's word 0x7EC7, always 0 in Darkness)
  if grep -q "^\[fight\] won" $P/run.log; then grep "saved now" $P/run.log; $P/bin/bk.sh save $S $B > /dev/null; exit 0; fi
done
echo "LOST every seed"; $P/bin/bk.sh load $S ${B}_pre > /dev/null; exit 1
