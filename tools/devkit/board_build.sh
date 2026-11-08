#!/bin/bash
# One board build (CLAUDE.md), then .pio/build removed
cd "${REPO:-$(cd "$(dirname "$0")/../.." && pwd)}"
bash /tmp/claude-0/ag7/pio_setup.sh >/dev/null 2>&1 || true
/tmp/claude-0/ag7/pio-venv/bin/pio run -e cyd28_ili9341_res > /tmp/claude-0/ag7/logs/board.log 2>&1
rc=$?
# (after the platform's first download the certifi bundles under ~/.platformio/penv exist: append and retry once)
if [ $rc != 0 ] && grep -qi "certificate\|SSL" /tmp/claude-0/ag7/logs/board.log; then
  bash /tmp/claude-0/ag7/pio_setup.sh >/dev/null 2>&1
  /tmp/claude-0/ag7/pio-venv/bin/pio run -e cyd28_ili9341_res > /tmp/claude-0/ag7/logs/board.log 2>&1
  rc=$?
fi
grep -E "RAM:|Flash:|SUCCESS|FAILED|error" /tmp/claude-0/ag7/logs/board.log | tail -8
rm -rf .pio/build
echo "board build rc $rc"
