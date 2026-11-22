Treasures of the Savage Frontier - the play-through kit (the PortMaster session, 2026-11).
Scratch layout: /tmp/claude-0/tplay (card/TREASURE = a copy of the game folder, bak/ = save backups,
transcript.txt = every text of every leg). The playsim binary: tools/devkit/build.sh splay
"-DCYD_TEST_HOOKS" tools/playsim/sim.cpp. Nothing from the game is kept here - scripts only.

p.sh       tokens from a save (SAVE=J): the state, last.png
auto.sh    tokens + the playsim's own continue / Quick fight (AUTOTOK)
go.sh      walk to x,y in a 3D map (MAPPATH a step at a time), events by AUTOPICK, fights fixed after
tourx.sh   "x,y x,y ...": go.sh to each from slot J, saved after each, transcript kept; loot on
           (AUTOLOOT=1 AUTOTAKE=/S/T/I). Never let it stop on a script's menu: a save there drops the
           event's rest (Leilon's guards were lost that way) - give AUTOPICK every word the place asks.
ovwalk.sh  the overland on foot (terrain flags, low nibble; A = a river: boat). Saves + fix.sh after
           each fight (Quick fights there kill the casters otherwise), listens to dwarves, stops at Enter?
ovgo.py    its planner.  ev.sh: a script's events, squares and first texts (disev.txt = the disassembly).
fix.sh     Healer cheat + rest + memorize.  bk.sh save|load <slot> <name>.  tlevel.sh, trade.sh, rdy.sh,
           shop.py, sell2.py: levels, items.
Lessons: the overland is windows of one world (script 51, maps 51-57) - the town table is at the script's
9B3A / 9B50 / 9B66 (map, x, y), names by the ON GOSUB at 8D8B; edge exits need a step INTO the edge;
"Bash Pick Dispell" doors held by magic open only to Dispell; a vertical menu after text needs the text's
key first ("./L...").

2026-11-21 (Daggerford, the Way Inn, the road to Longsaddle):
- sum.sh <square>: the transcript since the last "== to <square>", each text once.
- tbuy.sh "m:g,g ...": the Merchant cheat's Buy tokens (GOODSLOG's number, 18 a page). A purchase a run:
  '"1%m/P' + Buy tokens + '/B/E/E/S/E' - Pool first (an overloaded member can buy nothing), Share before
  leaving (pooled coins left in a shop are LOST, as the original's).
- Coins weigh: a party carrying its winnings is OverLoaded, takes no treasure ("take refused") and moves 6 in
  fights. Spend or share before a town; check "enc" in ITEMDUMP.
- loot.py reads "[play] sell offer" - Treasures' Sell (the Darkness-type words) now logs it too.
- ovwalk.sh: an overland fight lost is fought again with another SEED (QASPELLS=1). ovgo.py plans by the
  squares' flags only - the overland's WALLS block (rivers, mountains): a step refused says why ("The water
  is too deep!"). Bridges are crossed east-west though MAPPATH calls them closed: walk to the bridge's
  square, then step by hand. A map changes when the party steps ONTO an edge row (54;1,1 north -> 52;12,14).
- The sim's NORACE=1 holds in any run (Disable Race Level Limits), LOOTLOG=1 names what AUTOLOOT leaves.
- A fight lost leaves the slot saved with 0 characters: bk.sh save before every place.

2026-11-22 (Longsaddle, the Tower of Twilight):
- auto.sh: a fight lost (more "a fight:" than "[fight] won" lines) is played again on SEED 1..12; the seed
  that won is kept in seed.txt for the replays (p.sh reads it) until the next SAVENOW.
- A slot copied with bk.sh to ANOTHER letter still reads the first letter's character files (the .PTY names
  them): trials on H use J's characters as they are now. Test on J and restore, or expect that.
- Selling puts the coins on the seller: Pool + Share ('"1/P/S/E') after every loot.py run, or the seller is
  OverLoaded again and the next gift / treasure is refused (Malchor's ring and wand were, once).
- A script's gift is a treasure screen (op 27 + 23): AUTOLOOT takes it like a fight's.
- Lucky papers fill item slots: keep them on a member who is not the taker.
