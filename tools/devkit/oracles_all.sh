#!/bin/bash
# Every oracle the behave suites read, made anew in /tmp/claude-0/bt from the game files in scratch
# (2026-11-26: after a machine change most were gone and `toggles.sh base` showed 60 "oracle ... missing"
# failures that said nothing about the engine). Needs the unpacked programs (/tmp/claude-0/prog/<game>_start.bin
# / _game.bin) and, for the routines RUN in Unicorn (wrate_emu, aicast, areafind), a venv with unicorn:
#   python3 -m venv /tmp/claude-0/uvenv && /tmp/claude-0/uvenv/bin/python -m pip install unicorn
# Usage: [GAMES=/tmp/claude-0/games] [UPY=python-with-unicorn] oracles_all.sh   (its log: /tmp/claude-0/bt/all.log)
# The per-game behave oracles (pool, classes, items, darkness, ...) are behave.sh's; lv_oracle knows Curse,
# Darkness and Treasures only (the other games' `behave levels` values / multi lines say "none for <game>").
O=$(cd "$(dirname "$0")" && pwd)/oracles
G=${GAMES:-/tmp/claude-0/games}
U=${UPY:-/tmp/claude-0/uvenv/bin/python}
mkdir -p /tmp/claude-0/bt
L=/tmp/claude-0/bt/all.log
: > "$L"
( cd "$O" && python3 guard.py && python3 aistep.py && python3 qstep.py && python3 yell.py ) >> "$L" 2>&1
python3 -I "$O/tohit.py" >> "$L" 2>&1
for g in poolrad curse secret darkness champions knights gateway treasures; do
  ovr=$(ls "$G/$g"/*/GAME.OVR "$G/$g"/*/*/GAME.OVR 2>/dev/null | head -1)
  img=$(ls /tmp/claude-0/prog/${g}_start.bin /tmp/claude-0/prog/${g}_game.bin 2>/dev/null | head -1)
  python3 "$O/finder.py" "$ovr" "$img" $g >> "$L" 2>&1
  python3 "$O/turnfind.py" "$ovr" "$img" $g >> "$L" 2>&1
  python3 "$O/onhit.py" "$ovr" $g >> "$L" 2>&1
  python3 "$O/lv_oracle.py" $g >> "$L" 2>&1
done
Q="$G/queen/The Dark Queen of Krynn/DQK.EXE"
python3 "$O/onhit.py" "$Q" queen >> "$L" 2>&1
python3 "$O/qhooks.py" "$Q" >> "$L" 2>&1
for g in darkness treasures secret; do python3 "$O/statfx.py" "$(ls "$G/$g"/*/GAME.OVR | head -1)" $g >> "$L" 2>&1; done
python3 "$O/knights_hit.py" "$(ls "$G/knights"/*/GAME.OVR | head -1)" >> "$L" 2>&1
for g in champions knights; do python3 "$O/appraise.py" $g "$(ls "$G/$g"/*/GAME.OVR | head -1)" >> "$L" 2>&1; done
python3 "$O/appraise.py" queen "$Q" >> "$L" 2>&1
"$U" "$O/wrate_emu.py" >> "$L" 2>&1
"$U" -I "$O/aicast.py" >> "$L" 2>&1
"$U" -I "$O/areafind.py" >> "$L" 2>&1
ls /tmp/claude-0/bt | wc -l
