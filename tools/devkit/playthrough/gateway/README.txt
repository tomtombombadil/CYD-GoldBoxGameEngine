Gateway's kit (2026-11-16): the Curse kit's scripts on /tmp/claude-0/gplay (GAME=8), plus
  gbuy.sh "m:good,..."   the Merchant cheat: member m buys good n (GOODSLOG's numbers, 22 a page)
  glevel.sh <slot> <exp>  every own member given <exp> and trained at the Trainer cheat until none can
  ovgo.py <run.log> <x,y> [flags] [x,y D]   the overland: the way over squares whose MAPFLAGS value is
                         allowed (default the river, A - by boat) as tokens after /M; a town square on
                         the way asks "Enter ...?" - /N and plan on from there
Overland squares: map 22 Yartar 5,12, Nesme 9,4; map 23 Silverymoon 7,2, Everlund 6,5 (from the west
only), Sundabar 15,4; map 21 Luskan 1,1, Port Llast 2,7, Neverwinter 4,10. Saves: slots A-J only.
A map's events: its script's ON-ENTRY table (op 25 on [4C02] = the square's MAPFLAGS value).

The end (2026-11-17). Ascore's arena is a maze fight: the playsim's AUTOMAZE=d walks the member whose
turn it is off the field's d edge (0 north, 2 east, 4 south, 6 west) once no enemy is left; AUTOHEAL=1
puts everyone back up between fights (the arena has no camp); SETHP=n gives everyone n hit points
(saved with SAVENOW); AUTOSTOPSCRIPT=n ends the auto run in script n. The whole end from a save at
Ascore 9,7:  AT=3000 bin/auto.sh "|BFFRFFFFRFLFFFFLFF..../M" SAVE=I AUTOMAZE=0 AUTOHEAL=1
The original's ending without its fights (DOSBox, a scratch copy of the game only):
  tools/devkit/tools/daxp.py ECL6.DAX out.DAX 27 2d0109081280349d 20001c081280349d
puts NEWECL 28 at the arena's gate (block 27, 0x83FF).
