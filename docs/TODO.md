# Open tasks (Tom and Claude)

Tom, 2026-10-12: "start keeping a list of tasks we need to do - both you
and me." Kept up to date with every push (CLAUDE.md); finished items go
to the bottom with their version, then drop off after a while. Synced to
the Project as claude/TODO.md.

## Tom

- [x] Spell lists open on the first spell with Memorize lit, the highlight stays after Memorize, "Memorize these spells?" on Yes, Pool's rest menu lit by whole words, a menu left by Exit starts on its first word (the shop's Buy) - Tom, 2026-10-31 (v0.161.2)
- [ ] Show Rolls (2026-10-31, Settings, on by default): in a fight the
      panel's last rows show "To Hit: 17+2=19" / "Needed: 14" /
      "Damage: 4+3=7", spells' saves and damage. Is the light grey
      readable on your screens? Is "Needed" (20 - AC, modifiers counted
      from THAC0 20) the form you want, or the THAC0 / AC themselves?
- [ ] Pool's journal references (2026-10-31 fix): tap **Rescan** once
      (JOURNAL.DAT gets the 18 City Hall proclamations), then try the
      City Hall's postings, a tavern tale and the cabinet's "ENTRIES 23
      AND 14" (Back shows the second). If tavern tales still show only
      "Read Tavern Tale N ...": is the card's Pool the GOG or the Steam /
      SNEG release? (SNEG's journal comes from the Gold Box Companion,
      which has no tales or proclamations.)
      Answered (Tom, 2026-10-31: SNEG Pool on the 5.0" S3 - a tale said
      "unknown edition"): the next build reads the SNEG PDF's own text.
- [ ] SNEG journals from the PDF's text (2026-10-31, not pushed yet): start
      the board once (it shows "Preparing Journals" and reads each SNEG
      journal PDF - a minute or two each; or tap Rescan); then in Pool:
      a tavern tale, the City Hall's proclamations, and the Game menu's
      Journal PDF tab (the book as text, Prev / Next through the pages,
      Zoom = text size). Also Curse SNEG's tavern tales. Send Logs if
      the scan log says the text couldn't be read (memory).
- [ ] Claude: the Savage Frontier and Krynn SNEG journal PDFs (Project
      zips) through `journal::make_book` on the PC - their sections
      (Introduction, tavern tales) checked against pdftotext, as done
      for Pool / Curse / Secret / Darkness.
- [ ] The 5.0" boards (`TTB-CYD-GBGE_5.0in_Capacitive_ESP32-S3`, the
      FNK0115Q first; then the Hosyond - its back / box against the
      Freenove's): (1) the picture: 640x480 in the middle, black bars at
      the sides, colours right (red / green / blue blocks in Settings -
      if red and blue are swapped, Swap Red/Blue fixes it; say so and
      the board's default changes), no flicker or shifting (also while
      saving and in a fight); (2) touch: taps land where the stylus is,
      in both Rotate 180 settings, light taps seen, nothing from the
      bars; (3) the brightness slider dims the backlight (or only on /
      off?); (4) sound: Hit / Miss and the step sound, whole, and touch
      still fine while sounds play (the touch's INT pin is also the
      amplifier's LRCLK); (5) the serial line (115200, the CH340's port)
      shows "[lcd] whole screen doubled ... us" and "[touch] GT911 at
      0x.., 800 x 480" - send those two lines; (6) Send Logs after a
      fight.
- [ ] Flash the latest build and play a fight-heavy session on the 4.0"
      and the 3.2"; then Send Logs. What to look for: room left in fights
      (MEMORY.TXT), Hit / Miss sounds all through a fight, whether drawing
      feels slower with the 4-bit screen (v0.87.0).
- [ ] The 4.0" S3: does sound play (the codec, v0.103.1)? (Send Logs
      after a fight.)
- [ ] Send Logs goes to cyd.classic.games.logs@gmail.com (the Classic
      Games address) - keep it, or name another (one line in
      web/l/index.html).
- [ ] Try the items with the ITEMTEST characters (sent 2026-10-12 as
      ITEMTEST_characters.zip: Curse's six in its SAVE folder, Pool's five
      beside its SAVGAMA.DAT; what each carries in ITEMTEST.txt) and
      report what doesn't work as in the originals.
- [ ] Keep play testing Pool of Radiance and Curse; report anything that
      plays differently from the originals. New in v0.89.0 for Pool:
      potions / wands / the Manual, Gauntlets of Ogre Power, the cloaks
      and rings - worth a look.
- [x] Decided (Tom, 2026-10-31): a one-choice "PRESS ENTER TO CONTINUE"
      goes on with any key or a tap (movement keys too), unlike the
      original; after a script moves the party the position line shows
      the new square at once (the original shows the old one). SPEC 12.

## Claude

