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
  playsim WINFIGHT=1 (EXTRA for adv.sh): every foe down whenever a fight is up - for a story fight the party can't
                          win on Quick (the aerie's eight dragons with their fear): the after-fight script runs; the
                          fight's own rules are the rule checks' (fight_check / fuzz / matrix), not this walk's
  gostepy.sh <slot> <x,y> [answer]  gostep.sh with the questions met on the way answered (WAYANS, default Y)
  AUTOTAKE (playsim) takes TOKENS typed once at each treasure menu - "/T/I/T/T/T..." in Darkness (the selected
                          member takes the lit lines); fight.sh passes them, adv.sh needs them in EXTRA
  upstair.sh <slot>       the Red Tower: every stair side (wall type 2 - table 989F) reached is tried, the first
                          "THE PATH GOES UP" taken (WANT=DOWN the other way); up = facing west, down = east/north
  MAPWRAP=1 (playsim, GOENV for gostep): the path finders step off an edge and in at the other (the tower's
                          floors share one map and wrap); DISASMNOW=1 disassembles a block before it hands back
                          (message blocks 76 / 77 / 84 / 85); ECLTRACE=1 (CYD_TEST_HOOKS) every instruction
  stepwalk.sh <slot> <x,y> [answer]  one step at a time, MAPPATH again after each (Moander's heart pushes the
                          party along its arteries at every beat)

Moander's heart (2026-11-21): buy1.sh <slot> <m> <page> <line> - one Merchant-cheat purchase (from the pool);
several in one visit: "%m/B" + "/N" x page + "v" x line + "/B./E" each, after a first "/P" (Pool), ending
"/S/E" (Share - pooled coins left on the counter are lost, as the original's question says). rdy.sh RDYKEEP=
(regex: the kinds left as they are - default arrows, darts, potions ...). sellplain.sh: never sells an item
still unidentified (ITEMDUMP=2's hidden) - it sold Shal's vorpal sword as a plain long sword. trade.sh /
loot.py as the Secret kit's. adv.sh PICK=Try,Feed,Keep answers a script's word menu (the fake Tanetal, the
Mouth). The heart's events run on ARRIVAL (entry 1) - where its one-way flows made stepwalk circle, the
playsim's TELE=x,y,d (d 0-7, 2 = east) put the party beside the square and one real step entered it.
regear.sh <slot>: the party's standard gear bought again at the Merchant cheat and readied - for the gates
that take the party's magic items (Moander, the way back to the Realms, Bane's land) when nothing was stored.
A member whose pack is full (16 items - a fight's loot) gets nothing: sell first (sellplain / SELLN).

The Temple of Tyr's basements (2026-11-22): blocks 50 and 54 share map 50 - the first basement is its lower
left (in at 7,13), the second its lower right, joined by the stairs on 0,8's south side (8,9's north side back;
script 54 0x8088 - wall type 7, "STAIRS HERE LEAD DOWNWARD"). MAPPATH said "no way" to 12,8 and the play-through
ran the fire elementals' event by hand at 7,13 (RUNAT): the fight then got 7,13's battlefield - the party and
the elementals across its wall line, ended by the 15-round rule. Walked in properly (0,8 S, Yes, then to 12,8
from the west) it is won in three rounds. Never start a fight's event by hand away from its square:
MAPLINK="x,y,d>x2,y2,d2;..." (playsim, MAPPATH) lets the walk take a script's stairs ("F/Y" in the path), e.g.
MAPLINK="0,8,4>8,9,4;8,9,0>0,8,2" here; MAPDOORS=d prints each side's door state (MAPTYPES the wall type).
