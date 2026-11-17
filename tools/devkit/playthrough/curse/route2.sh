#!/bin/bash
# Hillsfar -> the Red Plume patrol (drv3) -> every item taken -> Yulash's gate: Ask permission (twice), Parlay,
# Go with guards -> saved inside Yulash; healed, weapons readied, loot sorted
P=/tmp/claude-0/cplay; cd $P; S=D
dots() { for i in $(seq 1 16); do TAILN=0 bin/p.sh "$T" SAVE=$S > /dev/null; st=$(grep '? screen' run.log | tail -1)
  echo "$st" | grep -q "screen 24" && return; echo "$st" | grep -q "Press button" && { T="$T."; continue; }; return; done; }
T="/J,@0,/T,"; dots; echo "start: $st" | cut -c1-80
if [ -n "$FTOK" ]; then T="$(cat $FTOK)"; o=WON; fi
if [ -z "$FTOK" ] && echo "$st" | grep -q "screen 24"; then
  for s in "" 2 3; do
    o=$(STEPS=700 timeout 6000 python3 bin/drv3.py $S "$T" $s 2>/dev/null | grep -E "^(WON|OUT)" | head -1); echo "fight seed '$s': ${o:0:80}"
    echo "$o" | grep -q WON && { [ -n "$s" ] && export SEED=$s; T="$(cat ftok.txt)"; break; }
  done
  echo "$o" | grep -q WON || { echo LOST; exit 1; }
fi
if echo "$o" | grep -q WON; then
  for i in 1 2 3 4; do TAILN=0 bin/p.sh "$T" SAVE=$S > /dev/null; grep '? screen' run.log | tail -1 | grep -qE "screen (5|0) " && break; T="$T."; done
  for m in 0 1 2 3 4 5; do T="$T%$m"; for k in $(seq 1 12); do T="$T/T/I@1/T/E"; done; done
  T="$T/S/E/N,"
  TAILN=0 bin/p.sh "$T" SAVE=$S > /dev/null; echo "taken $(grep -cE 'take ok' run.log), $(grep -E 'take (ok|refused)' run.log | tail -1)"
fi
for w in E A A P G; do T="$T/$w,"; dots; echo "$w: $(echo $st | cut -c1-70)"; done
TAILN=0 bin/p.sh "$T" SAVE=$S SAVENOW=$S > /dev/null; unset SEED
TAILN=0 bin/p.sh '' SAVE=$S > /dev/null; echo "saved: $(grep '^   now' run.log | tail -1)"
bin/bk.sh save $S yul2_raw > /dev/null
