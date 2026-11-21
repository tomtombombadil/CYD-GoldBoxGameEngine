Pools of Darkness play-through kit (2026-11-21): the Secret kit (../secret) with the play folder
/tmp/claude-0/dplay (card/DARKNESS = the GOG copy, card/SECRET = the Secret play-through's folder - the party
comes in by Load Saved Game - Secret, LOADFROM=S POOLDIR=/GOLDBOX/SECRET), playsim GAME=4; saves in
DARKNESS/SAVE (SAVGAMx.PTY, CHRDATxn.*, VAULTx.DAT - bk.sh). The rest of the Secret kit is copied to
/tmp/claude-0/dplay/bin with the paths changed (sed splay -> dplay, $P/SECRET -> $P/DARKNESS, GAME 3 -> 4).
New / changed here:
  adv.sh <slot> x <tokens> [Y|N]  runev.sh without RUNAT: texts and pauses (wait 13) pressed on, Yes / No answered,
                          treasure taken, fights fought, saved at the Move menu (EXTRA= env for every run)
  takeall.py              Darkness's treasure: "%m/T" opens the items list, "/T" takes the lit line
  evpeek.py <dis> [slot]  the square-event ON GOTOs (word 00BF) by PEEK: index -> address -> first texts
                          (match with MAPFLAGS=1's codes)
  ovgrid.py / ovpath.py   the overland (block 17): its code grid (rows at 99B0, 38 x 15) and the 8-way walk to x,y
                          ("/M", R / L = 45 degrees, F); codes 3 (snow) and 4 (water) avoided
