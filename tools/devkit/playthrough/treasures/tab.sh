#!/bin/bash
# tab.sh <slot>: the slot's script - the map's event numbers, the whole step table (RAW), journals, menus
P=/tmp/claude-0/tplay
$P/bin/ev.sh $1 > /dev/null
grep -A17 "MAPFLAGS map" $P/disev.txt | head -17
grep " op 25: 1:009A" $P/disev.txt | grep RAW | sed 's/.*RAW//' | python3 -c "
import sys
for line in sys.stdin:
    b=[int(x,16) for x in line.split()]
    if not b: continue
    i=b.index(1,1)+3+2; t=[]
    while i+2<len(b) and b[i]==1: t.append(b[i+1]|b[i+2]<<8); i+=3
    print(' '.join('%02X:%04X'%(k,v) for k,v in enumerate(t)))"
grep " op 25: 1:009A" $P/disev.txt | grep -v RAW | cut -c1-300
grep -o "JOURNAL ENTRY #[0-9]*" $P/disev.txt | sort -u | tr '\n' ' '; echo
grep -E " op 2B:| op 39:" $P/disev.txt | cut -c1-200
