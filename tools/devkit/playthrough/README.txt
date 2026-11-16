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
  (go.sh AUTOPICK="Attack,Take,...": a menu answered by its first word starting so; WHO=n a "WHO WILL" pick)
  hop.sh <from> <x,y> <to>     go.sh, then saved (HOPTOK= tokens first, e.g. F into the next area)
  tour.sh <a> <b> x,y ... hops in turn, fix.sh after each, alternating the slots
  fix.sh <slot> [rest]    the Cheats tab's Healer (Raise Dead, Cure Critical), MEMSET= the casters'
                          spells, the Sanctuary's rest
  buy.sh "m:i,i m:i"      Merchant tokens (goods index as GOODSLOG prints them)
  bk.sh save|load <slot> <name>  a slot's files kept / put back
  look.sh <slot> <tokens> <n>  Look n times where the party stands (Yes to "take it?"; the books)
  try.sh <from> <x,y> <to> [n]  hop.sh kept only without a drain / loss, else again with SEED=1..n
                          (RELEVEL=1: a drained member retrained instead - relevel.sh)
  tour2.sh <a> <b> x,y ...     try.sh square by square
  relevel.sh <slot>       members retrained to the party's target XP (SETEXP + the Trainer)
  morning.sh <slot>       the Sanctuary's rest to hour 0 (PoolRad's City Hall is locked from 14)
  walk.sh <slot> <tokens> <x,y>  go.sh with encounters answered (friendly text -> Wait, else Combat)
  stage.sh <slot> <x,y> <dir> <to>  walk to x,y, face dir, one more step (an edge / exit), save in <to>
  macefight.sh / fightdrv.sh  a fight turn by turn on Tom's tactics against a caster (TGT=, spell queues)
The playsim's play-through helpers: SETMONEY, SETEXP, MEMSET, VMSET, MAPLOCKED, MAPSECRET, MAPFLAGS, #NAME;yes~no~, GOODSLOG on the
Merchant token, AUTOFIGHT's "Continue Battle" / "treasure left" answers.
MEMSET's default comes from /tmp/claude-0/play/memset.txt (p.sh); MEMSET= (empty) for none.

curse/ (2026-11-16): the same kit for the Curse play-through - play folder /tmp/claude-0/cplay (card/CURSE the
game's copy, card/"Pool of Radiance" the PoolRad folder the party is added from, memset.txt the casters' spells),
GAME=2 (GAMEN=), the last snapshot by number (sort -V). New there:
  cbuy.sh "m:p.k,..."     Curse shop tokens: member m buys page p (1-3) line k (0-based)
  st.sh <tokens>          a fight a step at a time (FSLOT= the save): the field rows with fighters, every fighter's
                          place / team / HP ("FIG"), whose turn, the last pages
  endfight.sh <slot> <f>  a fight's remaining turns passed (Guard, else Quick; Continue Battle No), tokens in f
The playsim's fight tokens: "}d" a direction key (0 north ... 7 north-west: Move's steps, a step into a foe an
attack), "{" Select (the move ends), "[" the field and fighters now (FIELDNOW / FIG lines), "#@k;" aims at
fighter k (the FIG number). ITEMDUMP=1: each member's AC, HP, THAC0, experience, coins and items (* readied).
POOLDIR applies from PARTYMENU too (Add Character -> Pool).
