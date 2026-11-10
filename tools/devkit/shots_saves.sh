#!/bin/bash
# shots_saves.sh - the screenshots' saves (tools/devkit/shots_all.py): slot D
# in every game, made by the playsim (build.sh simshots "-DCYD_TEST_HOOKS"
# tools/playsim/sim.cpp) from the game's own save A on a copy of the scratch
# game folders ($GF/<short>/<GOG folder>, from /tmp/claude-0/games), placed
# where the 3D view shows something (TELE0 then a turn left, L - as
# tools/devkit/shots_viewscan.py scanned every square and facing) and in the block whose
# LOAD MONSTER the run's "runat" starts. The saves show game screens:
# scratch only.
set -e
SIM=${SIM:-/tmp/claude-0/ag7/bin/simshots}
GF=${GF:-/tmp/claude-0/shots/gf}
W=${WORK:-/tmp/claude-0/shots/sim}
mkdir -p "$GF" "$W"
for g in poolrad curse secret darkness gateway treasures champions knights queen; do
  [ -d "$GF/$g" ] || cp -r "/tmp/claude-0/games/$g" "$GF/$g"
done

# sim <short> <moves> : the playsim on a fresh copy of the game folder ($W/<short>), env passes
sim() {
  local s=$1; shift
  local folder; folder=$(ls "$GF/$s" | grep -v '^_CYD$' | head -1)
  local G="$GF/$s"
  [ "$s" = poolrad ] && { G="$G/$folder"; folder=POOLRAD; }
  rm -rf "$W/$s"; mkdir -p "$W/$s/_CYD" "$W/$s/out"; cp -r "$G/$folder" "$W/$s/"
  case $s in
    poolrad) export GAME=1;; curse) export GAME=2;; secret) export GAME=3;; darkness) export GAME=4;;
    champions) export GAME=5;; knights) export GAME=6;; queen) export GAME=7;; gateway) export GAME=8;;
    treasures) export GAME=9 CACHE=/GOLDBOX/_CYD/$folder;;
  esac
  (cd "$W/$s" && "$SIM" "$W/$s/$folder" "$@")
}

# save <short> <slot> <from slot> <moves> : saved after the moves, copied back into $GF
save() {
  local s=$1 slot=$2 from=$3; shift 3
  SAVE=$from SAVENOW=$slot sim "$s" "$@" | grep -E "saved now"
  local folder; folder=$(ls "$GF/$s" | grep -v '^_CYD$' | head -1)
  local wf="$W/$s/$folder" gfolder="$GF/$s/$folder"
  [ "$s" = poolrad ] && { wf="$W/$s/POOLRAD"; gfolder="$gfolder/POOLRAD"; }
  (cd "$wf" && find . -type f) | while read -r f; do
    cmp -s "$wf/$f" "$gfolder/$f" 2>/dev/null && continue
    mkdir -p "$(dirname "$gfolder/$f")"; cp "$wf/$f" "$gfolder/$f"
  done
}

# (Secret's save A waits at its first fight's treasure: Exit, No - save B, then D from there)
save secret B A '.*186,194;*34,194;'
TELE0=14,12,4 save poolrad D A L
TELE0=1,10,0  save curse D A L
TELE0=9,7,4   save secret D B L
TELE0=14,12,2 save darkness D A L
TELE0=12,12,6 save gateway D A L
TELE0=2,4,2   save treasures D A L
AREA=2 BLOCK=34 TELE0=2,9,0 save champions D A L
AREA=1 BLOCK=36 TELE0=7,3,0 save knights D A L
# (the Dark Queen's A: Palanthas' town menu - Leave: outside Cergoth)
save queen D A '__*290,195;__.__'
