#!/bin/bash
# CLAUDE.md's gnu++2b syntax check
cd ${REPO:-/home/claude/cyd-goldboxgameengine/.claude/worktrees/agent-a28264d39c510117f}
bad=0
for f in src/engine/*.cpp src/game/play.cpp src/game/look.cpp src/game/walk.cpp; do
  g++ -std=gnu++2b -fsyntax-only -Isrc "$f" 2>/tmp/claude-0/ag7/syntax.err || { echo "FAIL $f"; head -5 /tmp/claude-0/ag7/syntax.err; bad=1; }
done
echo "gnu++2b syntax: $([ $bad = 0 ] && echo ok || echo FAILED)"
