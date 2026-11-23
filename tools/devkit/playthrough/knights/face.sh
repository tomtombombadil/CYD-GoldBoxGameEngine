#!/bin/bash
# face.sh <slot> <N|E|S|W>: the turn tokens from the saved facing (SAVGAMx.PTY byte 1024+2, 0-7) to the one wanted
f=$(python3 -c "import sys;b=open('/tmp/claude-0/knplay/card/KNIGHTS/SAVE/SAVGAM$1.PTY','rb').read();print(b[1026]//2)")
w=$(echo NESW | grep -bo "$2" | cut -d: -f1); d=$(( (w - f + 4) % 4 ))
case $d in 0) echo "";; 1) echo "R";; 2) echo "RR";; 3) echo "L";; esac
