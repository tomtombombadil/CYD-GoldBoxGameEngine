#!/bin/bash
# doors.sh <slot> [steps]: Treasures' underground rooms (script 50) - at each square the step its flag asks
# (6-9: (flag - 6) & 3, A-D: flag - 10; 0 N 1 E 2 S 3 W); stops when the overland script loads.
P=/tmp/claude-0/tplay; cd $P; S=$1; N=${2:-60}
TAILN=0 bin/p.sh "|BLR" SAVE=$S MAPFLAGS=1 >/dev/null 2>&1
grid=$(grep -a -A17 "MAPFLAGS" run.log | sed -n 2,17p); l=$(grep -a '   now' run.log | tail -1)
T="|BLR"
for i in $(seq 1 $N); do
  pos=$(echo "$l" | sed -E 's/.*now ([0-9]+,[0-9]+) .*/\1/'); face=$(echo "$l" | sed -E 's/.*now [0-9]+,[0-9]+ (.).*/\1/')
  x=${pos%,*}; y=${pos#*,}; fl=$(echo "$grid" | sed -n "$((y+1))p" | cut -d: -f2 | awk -v c=$((x+1)) '{print $c}'); v=$((16#$fl))
  if [ $v -ge 10 ]; then d=$((v-10)); else d=$(( (v-6) & 3 )); fi
  cur=$(echo NESW | grep -bo $face | cut -d: -f1); turn=$(( (d-cur+4)%4 )); tk=$(echo "x R B L" | cut -d' ' -f$((turn+1)) | tr -d x)
  AT=400 QASPELLS=1 bin/auto.sh "${T}${tk}F" SAVE=$S >/dev/null 2>&1; T=$(cat tok.txt); l=$(grep -a '   now' run.log | tail -1)
  echo "$i: $pos flag $fl -> $(echo "$l" | cut -c8-22) $(grep -aE 'a fight' run.log | tail -1 | cut -c1-40)"
  if grep -aq "script 51 of" run.log; then grep -aE "TEXT|JOURNAL|a fight|won:|script .* loaded" run.log | uniq | cut -c1-400 >> transcript.txt; echo OUT; break; fi
done
echo "$T" > tok.txt
