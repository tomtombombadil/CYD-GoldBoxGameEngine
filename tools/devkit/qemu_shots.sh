#!/bin/bash
# qemu_shots.sh [out dir] - real screenshots of the engine's screens from the
# firmware itself (Tom, 2026-11-09: the Quick tab at both sizes): the S3 QEMU
# builds draw into frame buffers (the 4.0" qemus3: boards/qemu_fb.hpp,
# 480x320; the 5.0" qemus3_50: Panel_Double, 320x240) and TAPS.TXT's
# "shot <name>" dumps them (run_qemu.py --shots). Goes Home -> Curse (GOG
# folder) -> Play -> Load Saved Game A -> the Game key -> the Quick tab: the
# tab as it opens, after tapping Spells (its words), the NPC's row, Bandage
# First on it, the "?"; then the Settings and Cheats tabs.
# Save A is made with an NPC first (the playsim: MAKENPC=5 SAVENOW=A on a copy
# of the scratch Curse folder - the GOG sample party's PHILIPPE as an NPC).
# The QEMU card is read-only: "Not kept" shows on the strip after a toggle.
# Needs: Espressif's qemu-system-xtensa ($QEMU), the qemus3 / qemus3_50
# builds (pio run -c tools/qemu/qemu.ini -e ...), a playsim ($SIM, build.sh).
# The PNGs show game screens: scratch only, never the repo.
set -e
R=$(cd "$(dirname "$0")/../.." && pwd)
OUT=${1:-/tmp/claude-0/ag14/shots}
QEMU=${QEMU:-/tmp/claude-0/ag14/qemu/qemu/bin/qemu-system-xtensa}
SIM=${SIM:-/tmp/claude-0/ag7/bin/qm14sim}
W=${WORK:-/tmp/claude-0/ag14/qshots}
mkdir -p "$OUT" "$W"
# The save with an NPC
TAG=qshots SAVE=A MAKENPC=5 SAVENOW=A bash "$R/tools/devkit/run.sh" curse "$SIM" 2>&1 | grep -E "NPC|saved now"
rm -rf "$W/game"; mkdir -p "$W/game"
cp -r "/tmp/claude-0/ag7/run/curse-qshots/Curse of the Azure Bonds" "$W/game/"
G="$W/game/Curse of the Azure Bonds"
# The taps (screen pixels; the keys' centres as the emulator build prints them)
cat > "$W/taps40.txt" <<'T'
mem 8000
240 286 4000
240 180 4000
121 180 30000
160 100 8000
160 100 8000
160 100 8000
256 195 8000
60 115 6000
139 195 15000
400 231 5000
341 20 5000
shot s40_quick_open 1000
256 99 4000
shot s40_quick_spells 1000
93 232 4000
shot s40_quick_npc 1000
256 135 4000
shot s40_quick_npc_bandage 1000
446 286 4000
shot s40_quick_help 1000
240 160 4000
244 20 4000
shot s40_settings 1000
392 20 4000
shot s40_cheats 1000
mem 1000
T
cat > "$W/taps50.txt" <<'T'
mem 8000
160 220 4000
160 140 4000
81 134 30000
160 100 8000
160 100 8000
160 100 8000
256 195 8000
60 115 6000
139 195 15000
19 220 5000
218 14 5000
shot s50_quick_open 1000
171 78 4000
shot s50_quick_spells 1000
62 175 4000
shot s50_quick_npc 1000
171 109 4000
shot s50_quick_npc_bandage 1000
297 217 4000
shot s50_quick_help 1000
160 120 4000
143 14 4000
shot s50_settings 1000
257 14 4000
shot s50_cheats 1000
mem 1000
T
for env in qemus3 qemus3_50; do
  t=$W/taps40.txt; [ $env = qemus3_50 ] && t=$W/taps50.txt
  python3 "$R/tools/qemu/run_qemu.py" --qemu "$QEMU" --s3 --env $env --game "$G" --taps "$t" --seconds 700 \
    --out "$W/$env" --shots "$OUT" > "$W/$env.log" 2>&1 || true
  grep -E "^shot:|\[play\] quick|LOW|crash|panic|Guru" "$W/$env.log" | sed "s/^/$env: /"
done
