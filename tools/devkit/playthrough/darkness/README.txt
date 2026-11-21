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
  gostep.sh <slot> <x,y> [answer] [first tokens]  walk beside x,y (goto.sh, MAPLOCKED / MAPSECRET), face it, step in, adv.sh
  ovgo.sh <slot> <x,y> [answer]  the overland walk, re-planned after each stop (answer N: places on the way left alone;
                          a place is two squares - step onto its other square with Y to enter). The place codes are
                          decimal in the scripts (code - 10 = the ON GOTO's entry): 13 = 0x0D the steading
  adv.sh PICK="Word,..."  a menu answered by the first of these words it shows; fights: auto tokens (/Q each fighter),
                          QASPELLS=1 (EXTRA) so the casters cast; fix.sh = the Healer cheat's Heal for everyone