- [ ] Rule tests (2026-11-01; CLAUDE.md "Rule tests"): the fight checker
      (R1-R12), `fightfuzz` and `behave matrix` run clean on all nine
      games. Fixed from them: cornered fleeing fights where it stands
      (Pool part 16's open (c), the endless mummies), the to-and-fro
      flight, no morale test while fleeing, items / spells / Turn Undead
      before a flight, the free attack's hold, moves below 0, Champions'
      Fire Shield question, casts not putting off the no-attack end,
      Quick under fear + berserk; Game Fixes: saves below 0, a step
      taken held. Round 2 (2026-11-01): confirmed in every game's program
      (Secret, Darkness, Champions, Gateway, Treasures, Death Knights,
      Dark Queen): only an attack resets the 15 rounds (the one writer
      of the limit past the fight's start), the fleeing turn's shape
      (moralFailureEscape's third back-step). Fixed: panic for a turn
      only (each game's FleeCheck clears it first) and "is forced to
      flee" in every game; the morale test's moves with haste / slow and
      Curse never below 0; a lightning bolt bouncing between two walls
      for ever (the fight hung - Darkness, all games); Dark Queen's
      Entangle 0x1B / Power Word Stun
      0x6A (Death Knights' 0x06 for both is its program's - effect 6 is
      held there); Game Fixes: Protection from Evil's -2 to hit, a save
      never worse a level up (paladin 8th, Darkness's / Treasures' thief
      19th). The lich's 104 / 81: its MONCHA record as stored, DQK's
      LOAD MONSTER copies it (R6 lets it stay). Added: the matrix's
      weapon immunities, images / displacement, morale, spells on every
      status; `behave levels` (classes to their last level, Wisdom
      bonus, money sums, camp cures) and the monkey's party checker - all
      clean on the nine games. Open: (1) R10 still flags all-Quick
      stalemates the rules allow (Queen's chromatic dragons nothing in
      the party can hurt) - not bugs, the player would act; (2) not done
      yet: shops' buy / sell / Id prices against the originals, treasure
      sharing by the rules, encumbrance and movement, wands / scrolls /
      potions' use outside fights, poison / disease clocks over a rest,
      level drain and Restoration in the later games.

- [ ] Money with Game Fixes (Tom, 2026-10-31; SPEC 12 "Money"): done for
      Pool, Curse, Secret, Gateway. Open: (1) Champions' coin worths
      (copper, bronze, steel, platinum - from its program) before its
      money joins in; (2) Tom's rates were 2e's (1 pp = 50 sp); the engine
      keeps the games' own (1e: 1 gp = 20 sp) - tell Tom; (3) the wagon
      driver (Pool 2:9's whole-party payer) not run in the playsim yet.

- [ ] Pool full play test, side by side with DOSBox (part 1, v0.160.3:
      Phlan's civilized area, 17 fixes; part 2, v0.160.4: lists, wall
      ahead, Locked menu keys, monsters' valueless items). Part 3
      (2026-10-31): part 2's "facing north" placement and "Silver 144" were
      a wandering orc fight at 13,2 N (a replay ran into it) - no such
      bug. The original's placement is now dumped from dosbox-debug and
      matched square for square (kobolds 22 fighters, large orc 30, guards
      40, leaders 11); Pool gets its own placement (ovr11 +0x1577). Every
      slums fight fought side by side (kobolds, goblins, orcs, hobgoblins,
      large orc, guards, leaders, wandering orcs x3): XP totals and coins
      equal. Fixed: SURPRISE's result,
      the first member selected after a fight (Detect), the treasure's
      coin list (order, no Select, Exit -> Take menu, green prompt), the
      3D view after a treasure, THE END screen and the game's end, a
      monster copy's items reversed. Part 4: the slums finished (hemp
      market, Ohlo's potion, the guards, the 25th win) and the clerk's
      reward; training (cleric, magic-user) matched. Fixed: yes/no
      questions' start / colour per caller, the camp save's Quit TO DOS,
      0x7EC8 on a treasure without a fight, loot past 64 keeps the newest,
      Loot / Take Exits, the encounter distance in 0x7EC0 / 0x7EC1, script
      room zeros, spell list bar, first member after a hall, Memorize's
      screens (words, bars, starred list, Exit before the question), a
      rest's encounter (script first, no "rudely interrupted"). Part 5:
      the arena's duels built (CALL 0x8000 / 0x8001, ROLF; Curse's code
      the same, its scripts never call them) and matched side by side; the
      treasure menu after every fight; spell lists open on the page's last
      line; training keeps weight / movement; Pool's delay x 225 ms;
      Trade's picker / Overloaded; an engine save loaded by
      the original; the boat to Sokal Keep and its undead packs (placement,
      XP by the rules). Part 6: Sokal Keep finished (pale form / Ferran -
      LUX, the orc and hobgoblin force: 56 placed identically, the
      illusionary wall, the equipment, the frogs, the smithy's hammer,
      the boat back), the inn's rest, Cast in camp, Sell / Id / Buy, Halve
      / Join - all matched after fixes: the encounter picture at once,
      typed answers (column 1, white; the cursor kept - Tom), Quick clerics turn
      undead, fleeing fighters' way and "is forced to flee", "has found
      Treasure!" when all ran, first member at any treasure, Search /
      rest highlights, the camp text after a rest / Magic, the camp
      picture at Cast's on-whom, the shop's wrap / Id's
      timed word, payments as platinum + odd gold (Buy, Id, cures), yes/no
      questions not carrying a highlight. Part 7 (HP 250 party): Kuto's
      Well (lizardmen 31, the buildings' kobolds, the well, the
      catacombs, Norris the Gray's bandits), ECL1 block 18 (Bane's
      shrine, the robed orc, the Pitt, a locked door), Stojanow Gate
      (wagon, bugbears, the volleys), into ECL4 block 2 - all matched
      after fixes: record 0x113 / 0x114 after a fight (every round's
      start sets them for all, the fallen keep them), the encounter
      picture comes back after a fight, Knock at a passage-3 door with a
      magic-user in the party, a script DAMAGE's lines (white, from
      column 38, HP line by line, position line off) and its PRESS
      <ENTER>/<RETURN> wait. Undead fights' XP still differs by the
      dice (rules now the same). Part 8: the Textile House (ECL4 block
      2: the thieves' well and Restal, three hobgoblin fights, the man
      in chains - Free / Parlay x5, Skullcrusher's ogre and the iron
      box; placement identical, XP and coins equal) and Mendor's Library
      (2:15: the kobolds' map - entry 10, the stacks' books - entry 7,
      the spectre; teleported in past its locked doors) matched after
      fixes: a PICTURE missing from the area's PIC file leaves the
      sprite, Pool / Curse PARLAY's prompt " ", a LOAD CHARACTER past the
      party forgotten at WHO / the script's end (all games), Pool's map
      edge (past an edge = the far edge's first square; none in blocks
      0 / 10). Open: Grishnak's temple (block 2, 3,11) - the original's
      party (HP 250) lost twice from the same save, the engine won 6 of 6
      seeds (Hold Person / "slays helpless" rules read the same; suspect
      the Quick members' choices - bandaging, targets). Not reached:
      Cadorna's reward (the box in hand: Phlan's watch takes it), Valjevo,
      the cemetery, the Kovel mansion, the council's later commissions;
      Scribe (no scroll); a cleric level-up with 2nd-level spells. Pool
      has no Fix. Tom's "pushed back out onto the street after a
      merchant": not in Pool - DOSBox, the general store 9,11, the arms
      shop 8,11 and the Temple of Tyr: Exit (or No) leaves the party on
      the shop's square, same facing, as the engine; check it in Curse /
      Secret / Darkness. Part 9 (Tom's board, goblin fights; every
      game): the attack pose drawn over the ready pose and the turn's
      box (both icons at once) - now on the ground, as the original's
      0x74572; "Surrenders" with the panel already saying unconscious -
      now shown while it stands (no turn box), then off the field (the
      original's 0x644A7; both seen so in DOSBox, the slums' orcs). Then:
      Grishnak's temple - the computer's multi-target spells took one
      target (Hold Person's aim 6: the original picks (aim & 3) + 1 at
      random; DOSBox holds two from one cast; the engine now loses 2 of
      6 seeds there too); after the last enemy goes the round goes on
      (the rest act, then Continue Battle) and a surrender's panel stays
      (Tom's "unconscious" at Continue Battle); View works the character
      out first (weight after training); Pool's View shows the portrait
      where the state word 0x7F12 is 3 (Phlan, GAME.OVR 0x21537); View -
      Spells without "Choose Spell:". Matched: Kuto's Well -> slums (a
      wandering orc fight: XP 180 each both) -> Phlan, Cadorna's and
      Norris's rewards at the clerk (208 + 833 XP, coins), the
      commissions list, training Brother Sean to cleric 2 (HP, 3000 XP
      cap). Open: (a) the hall's party menu - DOSBox shows the first
      member lit on entry but moves from the old selection, and keeps it
      after Begin (part 4 saw it reset after Darkstar's training - the
      engine resets at Begin); (b) a computer's spell with no target:
      the original loses it ("Spell Aborted"?), the engine tries the
      next; (c) 2nd-level cleric spells need cleric 3 (Sean at the 3000
      cap). Part 10: (a) settled - the original keeps the selected
      member at Begin (cleric and magic-user halls, after training /
      learning / Create; the engine's reset undone); its "first member
      lit" is a display lag: after a script's LOAD CHARACTER loop (the
      proclamations at 4,3) its panel lights Bakshi while the selection
      is still the old one (Home / End move from it) - not copied (the
      engine lights the real one). (c) done: cleric 3 by training and by
      poked records; Memorize works a 2nd+ level caster's counts out
      again (level 1 keeps the record's); Wisdom 18 cleric: level 1 -> 3,
      2 -> 4, 3 -> 4 + 3 - the original's exactly (Tom's question). Also
      fixed: a spell list's Next stops at the last full page, the
      highlight keeping its line; the camp's text after View (empty) /
      Magic's Exit (kept when nothing covered it); Create's pick lists
      (heading hidden under the shared top line, "Exit" alone, Return /
      a tap takes the line). Matched: Create (race ... "Keep this
      character?", name, icon, saved .CHA), Remove / Add, Modify (DOSBox
      moves with Up / Down, not Home / End), the commissions list.
      (b) done (part 11): the computer's spells by their row's byte 14
      (Curse / Pool, Facts::foe_byte) - own-side ones at itself, cast
      again while on (the engine had sent aim-4 own-side spells at the
      enemy); Cure Light Wounds at the hurt one next to it, a body
      lying there of either side (Game Fixes: own side only, SPEC 12).
      Part 11 also: Pool's COMBAT clips the distance to the party's
      sight (Sokal's frogs), NEWECL forgets the selection to put back,
      PICTURE 0xFF takes a sprite away. Matched: Sokal Keep finished
      (Ferran, the stands, the boat back), its reward at the clerk
      (1308 XP), the inn's rest, the city hall locked after 14:00.
      Part 12: Tom's tavern brawls - the rule (a tale, then RANDOM 3
      = 1: one in four; Grab at the pickpocket) and the fighters
      (11 + 11 + 11 + 13, all placed facing south) match DOSBox; the
      dice were unseeded (the same tales / brawls / rolls at every
      Play) - seeded since 8e6bbf6 (Curse and the rest share it).
      (d) settled: the slums' kobolds load to the cap (63) in both,
      the original leaves 4 out at placement, as the engine (fighter
      table identical). A brawl entered facing east: 17 against 18
      in both (same squares). (c) settled: the city hall's last
      member is block 0's "MAD MAN" roll (RANDOM 19) - a NEWECL keeps
      the selection its loop left, as the engine. The slums' locked
      doors (Pick / Bash), Kuto's Well crossed, Podol's auction
      (Disguise, comments, a bid, the disguise blown, cancelled)
      matched; 77119bf a PICTURE counts as the encounter's picture
      shown (Curse: check). Open: (e) a leader's loot order.
      Part 13: matched the clerk after the cancelled auction (208 XP,
      250 gold + 200 platinum, the bishop's new commission), the
      bishop and Dirten joining, the harbour's boats and payer, the
      wilderness (steps, the city's west / east / north edges), the
      merchants' surprise, the graveyard's entry and its 63
      skeletons (199 XP each both), the northern gates, the Kovel
      block's Black Hand patrol (Flee). Fixed: the wilderness screen
      drawn by the menu (not before; rope at 16,16; the rider twice),
      CLEARBOX only clears there, the 3D frame back on leaving, a
      load there; ADD NPC on the party's side (Dirten was an enemy);
      View: no Trade for an NPC; wilderness encounters' sprite;
      the treasure screen's / camp's frame there; the main loop's
      redraw by state (DS 0x4432 / LOAD FILES + PIECES). Curse: check
      ADD NPC's side byte and View's Trade for NPCs. Not reached: the
      Kovel mansion inside, Valjevo, Scribe (no scroll yet), later
      commissions. The Add list's order (DOSBox: the newest .CHA first) not
      checked further.
      Part 14: the Kovel mansion - the wealthy block's guard rooms
      (eight orcs, the hay beds' orcs: 21 / 18 XP both), the patrol's
      Advance / Parlay (Nice: they wander off), the mansion (ECL3 14,
      north edge of block 24): locked doors (Pick per member, a failed
      Pick -> "Bash Exit", Bash), the thieves (let go / attack), the
      weapon pile's gas ambush (524 XP), the chase (225 x7 / 263 x6:
      Dirten dead in DOS - by the rule), the files (journal 38, 51),
      the scythes / needle / gas (find traps d100, Avoid / Disarm,
      who disarms), the scroll cabinet, the jail (journal 48), the
      loot room, the three cabinets (journal 23, 14; Pick / Break),
      the rugs - matched. Fixed: a377139 DAMAGE lines past the text
      window wait (white press words) and clear; adb5b86 SPELL (0x3B)
      always "first member, place 1" as the original's uninitialised
      byte (Cast Knock opens), the real search with Game Fixes
      (SPEC 12); b584fd5 "has no copyable scrolls" timed and cleared.
      Scribe: Darkstar (level 1, untrained) has 2 MU scrolls - "no
      copyable scrolls" in both; a real scribe needs a trained magic-
      user. Not reached: the caskets (6,7), the map (4,5), back to the
      clerk / the bishop, the graveyard's inside, Zhentil Keep,
      Valjevo. Curse: check its SPELL op (the engine reads rec 0x1F +
      k, the list is at 0x1E) and its DAMAGE page break.
      Part 15: the rest of the Kovel mansion - the four caskets (find
      traps, Avoid / Disarm, needle / dart / powder / 12 darts: DAMAGE's
      save and to-hit forms, the "PRESS ANY KEY TO CONTINUE" paging of a
      line that overflows mid-line and the blank line after an exactly
      full one; Pick / Break; 428 / 314 / 107 / 714 XP), the crude map
      (journal 29), the clerk's mansion reward (164 XP, then the
      graveyard commission: Yes -> 2000 XP, a +1 +3 vs undead two-handed
      sword, 3 clerical scrolls; the nomads' commission new),
      Darkstar's MU training (level 2, a spell learned; HP d4 by dice),
      Scribe on both sides (Bakshi: "YOU ALREADY KNOW THAT SPELL" timed;
      Darkstar: unidentified MU scrolls -> "no copyable scrolls"), the
      arena's partner question, the harbour WEST boat, wilderness
      encounters (driders: party destroyed in DOS by Sleep - avoided by
      re-syncs; kobolds 65 / 48 XP by dice), the graveyard's entry and
      a skeleton surprise (155 / 84 XP: the engine's turned skeletons
      fled - by dice). Fixed: ab240cc the step's position line drawn
      again before "Locked." after a DAMAGE took it away. Open: (a) the
      original's Right arrow at a script Yes / No menu (the thieves'
      trainer, Phlan 9,0) makes the first member selected (View shows
      Bakshi) - the engine keeps the selection; cause not found (the
      select routine at START 0x38C9 handles End / Home only). (b) The
      thieves' rooms 11,2 / 12,3 / 13,6 / 12,7 not repeated (the same
      script, both choices compared in part 14). Not reached: the
      graveyard's scripted squares (voice's gifts 8,2, crypt 12,4,
      tower 15,6, the paladin's wraith 15,8, the vampire 14,0, mummies
      9,11), Zhentil Keep, the pyramid, Mendor's library, Valjevo.
      Part 16: the graveyard - skeletons / zombies (surprise both ways,
      Flee, Advance -> Parlay), 1600 XP zombies, then wights (DOS
      destroys the level 1-2 party by drain, the engine too: THE END)
      and the mummies at 9,11 (party destroyed in DOS; the engine's
      turned mummies, immune to plain weapons, flee forever - open,
      see below). The graveyard's north (voice 8,2, crypt 12,4, tower,
      wraith, vampire) is behind the tower's wights at 9,5 - out of
      reach for this party. Matched: training (cleric / MU / fighter /
      thief halls: one level a visit, XP capped below the next, 1000
      gold, HP dice told), the spell to learn, the arena's partner
      (RANDOM 250 picks the NPC), Id at the shop (200 gold), Scribe's
      "can not scribe" (timed), the clerk's commissions, the harbour's
      BAY boat, wilderness kobold surprises (Flee), the nomads' camp
      (wire / crowd, parlay, feast, journal 55, two nights, "stay and
      fight"). Fixed: 3dd05d1 an NPC joins as Pool's load makes it
      (icon size / d6 colours, Random(256), portrait by class die,
      class values - THAC0 19); 175bcce Scribe's list in the items'
      order; 6962fd0 the nomads' tents' test (0x4C9F == 0 -> grass);
      ea4ebd6 ECL CLOCK (0x34) doesn't move the clock (Game Fixes:
      it does; SPEC 12); 6e120dd CAMPING with the view blocked;
      d56bcf6 CALL 0x2C90 redraws only after the block's LOAD FILES /
      PIECES; 26f3284 SPRITE OFF only while a sprite is up. Open:
      (c) a fight whose turned undead can't be hurt and can't get
      away never ends in the engine (their 0-damage blows reset the
      15-round rule) - DOS not seen (its cleric didn't turn them) -
      fixed by the rule tests: a cornered one fights where it stands
      (coab's moralFailureEscape);
      (d) with 8 members DOS's party-menu redraw after End / Home
      blanks "CREATE NEW CHARACTER" (cosmetic, not copied). Resume:
      the nomads' kobold war (save G, ECL7 17, after "stay and
      fight"), then Zhentil Keep, the pyramid, Mendor, Valjevo. Curse:
      check ADD NPC's icon / portrait / class values, Scribe's order,
      CALL 2E10 / SPRITE OFF conditions.
      Check in Curse's play test: spell-list paging,
      the camp text after View / Magic, Create's lists (Curse has the
      same list routine?), Begin's selection, View - Spells' prompt, PARLAY's prompt, Knock at a door's second locked
      kind, the DAMAGE lines' look, the Quick cleric's turning, the
      flee way, "is forced to flee", Curse's payments (Buy / Id / cures),
      the press words, word wrap,
      load path, camp picture / Quit to DOS, View prompt, shop and temple
      layouts, the list routine, movement keys at the Locked menu,
      monsters' valueless items, part 3's: the
      selected member after a fight, SURPRISE's result, the coin list,
      the placement routine, copies' item order; and part 4's: yes/no
      start / colours, Loot / Take Exits, 0x7EC0 / 0x7EC1, 0x7EC8; part 11's:
      the computer's own-side spells and Cure Light Wounds target
      (coab agrees - check it plays), PICTURE 0xFF over a sprite, NEWECL
      and the selection, COMBAT's distance against sight.
      Decided (Tom, 2026-10-31, v0.160.9; SPEC 12): Quick lasts one fight
      (part 3's carry-over undone), typed fields keep the cursor ("How
      much", script questions), lists stay lit while a question asks
      (the unlit lists of parts 3-6 and Gateway's / Darkness's undone).

- [ ] Code review follow-ups (branch review-fixes-engine, 2026-10-31):
      (a) Dark Queen reroll - the engine's queen_roll keeps spells a
      previous roll's training learned (its god's spells are learned at
      Done, before the roll); check DQK.EXE's roll (+0x1A90) / DOSBox
      whether the original drops them. (b) Death Knights: LOAD CHARACTER
      | 0x80 never removes a member (its second flag, DS 0xDB60, is never
      set in GAME.OVR / START.EXE) - as the original; check whether a
      script relies on it (then a Game Fixes case). (c) the Dark Queen's
      QSV header u16 at bytes 11-12: kept from the loaded game - find
      what DQK.EXE writes there. (d) the overland Thenols fight budget
      (144.6 KB) wasn't re-measured with the per-copy items.
- [ ] **Engine screens for ports**: the platform layer is done
      (v0.91.1: src/game + src/engine use only src/platform/*.h); still
      CYD-only are the engine's own screens (Settings, journal viewer,
      library, logs - `ui/viewer.cpp` etc. draw with LovyanGFX). For a
      PortMaster port: draw them through ui:: primitives a port supplies.
- [ ] Memory, next cuts (measure each with heapprof): the party's unused
      member slots (1.7 KB each), Data's words read on demand, the spell
      names' 4 KB read buffer in pieces.
- [ ] The plain C5's scan: "The Unlimited Adventures journal couldn't be
      processed (card full, or not enough memory)" (the Companion's) - look
      at why (8 MB free there, so the card or the file).
- [ ] S3 / C5 follow-ups once Tom has run them: fix what his reports show
      (colours, touch direction, sound level - the codec's volume is
      0 dB); then the 8-bit canvas for the VGA games there.
- [ ] Dark Queen on the classic boards: try it in its own EGA mode (Tom,
      2026-10-12) when we get to the Krynn games. Decided (2026-10-24,
      branch darkqueen, queen_facts.md): its EGA install's pictures are
      the VGA ones through each picture's own EGA table, pixel for pixel
      (not dithered by the installer) - the engine reads the GOG VGA files
      and draws them in 16 colours itself (no EGA install, no scan step).
      Open: the 16 colours' RGB on the EGA screen for pictures whose
      colours start at 0 (TITLE's: their own 16?) - check in DOSBox.
- [ ] **Dark Queen of Krynn** phase 3+ (Claude; facts queen_facts.md +
      queen_facts_2.md, its section 12 the plan). Done in phase 2 (branch
      darkqueen2): DQK.EXE as a plain MZ (data segment file 0x63A20), the
      profile (being built), maps of any size, the 3D view / area map /
      frame from its files, GAME.FON's text, ECL.GLB scripts in the VM,
      QSV saves and its characters, Load from Death Knights - the Play
      Test (Dev Tools) walks the forest after Palanthas. Next: the
      menus as its buttons, the title, picture animation, the far band of
      the 3D view, items / effects / spells, View, fights, sound, 256
      colours on the PSRAM boards. Undecoded: picture kinds 0x03 and
      0x19, SOUNDS.GLB's DIG4, colour cycles.
- [ ] Darkness's EGA mode: how the original's EGA.DRV turns VGA pictures
      into 16 colours (facts from the player's file), so Darkness plays on
      the classic boards as its own EGA mode shows it.
- [ ] QEMU: the cache-error panic inside play::open (parked).
- [ ] **Secret of the Silver Blades** (started 2026-10-12; facts and the
      work plan in the Project's claude/secret_facts.md; SPEC 6c). Done:
      program layout, words, scripts, frame, font, title, Screen / Walk
      Test (v0.92.0); the 439-byte record and the saves (v0.93.0); party
      menu, Load, Begin into Priam's house (v0.94.0); the rule tables,
      areas (v0.95.0); the scripts' view of a character (v0.96.0); View
      Character (v0.97.0); item names (v0.98.0); fights (v0.99.0: tables,
      monsters, effects, most words). v0.108.0: its temple (free, its
      own words), the shops' Take / Appraise / coins words, Appraise's
      halved values. v0.107.0: View's menu (all games) and
      Secret's View panel. v0.106.0: its monsters' abilities
      (no Turn in the original). v0.105.0: the spells' words (fight
      and camp), Heal / Harm / the Elixir of Youth / the druids' spells,
      item words. v0.113.0: the exploring menu's Move. v0.114.0: Alter and its Level. v0.115.0: the vault. v0.115.1: item names from their words. v0.115.2: View's coin choice on its panel. v0.116.0: Delayed Blast Fireball's delay. v0.116.1: the version screen. v0.116.2: the fight's colours (compared with DOSBox). v0.116.3: the remorhaz's swallow. Next, a chunk a commit:
      a dwarf's saves: the original's training recomputes the saves from
      the tables (Epona, DOSBox) and effect 0x61 adds the Constitution
      bonus at the roll (as the engine), yet the GOG save's Malachite (a
      dwarf F/T) kept his lower saves through a training in DOSBox -
      unexplained (secret_facts_2.md 28); approaching monsters (SPRIT1:
      sprites drawn as Curse's, not compared with DOSBox; the sky checked
      at the vault's door, v0.116.1); more behaviour tests (sweep's
      `behave secret` has THAC0, saves, training's hit points, the Level,
      the vault, the spells past 100, the cockatrice / remorhaz / iron
      golem - 239 checks, 0 failed, 2026-10-17; more of its monsters'
      abilities to add). v0.117.0: playable (Tom, 2026-10-18: Play on
      its game page). Done: the full checks - monsters (v0.120.1),
      spells (v0.121.1), items and situations (v0.124.0): 5,412 checks,
      0 failed; the ITEMTEST characters sent to Tom (2026-10-18).
      Still open: scroll bundles (Join on scrolls - not in the engine);
      Buy / Id prices, Share / Take / Pool, Look, the training hall's
      screens, the whole party dying, importing Pool (.WHO) characters -
      not checked yet; the unsure points in secret_facts_2.md 29.items.3
      / 29.situations.0b.
- [ ] **Pools of Darkness** (started 2026-10-17; facts: the Project's
      claude/darkness_facts.md; SPEC 6d). Done: the survey (program and
      overlay layout, files, saves, the scripts' operand counts);
      v0.118.0 the profile (Play off), scripts' counts, the title in its
      EGA colours. v0.119.0-v0.122.0: record, saves, party menu,
      Load, the 3D view and frame, the scripts' memory, Begin into
      Phlan. v0.123.0: event pictures (PIC1 animations, BIGPIC).
      v0.125.0: exploring menu, camp, View, item names. v0.126.0: the
      items screen. v0.127.0: the rule tables. v0.128.0: magic (camp
      screens). v0.129.0: sounds (ZOUNDS samples, SOUND op; Tandy = the
      SL / TL DAC, PC speaker as IBMSND.COM). v0.130.0: training (not
      compared with DOSBox: the hall's screen). v0.131.0: Secret's
      characters (Load / Add; Malachite's HP: done with creation).
      v0.132.0: approaching monsters (SPRIT1; compare with DOSBox when
      one is met).
      v0.133.0: the overland (matches DOSBox).
      v0.135.0: shops, temple, treasure, vault (open: the treasure-only
      result screen, Restoration's cheapest class, Remove Curse's words,
      Vault Items' way back, Raise Dead's stats recalculation).
      v0.136.2: the icon gallery shows Darkness's parts (VGA CHEAD /
      CBODY with their see-through plane). v0.137.0: camp Alter (Order,
      Drop, Speed, Level) and Darkness's own icon editor (camp, creation,
      the gallery's Edit) - matches DOSBox. Open: the party menu's Drop
      Character should ask "Pick Character Select" first (the same
      routine in the original); Darkness keeps the menu highlight's place
      in every menu - the engine does it in Alter / the editor only.
      v0.141.0: PLAYABLE (Play on its page; Tom, 2026-10-23: "finish
      Pools of Darkness") - its own level drain, camp spells, the gazes'
      pictures, Kalistes' weapon, the beholder's pick, menu highlights
      everywhere, the party menu's Drop pick, the overland fight's field
      (its own outdoor builder), Game Fixes: Power Word Kill, dragon fear,
      the bulette's leap, Remove Curse's slip, Resurrection's Con. Open:
      Protection from Fire in fights a plain effect; Secret's
      camp picture while asking whom (not checked).
      v0.136.0: fights. v0.138.0: the missing rules - Turn Undead's
      table, the 6th-9th level spells (Monster Summoning from a reserve
      of 10), dragon fear, Otto's, Kalistes' weapons, vorpal, blessed
      bolts, the gazes, beholders' rays, the burst sound, the Aim / Speed
      words (as DOSBox); monster records shared per group (Darkness's
      budget fight 145 KB). Open: the arena's third south ogre (31,19)
      against the original's (33,18) - its rank widths (DS 0xA7CE / 0xA7CF)
      are left over from an earlier count (facts R17); Darkness's own
      level drain (the engine uses Pool's with Darkness's words); the
      gazes' pictures (COMSPR 0x2D / 0x32); Kalistes' weapon on row 23;
      the beholder's ray target (the original's picker, ovr 0x23130); an
      Energy Drain check.
- [ ] Darkness 256 colours on the S3 / C5 (merged, branch darkness-vga):
      matches DOSBox's VGA mode. Open: the overland arena fight (jumper ->
      Arena -> Overland -> Prime, river No, monster 10 x 1) gives an
      all-river field with no monster and an empty frame after (EGA
      too; DOSBox: grass, the monster, the map back); the VGA fight's 40
      field tiles (23 KB) in internal RAM - one block would go to PSRAM;
      a Settings switch for EGA on the S3 / C5 (Claude: not now - Tom's
      rule is 256 colours there).
- [x] Tom's tests 2026-10-24 (v0.145.1 / v0.146.0): the C5's sound line
      stays at the middle (v0.145.1 - Tom to listen again); Gateway's
      ready-made characters (the SNEG copy's 12 .GUY; GAME.CFG's first
      folder line; no saved game ships with either release), Modify on new
      characters, the icon gallery's first page, Sling / Magic Hit sounds
      (the original plays the Miss sample - Game Fixes: the magic and
      missile sounds), the journal's Introduction (Gateway and Treasures,
      entry 'I' 1 - Rescan needed); a Loading screen on Play; one menu
      highlight rule for every game (white box, first word, the place kept
      after Select - SPEC 12); Treasures' Valhala Inn (a script CALL's
      operand - Darkness's Rolf tour fixed by it too). Tandy / PC Speaker
      in the recorded-sample games: only grain differs (told Tom).
- [x] Tom (2026-10-24): Gateway's Modify drew the changing number on
      Curse's rows - Gateway's View layout now (v0.146.1).
- [x] Slow game starts (v0.148.0): each program unpacked once into
      _CYD/<folder>/PROGRAM.BIN (by the scan, or the first Play on a card
      scanned before); QEMU: Play to title 6-35 s -> about 1 s.
- [ ] Tom: time a game's start on a board after Rescan (QEMU says ~1 s).
- [ ] Tom: Gateway test characters (gateway_itemtest.zip, 2026-10-24):
      into Gateway's SAVE folder, then Add Character -> ITEMTST1-3.
- [ ] Champions of Krynn (v0.147.0, SPEC 6g): Play on, in testing - plays
      end to end in the playsim; moons, gods, knights (petitions, Change
      Classes), kender Yell, draconian deaths (bozak, baaz), creation,
      Modify, the overland; matches DOSBox (title, party menu, View, the
      start); behave champions; budget fight 129.8 KB. Game Fixes: thief
      skills by race (the original reads Curse's race rows), a knight's
      saves (the original gives every knight row 7's level-0 saves).
      v0.149.0: its GOG journal table (88 entries, 57 tavern tales - the
      PDF reader now takes cross-reference / object streams; Rescan
      needed), the kapak's acid pool, "WHERE DO YOU WISH TO GO?" kept in
      shops / temple, its sound effects (the SSI driver in START.EXE, as
      Secret's). Open: the title music (SOUND.DAX / MIDI.DAX, not played);
      an in-game effect not yet compared with DOSBox.
- [x] v0.151.0: the journal viewer (Column / Whole, Turn for maps,
      pieces at their own widths - Treasures' introduction; the book a
      column at a time; grey print to black, paper threshold 232, cubic
      scaling; GOG map entries from the Companion's JE pictures when
      there; a Rescan remakes JOURNAL.DAT) and the party menu box
      (v0.150.0) from the list below. Tom: check Treasures' map J1.
- [ ] Death Knights (Play on, v0.153.0): the GOG journal table (v0.154.1:
      65 entries, 13 not in the book, maps 37 / 44); the playsim crashed
      started at JOURNAL instructions in areas 3 / 5 (RUNAT into blocks
      67 / 82 / 96 / 97) - check it isn't the engine; events block 16 (the intro) - a longer run or the sweep not
      replaying its fight per square; open: DK temple healing assumed free,
      effect 0x44 (feeblemind and drain?), Sivak morale / fleeing. Game
      Fixes: DK thief skills by race (each race its own row).
- [x] v0.154.2: the Krynn games' race adjustments at creation - the
      originals never apply them (the code tests a loop counter left at
      Human; DOSBox: Champions' kender Dex unchanged): Game Fixes on only;
      the young-Wisdom -1 (Secret's rule) now in Champions / Death Knights.
- [ ] Dark Queen (Play still off - Dev Tools; branch darkqueen4,
      queen_facts_4.md 9-12): in since v0.157.0 - the fights' two rows of
      buttons / status / Aim / Move's Undo Done, names' colours, flags /
      flags 2, monster xp (record 0x54 - checked), sound (SOUNDS.GLB), 256
      colours on S3 / C5, animation (0.5 s), Inv, Modify, `behave queen`
      (creation, train, knights, spells, monsters). Str00 test 50 confirmed.
      Plays from the start through Caergoth, the dragons, the Silver Shark
      and the cave's draconians. Branch darkqueen5 (queen_facts_5.md
      13-18): the Taladas overland (its script's 38 x 30 map, the marker,
      the strait crossings, cities, places, wandering encounters - checked
      in DOSBox), PROGRAM 9 back to the script (its inns), the icon picker
      (Alter, creation), Modify without the items, unarmed THAC0 without
      Strength, Knight New Order's refusals straight back, the camp's cast
      screens, the Sivaks' black row. Left: the overland map's colour
      cycling (the water); a city's place box shows "x,y" (the original:
      the time only); PROGRAM 1 (the gnomes' celebration, 58 / 60) passed
      over - not known what it does; the events sweep's fights in blocks
      36 and 63 (a few of the party against dragons / 17 monsters) ran
      600+ rounds without an end (god mode - check they end in real play).
      Play is on (darkqueen5); the flasher's list says "in testing".
- [ ] Tom: play the Dark Queen (Play on since darkqueen5): the Taladas
      overland (arrow keys step; the turns are the diagonals ahead), its
      cities and places, the icon picker (Alter -> Icon, and after a new
      character's name).
- [x] 5.0" 800x480 IPS capacitive S3 boards (Tom: a Hosyond and a
      Freenove FNK0115Q, 2026-10-30): board file built (branch board5in,
      env `cyd50s3_cap`, `src/boards/esp32s3_50_rgb.hpp`; untested -
      Tom's checks under Tom). RGB panel (LovyanGFX Bus_RGB, 13 MHz, frame
      buffer in PSRAM) with the 320x240 layout at 2x, centred (640x480,
      black bars); GT911 polled without INT; SD over SPI (FSPI); NS4168
      by I2S (`BOARD_AUDIO_I2S`); serial on UART0 (CH340C). QEMU:
      `qemus3_50` (2x copies checked: 0 bad pixels through Home, Settings,
      Rotate 180, Swap Red/Blue, Invert, Pool's Play). Facts:
      board_5inch_facts_2.md (scratch -> Project).
- [ ] Secret J45's piece takes in the "Journal Entry 47" heading (the
      map overlaps it; a rectangle can't separate them).
- [ ] Check: pdfview's State (FileSource cache, >4 KB) lands in PSRAM on
      the S3 - its card reads must go through the internal bounce.
- [ ] Tom's tests 2026-10-24 (12:14): Treasures' party menu shows no
      highlighted character (View then shows whom?); the C5's sound still
      distorted, harsh, like static (v0.145.1's fix didn't cure it); the
      Treasures introduction entry in two pictures - the full-width first
      paragraph, then the rest ("Dear Friends...") at one column's width;
      the journal viewer for ALL games: scans are whole facing pages - cut
      into columns (two a page), default one column wide, Whole for maps,
      no "Full Size", a 90-degree turn for map entries filling the screen;
      Treasures' first map illegible - clean it up (Tom: replace its text
      with readable font text - not allowed: the journal's text can't be
      shipped; cleaned-up picture / the Companion's own pictures instead);
      Settings' Volume as one key: tap left -, right +, 5% steps, the small
      "Volume" label, the percentage between - and +, a mute symbol at 0.
- [x] v0.149.1 (Tom, 2026-10-24 12:41: C5 sound fixed in every playable
      game): the C5's Curse / Secret sounds were also distorted - the
      output (PDM up-sampling / quarter scale).
- [ ] Tom (2026-10-24 12:57): review in testing - the exploring / camp
      panels keep the originals' white name for the selected member (the
      party menu has the white box since v0.150.0); box them too?
- [ ] Tom (2026-10-24, 12:21; 12:34 Settings: one Game Sound key naming
      the mode, tap to change - Tandy / PC Speaker / Darkness / Gateway /
      Treasures by what's on the card; in those three games just the game;
      Volume -/+ wider, half-width Mute toggle - done v0.150.0; the game
      modes play Tandy until the recordings are wired). Tom's idea:
      the later games' better sounds on the early games - Claude's plan: a
      Game Sound choice "Recorded" that plays the player's own Pools of
      Darkness (or Treasures / Gateway) ZOUNDS.DAX samples for Curse, Pool,
      Secret and Champions' effects, when one of those games is on the card
      (effect numbers already mapped as Darkness's; the player's files read
      at run time, nothing shipped). Built (branch rec-sound): the three
      modes play the recordings (`Clips::lent`, recorded_sound_facts.md).
      Tom to listen: Curse / Pool / Secret / Champions in each mode - the
      step (Darkness's pad 0.2 s, Gateway's / Treasures' a 16 ms tick),
      hit, miss, sling (the arrow), magic hit (the stars' sound), death;
      the title and anything else stays Tandy.
- [ ] Krynn (survey 2026-10-24, the Project's claude/krynn_facts.md):
      Champions = Secret's engine (port started, branch champions); Death
      Knights = Darkness's engine at 4 bits (LOAD FILES / PIECES 3
      operands, a 216 + 220-byte record); Dark Queen = a new C program
      with HLIB .TLB / .GLB libraries, only the scripts (Darkness's ECL)
      shared - a near-new port. Dark Queen on classic boards: chosen
      2026-10-24 (above: the VGA files drawn in 16 colours through their
      own EGA tables). Also: the Krynn games' sound is an EA driver
      (SOUND.DAX / MIDI.DAX), not decoded yet.
- [ ] Gateway to the Savage Frontier (v0.144.0, SPEC 6e): PLAYABLE (Play
      on) - matches DOSBox through Yartar, the wilderness, the vault, shops,
      fights; its GOG journal table (v0.142.0). behave gateway is light
      (creation 1084 cases, 48 menu states) - Claude: a deeper behaviour
      pass (spells, monsters, items, as Curse's sweeps). Open: music
      (MUSIC.DAX, sounds 0x80 up - no game plays music yet); the
      wilderness start selects ARNOLD where the original has MERL.
- [ ] Treasures of the Savage Frontier: PLAYABLE since v0.145.0 (Play
      on) - the 256-colour parchment, the clock's years and old age, the
      journal (83 entries; the PDF reader takes several pictures a page),
      the camp picture, LUCK texts as DOSBox; late-game screens checked
      in DOSBox from edited saves. Game Fixes: everyone ages a year each
      game year (the originals - Darkness too - age no one until year 100).
      Open: the VGA colour cycling of PIC 130's crown sparkles (no cycle
      facts yet); the post-fight old-age texts in DOSBox.
- [x] Journal tables (v0.142.0): Secret 72 entries, Darkness 99 (its
      book has no entry 13), Gateway 62 - checked by eye; Darkness's
      mentions now offered at a key wait too. Small leftovers: neighbouring
      text at the edges of four maps (Secret 45, Darkness 36 / 38, Gateway
      7). Open: Treasures' GOG PDF (72 dpi, several pictures a page) needs
      the PDF reader to take more than one image a page first.
- [x] Darkness's fight memory (v0.136.1): the 30-monster test fight 169
      -> 150.6 KB (Curse 146 -> 132, Pool 147 -> 133): the script's code
      parked during fights and its memory the fight's arena, the monster
      groups made as needed, the spell names packed, smaller fighters,
      Darkness's background picture freed. Only 3 KB spare: next cut the
      monster records (~540 bytes each, up to 48) - a group's copies
      sharing one, or growable effects (108 bytes each).
- [ ] Darkness's sounds, with its fights: the area spells' burst plays
      effect 11 again as their picture shows (spells 0x22, 0x2F, 0x40,
      0x57, 0x5B, 0x73 - GAME.OVR 0x29D34 +0x1451; and an item's burst,
      +0x5FB8); a ranged attack's one sound: 12 for the item types 4, 5,
      7, 0x0C, 0x1E, 0x42, 0x4A, else 9 (+0x2800 of GAME.OVR 0x1E642 -
      check Darkness's shot kinds give that). darkness_sound_facts.md 4.
- [x] Tom (2026-10-23): yes - Darkness's race adjustments and the
      cleric's experience bonus from Wisdom 16 behind Game Fixes.
- [x] Tom (2026-10-23), Darkness's fights: Power Word Blind's worked-out
      time behind Game Fixes - yes; Energy Drain stays one level (no
      switch); Power Word Kill and Resurrection as AD&D behind Game Fixes;
      dragon fear ends with the fight behind Game Fixes. (Claude: the
      last three on branch darkness-leftovers.)
- [x] Tom (2026-10-23): the bulette's leap - the original's way (never
      happens) in the game, the working leap behind Game Fixes; the same
      answer for every future question of this kind (CLAUDE.md).
- [x] Darkness's sounds (Tom, 2026-10-23): Claude's recommendation -
      two choices, Tandy = the recordings as they are (the SL / TL DAC,
      the GOG Sound Blaster), and the PC speaker; no EX / TX chip choice
      (it differs only by a little grain). The game goes on while a
      sound plays (Tom OK, 2026-10-19).
- [ ] Tom: flash v0.104.0 on the 4.0" S3: the touch (keys now light as
      the stylus touches; light taps after a pause, finger and stylus;
      small keys) and the sounds (Settings' sound test - Step, Hit; Pool's
      steps and fights). Also on a resistive board: keys light on touch
      there too. Sounds: Step and the others work now (Tom, 2026-10-16) -
      still to say: any hum or hiss between sounds on the 3.5" now that
      its amplifier stays on? If taps still land off
      the mark, Settings - Recalibrate Touch (new on this board: tap
      each arrow tip) and say whether it helped. Then the yellow tint:
      the display's set-up is the same as the maker's (same gamma and
      voltages), so please try Settings - Brightness all the way up, and
      say which firmware looked right (Freenove's demos?) - the tint may
      be the backlight at 78% (our default) or the engine's own colours.
- [ ] Claude: decode any crash line in Tom's S3 logs (CI's
      elf-cyd40s3_st7796_cap for that build, xtensa-esp32s3-elf-addr2line).
- [ ] Claude: watch "stack left" in Tom's classic-board memory logs (QEMU:
      1.5 KB at a fight's start); slim the deepest frames if it drops
      under ~1 KB (fight_load_monster 1.2 KB, draw_overland 1.4 KB,
      load_party_text 1.3 KB, save_game 1.1 KB).
- [ ] Tom: v0.104.2 - Pool's walls round Phlan's gateway / the
      temples, and Center in a fight (Aim, Manual, an empty square).
- [ ] Tom: Secret test characters (2026-10-18, secret_itemtest.zip):
      its files into Secret's folder on the card beside SAVGAMA.DAT
      (README inside), then Add Character -> Secret -> ITEMTEST1 ... 6
      (every magic item of the game between them); say what looks wrong.
- [x] Tom (2026-10-23): the switch is **Game Fixes** (was Game Fixes) -
      the originals' behaviour in the game, the engine's fixes behind it
      (v0.139.0).
- [ ] Claude: Secret's Barkskin - the original runs Heal's routine for it
      (the AC effect AND nearly full hit points). Tom's rule (2026-10-23:
      "Keep original game behaviors in the game and keep our fixes under
      the ... toggle"): fix it (AC only) behind Game Fixes.
- [ ] Tom: try the new library page (v0.103.0) on a board - the art,
      the header keys, the thirds and the triangles.
- [ ] Tom: when you have a moment, Secret's Screen Test and Walk Test
      (Settings - Dev Tools on its page) on a board - the frame, title,
      credits and maps should look like the original's.

## Done

- v0.116.4: Secret's temple Raise Dead checked in DOSBox (Con - 1, own
  and in use; the maximum HP kept) and made so.
- v0.112.0: Secret's sound effects (Tandy / PC speaker) from its program's
  driver, as Curse's.

- v0.111.0: Secret's Create New Character (its words, races, rolls, 200,000
  experience, silent training's spells) and Modify (Add / Subtract).

- v0.110.0: Secret's Add Character - its own characters and Curse's
  (without items or money, as the original takes them in).

- v0.109.0: Secret's spells past 100 - Death Spell, Disintegrate, Globe,
  Stone to Flesh, Flesh to Stone, Delayed Blast Fireball, Mass
  Invisibility, Power Word Stun; its monsters' magic resistance.

- v0.108.1: Secret's training - free, its own hit dice, demihuman
  clerics' limits; the temple's list with the shop's bar.

- v0.108.0: Secret's temple - free, each cure at once with its own words
  ("can see!", "is ressurected" ...); its shops' Take / Appraise words,
  Appraise's values (Curse's halved), the priest's / shopkeeper's coins
  words.

- v0.107.0: View Character's menu in every game - Spells, Trade, Drop,
  the paladin's Heal and Cure (they weren't in the engine); Secret's View
  shows its money and the readied weapon / armour.

- v0.106.0: Secret's monsters - iron golem, cockatrice, remorhaz,
  pyrohydra, umber hulk, the dragons' breaths, the giant slug's spit, the
  dreadlord's immunities.

- v0.105.0: Secret's spells say its own words ("is Blessed", "falls
  Asleep", "is Hastened" ...) in fights and camp; its Heal, Harm, Elixir
  of Youth and the druids' Charm Person or Mammal / Cure Light Wounds;
  its item words (Use ... Select).

- v0.104.2: Pool's area 3 walls (the "unsettled areas" round Phlan,
  the Temple of Tyr) from their own tiles - they were drawn from other
  areas' tiles since v0.85.2 (Tom: "made up of tiny icons"); Center in
  fights centres the view on what's aimed at, Manual's empty squares too,
  and the Manual square keeps the view with it (Tom).

- v0.104.1: the ESP32-32E boards' amplifier stays on from Play's start
  (its ~100 ms wake-up ate Step and Hit on Tom's 3.5").

- v0.104.0: keys and game-screen words light up as the stylus touches,
  not as it lifts (Tom); the tap still acts on the lift.

- v0.103.6: the S3's touch kept in its active mode; its sound output and
  amplifier stay awake (the amplifier's 100 ms wake-up ate short sounds);
  every board's amplifier off only after the sound's end has played.

- v0.103.5: the S3's touch read by a task of its own every 10 ms (quick
  light taps were missed while the loop worked); Recalibrate Touch on
  the S3 too.

- v0.103.4: the S3's touch polled every time (its INT pin no longer
  gates it) and reset as the maker does; a capacitive press starts on
  its first reading; the S3 marked tested (Tom: Pool, Curse, Secret
  play on it).

- v0.103.3: the crash at Play on the S3 (and Secret's): loop()'s stack
  ran out - play::open split into steps (the classic boards had under
  200 bytes to spare there too; now 2.1 KB), 16 KB of loop stack on the
  S3 / C5; the memory log shows the stack left.

- v0.103.2: the library's pictures only from the files the scan made
  (Tom) - a start without them for this screen makes the missing ones
  first; the page never decodes.

- v0.103.1: S3 fixes from Tom's first run - the codec set up from the
  main task, the card's DMA kept out of the PSRAM, crash lines in the
  Restarts log; the library page's triangles come first; the picture
  files' problems noted in the Memory log; the S3 in QEMU (`qemus3`).

- v0.103.0: the library page as Tom picked it - full-width art,
  top-justified, Rescan / Settings in the header, tap thirds (round),
  faint triangles.

- v0.102.0: the SNEG Launcher's box art on the library pages (Tom), the
  game's name centred over it.

- v0.101.0: Secret's camp magic - 7 spell levels, spells 101-117,
  its names, words, counts and Display as the original's.

- v0.100.0: Secret's start (the opening texts, block 17), the walls of
  Priam's house (LOAD PIECES 127 kept the set), the treasure and camp
  screens as the original's; " CAMPING" on the position line (Curse too).

- v0.99.0: Secret's fights - its tables, monsters (439-byte records),
  their effects in its own numbers, most words; Turn left out until its
  table is found; CALL 2DCB redraws the 3D view.

- 2026-10-12 (Tom): the C5 boards work with v0.94.1 (Home, the scan, a
  Secret Play Test) - marked tested.

- v0.98.0: Secret's item names (the records' own) and its word table /
  types.

- v0.97.0: Secret's View Character in its own layout, pixel for pixel
  the original's (DOSBox, the GOG party).

- v0.96.0: Secret's scripts read and write a character through its own
  field switch (race in its numbers, HP, levels, gems, status).

- v0.95.0: Secret's rule tables in its own shapes (levels to 18, 7 spell
  levels, 119 spells, its race order) - the GOG party's values come out
  of them; a ranger learns only the druid spells of levels he has slots
  for (Curse: the same, its druid spells are all 1st level).

- v0.94.1: the C5 hung at Home drawing the first icon (Tom's logs, both
  C5 boards): the drawing held the SPI bus the card shares there. Card
  reads while drawing (icons, journal pictures, the PDF book) no longer
  sit inside one display transaction on that board (`ui::card_draw_begin`).

- v0.94.0: Secret's party menu, Load Saved Game and Begin (Priam's house
  at 3,3 S); script blocks found across ECL1-3; the Play Test opens under
  Dev Tools.

- v0.93.1: Secret's race numbers (human 6, Curse's 7) turned into Curse's
  when read, back when written.

- v0.93.0: Secret's characters and saves - its 439-byte record in and out
  of the engine's (Curse's) layout with an extension for what has no
  place there; saves without the script's bytes; .STF items (67 bytes),
  .SFX effects, .WHO characters.

- v0.92.0: Secret of the Silver Blades - its scripts decode (Curse's
  opcodes but 0x3C / 0x41), its frame bars (tiles 52-59 through a tile
  window), font (8X8D5), title, credits; Screen Test and Walk Test work.

- v0.91.1: the platform layer - the game code (src/game: play, fights,
  Screen / Walk Tests) and the engine use only src/platform/*.h (time,
  log, memory, files, sound, memory log); boards: hal/platform_esp32.cpp;
  PC: tools/host (the playsim and sweeps run on it; CI builds the game
  code with no board headers and tests the PC side).
  Found by its tests and fixed: a script's treasure with no fight (COMBAT
  with no monsters - Curse ECL1 block 80) crashed since v0.86.1 (the
  fight's fighter blocks weren't made for the party).
- v0.91.0: one Settings screen everywhere (Home, the game's page, the
  Game menu's Settings tab): board keys + comforts + the game's own.
- v0.90.0: Tom's questions on the Cloak of Elvenkind (95 in 100 is the
  originals' own code - answered) and the Flame Tongue: the Game Fixes
  switch (Pool's Flame Tongue as AD&D, files kept as the original's), the
  Flame Tongue's blows fire in both games; Settings switches in pages.
- v0.89.1: every item of both games tested (equip, the class / alignment
  / Strength rules, to hit, damage, AC, saves, stats, effects, use, fights;
  Curse 4069 checks, Pool 2738) plus the ITEMTEST characters themselves
  through Add Character (Curse 204, Pool 176 - each item readied alone,
  each usable one used, a fight with them all): fixed Pool's sword +3 vs
  undead and the Cloak of Elvenkind (both did nothing extra).
- 2026-10-12: Secret's files checked with the engine's readers: every DAX
  block decodes, 16 colours; the differences listed in the Project's
  claude/secret_facts.md.
- v0.89.0: Pool's items - its own item codes (generic effects, Ogre Power,
  the boulder, keyed swords), its item spells (0x17 higher: potions,
  wands, the javelin, the necklace now work), Enlarge / Strength / giant
  strength, Keoghtum's Ointment, the Manual of Bodily Health, the
  Spiritual Hammer's item, Strength / Charisma in Pool's files. The
  magic-user's spell pick at training was already compulsory (no Exit).
- 2026-10-12: README.md's board table has the S3 and C5 rows (after Tom's
  README edits); its Logs paragraph mentions Send Logs.
- v0.88.0: ESP32-S3 4.0" capacitive and ESP32-C5 2.8" resistive builds on
  the flasher (untested); Tom decided the C5 boards get a build.

- 2026-10-12: Tom decided the boards / games question (SPEC 5d); boards'
  facts in the Project (board_facts.md); the VGA games' own EGA modes
  checked (Darkness in DOSBox).
- 2026-10-12: DOSBox - the mountains in Pool's wilderness don't stop the
  party in the original (12 hours a step, as in the engine, which already
  matched): poolrad_overland_facts.md 7.4.
- 2026-10-12: the other games surveyed (Project: other_games_facts.md).
- v0.87.3: room for fighters joining mid-fight; Hit / Miss always sound.
- v0.87.2: flasks of oil burn.
- v0.87.0-v0.87.1: 4-bit screen (32 KB), lighter fights, heap budget.
- v0.86.0-v0.86.1: Send Logs, Logs in the Game menu, no Esc, memory recovery.
