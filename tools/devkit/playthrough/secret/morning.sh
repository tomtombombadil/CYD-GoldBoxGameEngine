#!/bin/bash
# morning.sh <slot>: the Sanctuary's rest until the clock's hour (0x4BC9) is 0-1 (PoolRad's day: hours 0-13); saved back
P=/tmp/claude-0/splay; S=$1
TAILN=0 $P/bin/p.sh "|B" SAVE=$S PEEKEND=4BC9 > /dev/null
h=$((16#$(grep PEEKEND $P/run.log | sed -E 's/.*= ([0-9A-F]+).*/\1/')))
n=$(( (24 - h) % 24 )); [ $n -eq 0 ] && { echo "hour $h: already"; exit 0; }
I=$(printf '/I%.0s' $(seq $n)); W=$(printf '_%.0s' $(seq $((n * 8 + 10))))
TAILN=0 $P/bin/p.sh "|B\"2/R/H$I/R$W/E" SAVE=$S SAVENOW=$S > /dev/null
TAILN=0 $P/bin/p.sh "|B" SAVE=$S PEEKEND=4BC9 > /dev/null
echo "hour $h -> $((16#$(grep PEEKEND $P/run.log | sed -E 's/.*= ([0-9A-F]+).*/\1/')))"
