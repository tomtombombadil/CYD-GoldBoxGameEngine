#!/bin/bash
# The per-game behave baselines (oracle-based): makes ITEMS_BIN and the
# oracles (tools/devkit/oracles/*.py -> /tmp/claude-0/bt), builds each
# game's sweep (build.sh, objects in /tmp/claude-0/ag7/obj/<name>) and runs
# it on a fresh copy of the game (run.sh); logs in /tmp/claude-0/ag7/logs/
# behave_<game>.log, a summary line per game.
# Usage: behave.sh [games...]   (default: all)  NOBUILD=1 reuses the binaries
D=$(cd "$(dirname "$0")" && pwd)
O=$D/oracles
L=/tmp/claude-0/ag7/logs
B=/tmp/claude-0/ag7/bin
mkdir -p "$L"
H="-DCYD_TEST_HOOKS"
declare -A FLAGS=(
  [curse]="$H -DBEHAVE_CLASSES -DBEHAVE_ITEMS -DBEHAVE_SPELLS -DBEHAVE_MONSTERS -DBEHAVE_EVENTS -DBEHAVE_SAVES"
  [poolrad]="$H -DBEHAVE_POOL -DBEHAVE_SPELLS"
  [secret]="$H -DBEHAVE_SECRET"
  [darkness]="$H -DBEHAVE_DARKNESS"
  [treasures]="$H -DBEHAVE_TREASURES"
  [champions]="$H -DBEHAVE_CHAMPIONS"
  [knights]="$H -DBEHAVE_DEATHKNIGHTS"
  [gateway]="$H -DBEHAVE_GATEWAY"
  [queen]="$H -DBEHAVE_QUEEN")
declare -A PART=([curse]="" [poolrad]=pool [secret]=secret [darkness]=darkness [treasures]=treasures
  [champions]=champions [knights]=deathknights [gateway]=gateway [queen]=queen)
declare -A ORACLES=([curse]="items spells monsters classes" [poolrad]="pool" [gateway]="gateway" [secret]="secret secret_monsters secret_spells secret_items secret_situations")
games=${@:-curse poolrad secret darkness treasures champions knights gateway queen}
[ -f /tmp/claude-0/sw/items_curse.bin ] || bash "$D/items_bin.sh" curse
for g in $games; do
  for o in ${ORACLES[$g]}; do python3 "$O/$o.py" 2>/dev/null; done
  [ -z "$NOBUILD" ] && bash "$D/build.sh" "b$g" "${FLAGS[$g]}" tools/playsim/sweep.cpp > "$L/build_b$g.log" 2>&1
  ITEMS_BIN=/tmp/claude-0/sw/items_curse.bin TAG=bh timeout 3600 bash "$D/run.sh" "$g" "$B/b$g" behave ${PART[$g]} > "$L/behave_$g.log" 2>&1
  printf '%-10s %s / %s\n' "$g" "$(grep '^== behaviour' "$L/behave_$g.log" | tail -1)" "$(grep '^== .* problems' "$L/behave_$g.log" | tail -1)"
done
