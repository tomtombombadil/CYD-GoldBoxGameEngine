#!/bin/bash
# journey.sh <slot> <list row> [Trail|Wilderness word letter, default T]: Journey on to the row's place by the
# trail; a fight on the way played and finished (fight.sh); the slot saved at the place's menu
P=/tmp/claude-0/cplay; S=$1; R=$2; W=${3:-T}
T="${PRE}/J,@$R,/$W,"
for i in $(seq 1 12); do TAILN=0 $P/bin/p.sh "$T" SAVE=$S ALLTEXT=1 > /dev/null; st=$(grep '? screen' $P/run.log | tail -1)
  if echo "$st" | grep -q "screen 24"; then $P/bin/fight.sh $S "$T" jfight_$(date +%s) | grep -vE "^rdy|Traceback|File|^ +|Error"; exit 0; fi
  echo "$st" | grep -q "Press button" && { T="$T."; continue; }; break; done
TAILN=0 $P/bin/p.sh "$T" SAVE=$S SAVENOW=$S > /dev/null
echo "$st" | cut -c1-80; grep -E "TEXT" $P/run.log | grep -v "(clear):  $" | tail -2 | cut -c1-60
