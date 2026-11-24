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

2026-11-22 (the Tower's way out, the smugglers' dock, Mintarn):
- The sim's AUTOTAKE now picks the taker: of the first six, the fewest items, then the lightest (TAKER=n
  names one). A plain "Helm" / "Robe" is left (a Helm +2 has its plus in the record).
- Coins: 600 platinum a member is fine, 1300 made ELWYN refuse a gift. Spend at the Merchant before events.
- The Tower of Twilight: the island's shore squares bounce ("too deep and cold"), MAPPATH does not know -
  out by 11,4 12,4 13,4 13,5 13,6 13,7 14,7. Stairs 8,8 -> the alcove 5,7; 9,8 -> 10,8.
- Ships: Waterdeep's square (55;8,7) is now the smugglers' dock (block 21): TRAVELLING, ACCEPT (20 pp,
  journal 25); be on a pier (2,10 Mintarn, 2,12 Orlumbor) facing WEST at hour 5 or 6 (script byte 0x08):
  rest by the Sanctuary cheat next to the pier ('"2/R/H' + '/A' x hours + '/R' + '_' waits + '/E'), a
  walk across town costs hours. A fight with the night ship's thieves closes that pier for good
  (table 0x142 + pier = 9). Mintarn's own ships go to Neverwinter and Orlumbor at 0600.
