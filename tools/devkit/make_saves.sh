#!/bin/bash
# Test saves made with the playsim (no game bytes in the repo): a new party
# created as the game creates it (CREATE=race,sex,class,alignment list
# positions, one character a run), added (ADDGUYS), BEGIN, then saved as it
# stands (SAVENOW). The saves go into the scratch game folder's SAVE so
# run.sh's copies have them, and a copy to /tmp/claude-0/saves/<short>/.
# Usage: make_saves.sh <short> [slot] [spec ...]
#   spec = race,sex,class,align[:NAME]   (default: six characters below)
#   BIN=playsim binary (default /tmp/claude-0/ag7/bin/sim; build.sh sim ...)
# Examples:
#   make_saves.sh gateway A          (Gateway's empty SAVE folder - its test party)
#   make_saves.sh knights B          (Death Knights' save B: a new party)
set -e
D=$(cd "$(dirname "$0")" && pwd)
s=$1; slot=${2:-A}; shift; [ $# -gt 0 ] && shift
BIN=${BIN:-/tmp/claude-0/ag7/bin/sim}
specs=("$@")
if [ ${#specs[@]} -eq 0 ]; then
  # (the last race of the list is human in every game; class / alignment
  # positions 0-2 - what each game offers there: its fighters, thieves,
  # clerics, magic-users)
  specs=("5,0,0,0:TONE" "5,1,1,0:TTWO" "5,0,2,0:TTHREE" "5,1,3,0:TFOUR" "5,0,4,0:TFIVE" "5,1,5,0:TSIX")
fi
G=/tmp/claude-0/games/$s
folder=$(ls "$G" | grep -v '^_CYD$' | head -1)
[ "$s" = poolrad ] && folder=POOLRAD
W=/tmp/claude-0/ag7/run/$s-mksave
rm -rf "$W"
names=()
for sp in "${specs[@]}"; do
  pick=${sp%%:*}; nm=${sp#*:}
  out=$(TAG=mksave KEEP=1 PARTYMENU=1 CREATE=$pick NAME=$nm bash "$D/run.sh" "$s" "$BIN" 2>&1 || true)
  echo "$out" | grep -E "pick:|rolled:|ask \[Save" | tail -3 | sed "s/^/  $nm: /"
  names+=("$nm")
done
list=$(IFS=,; echo "${names[*]}")
TAG=mksave KEEP=1 PARTYMENU=1 ADDGUYS=$list SAVENOW=$slot bash "$D/run.sh" "$s" "$BIN" 2>&1 | grep -E "added|saved now" || true
mkdir -p "/tmp/claude-0/saves/$s"
src=$(find "$W/$folder" -maxdepth 3 -iname "SAVGAM${slot}.*" | head -1)
[ -n "$src" ] || { echo "no save made"; exit 1; }
sd=$(dirname "$src")
# (everything the save wrote - SAVGAMx, the members' own files (CHRDATA*,
# *.SAV / .FX ...), VAULT - into the game's own save folder and the copy)
gsave=$(find "$G/$folder" -maxdepth 3 -iname SAVE -type d | head -1)
[ -n "$gsave" ] || gsave="$G/$folder/SAVE"
mkdir -p "$gsave"
cp "$sd"/* "/tmp/claude-0/saves/$s/"
cp "$sd"/* "$gsave/"
echo "save $slot for $s: $(ls "/tmp/claude-0/saves/$s" | tr '\n' ' ')"
