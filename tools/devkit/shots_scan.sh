#!/bin/bash
# shots_scan.sh <env:style> ... - the card scan on a WRITABLE card for the
# screenshots (tools/devkit/shots_all.py): the classic QEMU builds qemu28
# (320x240) / qemu35 (480x320) read the card over SD/MMC, the S3 ones only a
# read-only card. Every game (the scratch folders, Curse with its GOG journal
# PDF), style box = + the Launcher folder (box art), icon = without (the GOG
# icons); stopped at the scan's "Done." ([scan] lines on the serial, CYD_QEMU
# only) and its _CYD folder pulled out of the card image into
# $SCAN/<env>-<style>/_CYD. e.g. shots_scan.sh qemu28:box qemu35:box qemu28:icon qemu35:icon
# Scratch only: what the scan makes is from the games.
R=$(cd "$(dirname "$0")/../.." && pwd)
QEMU=${QEMU:-/tmp/claude-0/ag14/qemu/qemu/bin/qemu-system-xtensa}
S=${SCAN:-/tmp/claude-0/shots/scan}
LAUNCHER=${LAUNCHER:-/tmp/claude-0/shots/launcher/Launcher}      # (the Project's Launcher.zip)
PDF=${PDF:-/tmp/claude-0/shots/jrn/CURSE - Adventurers Journal.pdf}   # (the Project's CURSE journal zip)
mkdir -p "$S"
echo "mem 3000000" > "$S/taps_wait.txt"
# (Curse with its journal: a folder of links)
ST=$S/stage; rm -rf "$ST"; mkdir -p "$ST"
cp -al "/tmp/claude-0/games/curse/Curse of the Azure Bonds" "$ST/"
[ -f "$PDF" ] && ln "$PDF" "$ST/Curse of the Azure Bonds/"
games=()
for g in poolrad curse secret darkness gateway treasures champions knights queen; do
  f=$(ls /tmp/claude-0/games/$g | grep -v "^_CYD$" | head -1)
  p="/tmp/claude-0/games/$g/$f"; [ $g = curse ] && p="$ST/$f"
  games+=(--game "$p")
done
for run in "$@"; do
  env=${run%%:*}; style=${run#*:}
  extra=(); [ "$style" = box ] && extra=(--game "$LAUNCHER")
  O=$S/$env-$style; rm -rf "$O"; mkdir -p "$O"
  ( python3 "$R/tools/qemu/run_qemu.py" --qemu "$QEMU" --env "$env" --taps "$S/taps_wait.txt" --seconds "${SECS:-2400}" \
      --out "$O" "${games[@]}" "${extra[@]}" > "$O/run.log" 2>&1 ) &
  pid=$!
  while kill -0 $pid 2>/dev/null; do
    sleep 10
    if grep -aq "\[scan\] Done\." "$O/serial.log" 2>/dev/null; then sleep 5; pkill -f "file=$O/flash.bin"; fi
  done
  MTOOLS_SKIP_CHECK=1 mcopy -s -n -i "$O/sd.img" ::GOLDBOX/_CYD "$O/" 2>&1 | tail -2
  echo "$run: $(ls "$O/_CYD" 2>/dev/null | wc -l) entries in _CYD"
done
