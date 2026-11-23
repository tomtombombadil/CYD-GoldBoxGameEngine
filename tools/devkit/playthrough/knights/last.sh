#!/bin/bash
# last.sh <dir> [n] - the last n (1) snapshots of a playsim run as one PNG
d=$1; n=${2:-1}
f=$(ls $d/out/*.ppm | grep -v _end | tail -$n)
convert $f -append -scale 200% /tmp/claude-0/knplay/last.png && echo /tmp/claude-0/knplay/last.png
