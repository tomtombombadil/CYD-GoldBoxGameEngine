#!/bin/bash
# Run a playsim / sweep binary on a fresh copy of a game's folder, with the
# game's env (fight_fuzz.inc's header). Usage:
#   run.sh <short> <binary> <mode> [args...]     (extra env passes through)
# short: poolrad curse secret darkness champions knights queen gateway treasures
# TAG=x makes the copy's folder name unique (parallel runs); KEEP=1 reuses it.
s=$1; bin=$2; shift 2
G=/tmp/claude-0/games/$s
folder=$(ls "$G" | grep -v '^_CYD$' | head -1)
[ "$s" = poolrad ] && { G="$G/$folder"; folder=POOLRAD; }   # (GOG PoolRad: the DAX files one down)
W=/tmp/claude-0/ag7/run/$s${TAG:+-$TAG}
if [ -z "$KEEP" ] || [ ! -d "$W/$folder" ]; then
  rm -rf "$W"; mkdir -p "$W/_CYD"; cp -r "$G/$folder" "$W/"
fi
case $s in
  poolrad)   export GAME=1 ;;
  curse)     export GAME=2 ;;
  secret)    export GAME=3 ;;
  darkness)  export GAME=4 ;;
  champions) export GAME=5 MAREA=${MAREA-2} MBLOCK=${MBLOCK-34} FIGHTAT=${FIGHTAT-8441} ;;
  knights)   export GAME=6 BOOTSLOT=${BOOTSLOT-A} MAREA=${MAREA-1} MBLOCK=${MBLOCK-36} FIGHTAT=${FIGHTAT-815B} ;;
  queen)     export GAME=7 FIGHTAT=${FIGHTAT-8C11} ;;
  gateway)   export GAME=8 ;;
  treasures) export GAME=9 CACHE=${CACHE-/GOLDBOX/_CYD/$folder} ;;
esac
export ORACLE_DIR=${ORACLE_DIR-/tmp/claude-0/bt}
cd "$W"
exec "$bin" "$W/$folder" "$@"