- An event that ends by moving the party (Mintarn's Tyrant) - tourx walks back to its goal square after.
- Menus a walk must answer: Investigate, Hack, Climb, CAME (to help), ACCEPT (her), Cross.

2026-11-22 (Mintarn's ship, Orlumbor, the crystals):
- The crystals: script byte 0x3C = the bits won (1 Zhentil red, 2 Hosttower green, 4 Kraken blue), 0x3E =
  the kind that fell first in the last alliance fight. Quick always fells a Hosttower mage first. The
  sim's FELL=k (0 / 1 / 2) strikes the first villain of that kind down through combat::damage, so the
  wanted crystal is the one won - PEEKEND=003C to see what is held before choosing.
- tourx's healing after fights RESTS: the hour moves. For a 0600 ship: rest by the Sanctuary to hour 5 on
  the square next to the dock, save, then step by hand. Mintarn: 7,1 north to 7,0. Orlumbor: 13,11 entered
  facing EAST (from 12,11). Fares 80-175 pp.
- Orlumbor's fights by flag: 7A ogres 5,11; 7C the rope webs 1,3; 7B the holes 3,0; 7E the rubble 7,14;
  7F the secret temple of Talos 10,15 (the alliance fight; then Wrighttson at 11,3).

2026-11-22 (Neverwinter, Farr Windward) - A KIT FAULT THAT COST A STRETCH:
- After a merge the sim's auto fight pressed Aim whenever a fighter's menu had no Move word (out of
  moves): every fight stopped in the aim menu, the walk timed out, and the next walk started from the last
  save with the event's flag already set - the fight and everything after it in its script skipped.
  Fixed in sim.cpp (a menu with Quick is a fight menu). CHECK AFTER EVERY WALK: each "a fight:" line in
  the transcript is followed by "[fight] won" before the next "== to" (the awk line below); a walk's time
  limits (TL, timeout) must be generous - a fight cut short is an event lost.
    awk '/== to /{if(o)print "UNFINISHED before " NR; o=0} /a fight:/{o=1} /\[fight\] won/{o=0}' transcript.txt
- Neverwinter (block 28): Nasher's square is 5,7 from the west. First visit: searched, arrested, the
  prison ship, Farr Windward's cave (journal 33) and town (block 47, journal 45). Second visit: his apology
  and the ambassadors - Froal behind the Fat Man's Table (8,1 Talk: journal 58; 10,1 a fight; 10,0), Wimeg
  in the gardens (15,11 15,13 fights; 15,15). Journal 59 is told only if Wimeg is freed BEFORE Froal
  (script byte 0x1C4). Then Nasher's gift (a rod).
- Farr Windward: Ougo at 7,13 (ACCEPT), the office 5,8, the branding room 9,4 - then the script runs to
  Haalbok (journal 38) and Ougo the statue; out by 15,1 (LEAVE), the tunnel to Orlumbor's 0,1, Wrighttson
  at 11,3 (journal 43).

2026-11-23 (Port Llast, Luskan 1, Yartar, Triboar)
- Any script of the game disassembled without going there: p.sh "|B" SAVE=J AREA=1 BLOCK=<n> DISASMNOW=1
  DISASM=8000,FFFF (a loop over 1..60 into dis/<n>.txt; grep "1:012A" finds who writes a script byte).
- A typed answer: token =TEXT; (or TYPE=text in the env) - Triboar's password door (13,0 facing east).
- "Enter <town>?" on the overland comes one key after the weather line: |B<step>./Y
- The tour ship (Port Llast pier 2,8, hour 9-15; tourx's healing rests, so rest to 9 right at the pier).
- Luskan's Jagaerda needs script bytes 0x127-0x12E all set (0x12A = the Waterbaron, Triboar 1,7).
- Yartar -> Triboar: leave by Yartar's WEST gate (0,7) onto the bridge 54;5,3; the north gate's side of
  the river has no way west.
- rdy.sh makes the readied items EXACTLY the list given - name everything the member should wear.
- ovwalk.sh saves over the slot after a fight: back the slot up first; it stops at a place's question.

2026-11-23, later (Ruathym)
- Coins weigh: with ~1,500+ platinum each every Take is refused and (since the auto run waits at a refused
  treasure) the walk stalls there. tourx.sh sets TAKELEAVE=1; sellall.sh <slot> after loot.py sells the unreadied,
  sets every member's platinum to 500 (SETMONEY=4:500) and heals.
- GROUNDLOG=1: the item list's count after each token (how the shop's goods in the treasure were found).
- An event's square from the script's table: the disassembler prints only the first 22 targets of the step
  table - count on from there by the END (op 23 / 00) boundaries (Luskan: 0x1E = 7,4 Jagaerda bound, 0x22 = 6,6).
- A level drain: SETEXP=m:exp, tlevel.sh (NORACE=1 for Elwyn), SETHP=n.
- The original from the engine's save: cp bak/<name>/* /tmp/claude-0/dbx/T/SAVE/, dosbox -conf dbt.conf (Xvfb :5),
  Load Saved Game - Treasures - J, Begin (arrows + Return), the journal word (ans.py), "m" for Move.

2026-11-24 (Mirabar, Fireshear)
- tab.sh <slot>: the script's event map, its whole step table (sim.cpp's DISASM prints a long table's bytes
  as RAW), the journal numbers and menus it holds. An event past the 22nd is found there.
- Events that test the facing (op 03 on 0x11): enter the square from the side the script wants (Fireshear's
  Great Hall 3,9 from the east; Neverwinter's piers facing west).
- One-chance houses (Mirabar's Raurym / Steelfist): after the welcome go straight to the room.
- Fireshear is reached by Neverwinter's Northpier Shipping (4,12) after Mirabar's council; the mines' shafts
  join pockets of both levels - a "no way" square is reached through another shaft.
- The beholder (Fireshear 1,1) can disintegrate (GONE for good): back up before, and play the leg on SEED=n
  until nobody shows HP 0 after the fix.
- AUTOPICK: "No" for the walk, "Yes" only on the leg that needs it (inns and shops otherwise).
- sim.cpp: the auto fight took Aim for a fighter out of moves (stalls, then a stop saved mid-fight) - fixed.

2026-11-24, later (Aurilssbarg, Bjorn's Hold, Icewolf)
- Aurilssbarg's cell opens a full day after the capture (rest 24 hours, then step to the door); its exits: east
  edge x 15 at y 5 / 10 / 13 facing east (-> Bjorn's Hold), north edge y 0 at x 0 / 3 / 7 / 10 / 13.
- Bjorn's square (12,3) sends the party back: go.sh ... MAPAVOID=12,3 to the pier 15,3.
- The two blackrobed fights are not won by Quick play: WINFIGHT=1 for that leg only, and say so.
- A branch a walk does not reach (a 5 % encounter): auto.sh "|B" SAVE=J RUNAT=<address> shows its text.
- Training stops at Treasures' last levels since engine 0.216.1 (cleric 10, the others 11): SETEXP beyond
  them does nothing.

2026-11-24, last (Ice Peak, Freezefire's lair, the ending)
- A walk does not cross blocks: cross.sh <slot> <script> steps off the edge square into that script and saves. The exits are in
  each script's leave routine (PEEKEND=<table>): Icewolf west edge x 0 at y 1 / 4 / 7 / 11 / 14 / 15 -> Ice Peak
  x 15, the same y; Ice Peak south edge y 15 <-> Aurilssbarg y 0 at x 0 / 3 / 7 / 10 / 13.
- Ice Peak: 15,1 is the maze (the undead, the giants), 15,14 a pocket whose 13,15 goes south; the way to the
  lair is outside - Aurilssbarg 10,0 north -> Ice Peak 10,15 -> 10,10 -> the avalanche 9,10 (Enter Tunnel) ->
  the cavern 7,8 -> the lair 3,6 / 2,6.
- The lair's two fights by Quick leave half the party down and Kriiador's speech unsaid (FIND NPC answers
  0x80 + place for a member not okay): WINFIGHT=1 for that leg, said so in the notes.
- The ending's rewards are two treasure screens: auto.sh with AUTOLOOT=1 AUTOTAKE=/S/T/I TAKELEAVE=1 shares
  them; AUTOPICK "No" walks past them.
- PROGRAM 5 (engine 0.216.2): everyone made well, then a key - the ending's "all are healed", the demo's fight.
- After the ending the script puts the party on the overland at 54;7,1 and play goes on.
