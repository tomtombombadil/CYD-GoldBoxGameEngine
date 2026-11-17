Gateway's kit (2026-11-16): the Curse kit's scripts on /tmp/claude-0/gplay (GAME=8), plus
  gbuy.sh "m:good,..."   the Merchant cheat: member m buys good n (GOODSLOG's numbers, 22 a page)
  glevel.sh <slot> <exp>  every own member given <exp> and trained at the Trainer cheat until none can
  ovgo.py <run.log> <x,y> [flags] [x,y D]   the overland: the way over squares whose MAPFLAGS value is
                         allowed (default the river, A - by boat) as tokens after /M; a town square on
                         the way asks "Enter ...?" - /N and plan on from there
Overland squares: map 22 Yartar 5,12, Nesme 9,4; map 23 Silverymoon 7,2, Everlund 6,5 (from the west
only), Sundabar 15,4; map 21 Luskan 1,1, Port Llast 2,7, Neverwinter 4,10. Saves: slots A-J only.
A map's events: its script's ON-ENTRY table (op 25 on [4C02] = the square's MAPFLAGS value).
