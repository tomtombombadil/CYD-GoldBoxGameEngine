#!/bin/bash
# PlatformIO in a venv; the proxy CA appended to the certifi bundles (CLAUDE.md)
set -e
V=/tmp/claude-0/ag7/pio-venv
[ -x $V/bin/pio ] || { python3 -m venv $V; $V/bin/python -m pip install -q platformio; }
for f in $(find $V ~/.platformio/penv -name cacert.pem 2>/dev/null); do
  grep -q "ag7-proxy-ca" "$f" || { echo "# ag7-proxy-ca" >> "$f"; cat /root/.ccr/ca-bundle.crt >> "$f"; }
done
$V/bin/pio --version
