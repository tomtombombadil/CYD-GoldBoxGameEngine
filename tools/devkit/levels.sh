#!/bin/bash
# behave levels on each game: its oracle (oracles/lv_oracle.py: Curse,
# Darkness, Treasures - the others have none yet, their values / multi parts
# say "none for <game>"), the blv build (build.sh, -DBEHAVE_LEVELS), a fresh
# copy of the game (run.sh); logs in /tmp/claude-0/ag7/logs/lv_<game>.log.
# Usage: levels.sh [games...]   (default: curse darkness treasures)  NOBUILD=1 reuses the binary
D=$(cd "$(dirname "$0")" && pwd)
L=/tmp/claude-0/ag7/logs
mkdir -p "$L"
[ -z "$NOBUILD" ] && bash "$D/build.sh" blv "-DCYD_TEST_HOOKS -DBEHAVE_LEVELS" tools/playsim/sweep.cpp > "$L/build_blv.log" 2>&1
for g in ${@:-curse darkness treasures}; do
  python3 "$D/oracles/lv_oracle.py" "$g" 2>/dev/null
  TAG=lv timeout 3600 bash "$D/run.sh" "$g" /tmp/claude-0/ag7/bin/blv behave levels > "$L/lv_$g.log" 2>&1
  printf '%-10s %s / %s\n' "$g" "$(grep '^== behaviour' "$L/lv_$g.log" | tail -1)" "$(grep '^== .* problems' "$L/lv_$g.log" | tail -1)"
done
