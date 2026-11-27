#!/bin/bash
# hwalk.sh SLOT: the Dark Queen's Hulderfolk wood - walk back and forth (FB) until the hunger step clears byte 0xB8
# (the next rest can then be interrupted by the voices; script 26's pre-camp 0x81DA) - saved
P=/tmp/claude-0/qplay
for i in 1 2 3 4 5 6; do
  EXTRA="WINFIGHT=1" $P/bin/adv.sh $1 x "FBFBFBFBFBFBFB" N > /dev/null 2>&1
  TAILN=0 $P/bin/p.sh "." SAVE=$1 PEEKEND=B8 > /dev/null
  grep -q "PEEKEND 00B8 = 0000" $P/run.log && { echo "B8 clear after $i"; exit 0; }
done
echo "B8 still set"; exit 1
