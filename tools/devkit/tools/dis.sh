#!/bin/bash
# dis.sh <ovr file> <overlay base hex> <file offset hex> [lines]
# disassembles from a file offset, addresses shown relative to the overlay's base
f=$1; base=$((16#$2)); at=$((16#$3)); n=${4:-80}
ndisasm -b16 -o $((at-base)) -e $at "$f" | head -$n
