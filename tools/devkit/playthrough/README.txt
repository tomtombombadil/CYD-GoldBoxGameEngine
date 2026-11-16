The play-through kit (Tom, 2026-11-16: play each game through as a player, PoolRad first; CLAUDE.md
"Play-throughs"). Shell scripts around the playsim (bin/splay: build.sh splay "-DCYD_TEST_HOOKS"
tools/playsim/sim.cpp) on a play folder /tmp/claude-0/play/poolrad (the game's folder copy as
POOLRAD/, the saves in it; _CYD/QUICK.TXT the casters' Quick Actions). Saves, screenshots and the
folder stay in scratch (test saves to the Project).
  p.sh <tokens> [env]     a run (PARTYMENU=1 or SAVE=x), state lines, the last snapshot to last.png
  auto.sh <tokens> [env]  the same with AUTOTOK / AUTOFIGHT (NOAF=1: an encounter menu stops it) (texts, Quick fights, "Continue Battle"
                          No, "treasure left" No); the tokens that ran in tok.txt
  go.sh <slot> <tokens> <x,y>  a step at a time to x,y (MAPPATH, locked doors bashed - MAPLOCKED;
                          AUTONO=1 answers Yes/No with No; MAPAVOID= squares passed by; PV=OVPATH
                          walks the wilderness instead);
                          after a fight the party fixed on slot J; stops at a choice
  hop.sh <from> <x,y> <to>     go.sh, then saved (HOPTOK= tokens first, e.g. F into the next area)
  tour.sh <a> <b> x,y ... hops in turn, fix.sh after each, alternating the slots
  fix.sh <slot> [rest]    the Cheats tab's Healer (Raise Dead, Cure Critical), MEMSET= the casters'
                          spells, the Sanctuary's rest
  buy.sh "m:i,i m:i"      Merchant tokens (goods index as GOODSLOG prints them)
  bk.sh save|load <slot> <name>  a slot's files kept / put back
  look.sh <slot> <tokens> <n>  Look n times where the party stands (Yes to "take it?"; the books)
  morning.sh <slot>       the Sanctuary's rest to hour 0 (PoolRad's City Hall is locked from 14)
The playsim's play-through helpers: SETMONEY, SETEXP, MEMSET, VMSET, MAPLOCKED, GOODSLOG on the
Merchant token, AUTOFIGHT's "Continue Battle" / "treasure left" answers.
MEMSET's default comes from /tmp/claude-0/play/memset.txt (p.sh); MEMSET= (empty) for none.
