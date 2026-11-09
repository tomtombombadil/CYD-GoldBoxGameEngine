#!/bin/bash
# One board build (CLAUDE.md), then .pio/build removed. board_build.sh [env] (default cyd28_ili9341_res;
# e.g. cyd40s3_st7796_cap, cyd50s3_cap). The log: /tmp/claude-0/ag7/logs/board_<env>.log
cd "${REPO:-$(cd "$(dirname "$0")/../.." && pwd)}"
E=${1:-cyd28_ili9341_res}
L=/tmp/claude-0/ag7/logs/board_$E.log
mkdir -p /tmp/claude-0/ag7/logs
bash /tmp/claude-0/ag7/pio_setup.sh >/dev/null 2>&1 || true
/tmp/claude-0/ag7/pio-venv/bin/pio run -e $E > $L 2>&1
rc=$?
# (after the platform's first download the certifi bundles under ~/.platformio/penv exist: append and retry once)
if [ $rc != 0 ] && grep -qi "certificate\|SSL" $L; then
  bash /tmp/claude-0/ag7/pio_setup.sh >/dev/null 2>&1
  /tmp/claude-0/ag7/pio-venv/bin/pio run -e $E > $L 2>&1
  rc=$?
fi
grep -E "RAM:|Flash:|SUCCESS|FAILED|error" $L | tail -8
[ -z "$KEEP" ] && rm -rf .pio/build/$E
echo "board build $E rc $rc"
