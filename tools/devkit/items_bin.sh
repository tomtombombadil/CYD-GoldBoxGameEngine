#!/bin/bash
# ITEMS_BIN for behave items: every item of a game (sweep testchars with
# ALLITEMS=1 LIST=1 - the game's ITEMS file, shops and random treasure rows,
# read from the player's files), written to /tmp/claude-0/sw/items_<short>.bin
# (Curse's also as items.bin, behave_items.inc's default path).
# Usage: items_bin.sh [shorts...]   (default: curse poolrad)
# Needs a sweep binary (any flags): BIN=... (default /tmp/claude-0/ag7/bin/bcurse).
# PoolRad's save-only items (EXTRA=) came from a lost scratch file: pass
# EXTRA=<file of 63-byte records> if you have one.
D=$(cd "$(dirname "$0")" && pwd)
BIN=${BIN:-/tmp/claude-0/ag7/bin/bcurse}
[ -x "$BIN" ] || { echo "no sweep binary $BIN (build.sh bcurse ...)"; exit 1; }
mkdir -p /tmp/claude-0/sw
for s in ${@:-curse poolrad}; do
  out=/tmp/claude-0/sw/items_$s.bin
  TAG=items ALLITEMS=1 LIST=1 ITEMS_OUT=$out bash "$D/run.sh" "$s" "$BIN" testchars > /tmp/claude-0/sw/items_$s.txt 2>&1
  echo "$s: $(( $(stat -c %s "$out") / 63 )) records -> $out"
done
[ -f /tmp/claude-0/sw/items_curse.bin ] && cp /tmp/claude-0/sw/items_curse.bin /tmp/claude-0/sw/items.bin
exit 0
