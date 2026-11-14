# Open tasks (Tom and Claude)

Tom, 2026-10-12: "start keeping a list of tasks we need to do - both you
and me." Kept up to date with every push (CLAUDE.md); finished items go
to the bottom with their version, then drop off after a while. Synced to
the Project as claude/TODO.md.

## Tom
- [ ] Try v0.196.6 (Tom, 2026-11-11: "my characters ... fail to ready
      their missile weapons and charge into melee"): Quick with Missile
      ticked now readies a loaded bow / crossbow / sling whenever no foe
      is next to the character, at every step of its turn. Found: the
      originals (and the engine until now) choose the weapon once, at the
      turn's start, and by a rating that keeps a much better sword over
      the bow - so a fighter who started its turn next to a foe walked on
      to the next one sword in hand. Your save F's fighters (+5 bows,
      arrows readied) shot in every test fight from it; tell me where it
      still happens (thrown things - darts, oil, holy water - stay by the
      games' rating).
- [x] 2.8" ILI9341: no sound (Tom, 2026-11-08) - silent in Classic-Games /
      Classic-Arcade too, the 3.2" and 2.8" ST7789 fine. v0.179.2: GPIO 4 (the red LED's pin)
      is the amplifier's enable on the 2.8" ILI9341 only (v0.179.1 had both), low from Play's start (a
      CYD with the same silence played once 4 was low - CYD issue #372). Tom:
      flash v0.179.2 and listen; the red LED on the back lights while playing.
      Tom, 2026-11-08: v0.179.1 plays sound on his 2.8" ILI9341 - fixed.

- [ ] "Game Enhancements" (Tom, 2026-11-01: "We may need to have another
      toggle/list in addition to Game Fixes ... I'll think about that"):
      should additions like Pool's camp Fix get a list of their own
      beside Game Fixes? Meanwhile Camp Fix sits with the comforts
      (Settings key Camp Fix, features.h CYD_CAMP_FIX). Don't ask again -
      Tom's to decide.
- [ ] Try (2026-11-01, not pushed yet): Pool's camp Fix (a hurt party,
      a cleric with Cure Light Wounds); the Game menu's Cheats tab -
      Healer (the temple's cures free), Merchant (every shop's goods,
      Next / Prev through them; Curse's half price), Sanctuary (a rest in
      Phlan's slums the watch can't break). Cheats work at the exploring
      menu only; Settings - Cheats Tab Off hides the tab.
- [ ] Try (2026-11-06, not pushed yet): Cheats - Trainer (the game's
      training hall party menu: Train any class free, Modify on anyone)
      and the Max Hit Points switch (Cheats tab or Settings, off by
      default: training's hit dice at their highest). The Trainer's Modify keeps a trained character's own hit points as
      the highest allowed when the game's bound (made for new characters)
      is lower (Claude's choice, v0.169.0).
- [ ] Try (2026-11-06, not pushed yet): fights at game speed 0 - the
      engine waited 0 as 4 everywhere; now 0 waits nothing, as the
      originals (playsim: Curse's test fight 116 s -> 11 s, Pool's 181 ->
      9 s). A new game starts at 4 (Pool 1), a saved game at its own.
- [x] Spell lists open on the first spell with Memorize lit, the highlight stays after Memorize, "Memorize these spells?" on Yes, Pool's rest menu lit by whole words, a menu left by Exit starts on its first word (the shop's Buy) - Tom, 2026-10-31 (v0.161.2)
- [ ] Companion Views (2026-11-01, Settings, on by default): 480x320 -
      tap the map beside the game: Party (icons, HP bars, effects), then
      in a fight Battle (the whole field: green party, red foes, grey
      walls, blinking asleep / held / down, the screen's part outlined
      blue); a fight starts on Battle. 320x240 / 5.0": the Map key in a
      fight (also in the manual Move / Aim row, before Game) shows the
      battlefield. Are 3 px squares on the 480x320 panel big enough (the
      field is 50 x 25 in every game)? Effects words enough, or symbols?
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
- [ ] Try Quick Actions (2026-11-09, not pushed yet; SPEC 11): the Game
      menu's Quick tab (Exit Game is a narrow two-line key now) - since
      Tom's note the same day an engine page on the whole screen (names
      down the left, NPCs marked; the chosen one's switches as keys with
      boxes and full words; what the last tapped does under them; "?").
      Check it reads well on both sizes and the 5.0"; tap a name, flip
      switches with the stylus (each lights as it touches); the "?"; a
      fight: set a fighter to Melee off (it guards beside a foe), Move off
      (it holds its square), Guard off (its turn just ends), a cleric to
      Turn Undead off, someone to Bandage First (a dying friend bandaged
      first thing in its turn), then Quick; the switches follow a
      character into another save / the next game (QUICK.TXT on the card,
      readable). Settings - Quick Actions off hides the tab and plays
      Quick as the originals. Also new (a fix, round 12): the computer
      (Quick members, NPCs) bandages in its move step as the originals,
      not first thing; PoolRad's never bandages.
- [x] Quick Actions' Spells column (2026-11-09, Tom: "I could have sworn
      that Quick casted spells"): it does - with the fight's Magic On
      (Alt-M; off at every fight's start), in all nine games (each AI's
      spell gate; DOSBox: Curse, PoolRad). Spells ticked = Magic On for
      that character: Quick casts its memorized spells (not pushed yet).
- [x] Tom: Spells starts UNTICKED (the originals start every fight with
      Magic Off) - Tom, 2026-11-09: keep it ("matches the original game's
      behavior"; players come to the tab to change that).
- [x] Quick Actions for the party's NPCs (Tom, 2026-11-09): every party
      member on the tab, NPCs marked; theirs start at the originals' AI
      (all but Bandage First, Spells ticked) - unticking limits them.
- [x] Tom: the Quick tab shows one character's switches at a time - Tom,
      2026-11-09: no "All Characters" key (the tab is for customizing
      characters one by one).

## Claude

- [ ] Sweeps' order (Tom, 2026-11-10, 5.0", PoolRad, Podol Plaza goblins:
      one swing, the first skull, then all the swept goblins gone at once,
      no more sounds or rolls). Not reproduced: Tom's own party (his save
      G, speed 1) in the playsim's fights (12 sweeps) and in the REAL 5.0"
      firmware in QEMU (runat 85F9 + "runat FFFF" = all Quick; the
      emulator build logs each page / event with its time) - every blow
      its own attack pause, roll, hit page, fall + skull, field put back,
      then the next. Asked Tom for his firmware version and how he
      attacks (Aim / Target, a step into the goblin, Quick).
      Tom (19:00): his Speed is 0, he attacks by Move + a step into the
      foe (and on Quick). At speed 0 the pages wait 0 ms and our attack
      pose 10 ms (SPEC 12: the originals' Delay(100) is fixed) - a 3-goblin
      sweep is over in ~0.3 s. Open, asked Tom: (a) the originals' fixed
      100 ms attack pause back; (b) "a tap moves on" in fight pages is the
      engine's (v0.32), not checked against the originals - check / follow;
      (c) capacitive taps are queued (cap::sampler) - several taps act on
      several pages in a row.
      (b) decided (Tom, 19:05): "a tap on the combat area should end Quick
      for all characters. It shouldn't be skipping game events or display
      events" - v0.195.1: a fight tap skips nothing, takes the party off
      Quick (SPEC 12 "A tap in a fight"); checked by fightfuzz FUZZTAPQ
      (R14, nine games, and R14 fails with the hand-off taken out). (c) is
      now moot for pages (a tap no longer moves them on). (a) decided (Tom,
      20:05): no - the attack pause stays ours (10 ms + 10 a speed step).
- [x] Status lifecycle round (Tom, 2026-11-11: "statuses ... enabled
      when they should be ... disabled when they should be ... don't last
      or hang too long or not long enough"; no firmware change needed):
      fightfuzz R17 - every effect against the game clock's minutes (a
      minute a round; time 0 for ever; at or under the step it ends; the
      Dark Queen's clock at the round's start): too long / a wrong count /
      gone in a minute nobody acted; R18 - the fight's end takes exactly
      the game's own list (+ charm / dragon fear with Game Fixes) and keeps
      the rest; monkey P8 (P8FX=1 puts timed effects on) - out of fights
      the same clock rule. Results, nine games x two seeds: thousands of
      countdowns and run-outs, 0 broken. Also found: party_check never kept
      `s.hp`, so P1's wrap, P3 and P4 (checks against the last look) never
      ran - fixed, they pass.
- [x] Companion: Charmed after the fight / Dirten Drained (Tom,
      2026-11-11, PoolRad, Valhingen's vampire; his save F): v0.196.4 -
      the charm's effect ends with the fight under Game Fixes (PoolRad's
      own end keeps it; a loaded save's goes too); Dirten was never
      drained - his NPC record (MON1CHA / MON3CHA 107) has 0xFF in the lost
      levels / HP, counted down by his training and Restorations to 250:
      Drained counts 1-127 only, and with Game Fixes 0x80+ is none for
      Restoration, training and drains (SPEC 12). Checked on his save in
      the playsim (Party view: all OK), R16 / R16NEG, host test.
- [x] Drained in the Companion's party view (Tom, 2026-11-10, v0.196.0):
      red, also for characters drained before (their saved lost levels).
      Checked: fightfuzz FUZZDRAIN=1 (every monster drains) fight_check R15
      - each drain shows at once (PoolRad 39, Darkness 41, Treasures 178,
      Knights 144, Queen 111; 0 missed), host test test_drained (saved
      PoolRad / Curse / Darkness records), a playsim snapshot (PoolRad save
      with one member's lost-levels byte set: "Drained" in red).
      Curse, Secret, Gateway and Champions have no fight-time level drain
      in the engine - as their originals: their programs have no drain
      words at all (GAME.OVR / EXEs; PoolRad "is drained", Darkness /
      Treasures / Knights "gets drained" - the games the engine drains in).
      Their Restoration still serves imported drained characters.
- [x] Treasures K8 (fightfuzz FXCHECK, seed 5 fight 1): with Show Rolls
      on, a blow that met a mirror image sent the attack back to the
      original's pages, and a miss beside it lost its roll - v0.195.2: one
      page a blow there too (Secret / Gateway / Treasures fuzz clean).
- [x] Flash check (2026-11-10, all games, seeds 3 and 7: 0): the rest's
      clock row no longer counted; the Dark Queen's box frame cells at a
      row's ends no longer make a cleared row "shown" (its one report was
      that). v0.195.2: an encounter rolled on a rest step that had its own
      message ("has memorized ...") is told after that message's wait, not
      over it (same dice; stopping the rest then still lets it break in).

- [x] Messages too quick to read (Tom, 2026-11-10, PoolRad's training
      "Not Enough Experience"; v0.194.0): Readable Messages (CYD_READ_TIME,
      Settings key, on) - the menu line's notes, camp spell lines, the
      rest's words, a script's pause after its text, PoolRad's treasure
      lines stay at least 1 s or until a tap. sweep flashcheck (devkit
      flashcheck.sh): originals' timing 88 finds over 2 seeds x 9 games,
      with the comfort 0 over 3 seeds x 9 games. Fights keep the Speed
      setting. Not reached by the monkey: every script's own pauses.
- [x] Fights and Readable Messages (Tom, 2026-11-10): fights' pace is the
      Speed option's - Readable Messages stays out of fights.

- [x] 5.0" Companion Strip (Tom, 2026-11-10; v0.192.0): Settings switch
      (on by default), the game against the left edge, the strip 1:1 -
      Party y 0-218, map / battle y 220-377 (map lines and legend cut on
      every board, battle's round line gone, legend at the bottom).
- [ ] Companion buttons for the 5.0" strip's y 381-479 (Tom: to design);
      what the 5.0"'s Map key does with the strip on; taps on the strip
      (dropped for now). Tom: try the strip on the 5.0" (both Rotate 180
      settings - the strip stays at the right as you look at it).

- [x] Short statuses without full stops (Tom, 2026-11-10; v0.192.0).

- [x] Party view class / level line (Tom, 2026-11-10, v0.191.0): classes
      left ("Fighter 7" when it fits, else "F7/M9"), the state right,
      short forms when tight, colour coded; 2 px between members.

- [x] Screenshot findings (2026-11-10; v0.192.2): Darkness's 3D view
      lost its colours after a Companion tap - an NPC's picture loaded for
      the Party view sent its palette (the screen's now put back); the
      fight Map view's "Party menu" heading went with v0.192.0's map lines;
      the scan's "newer format - not playable yet" for Queen removed.
- [x] Queen outside Cergoth (save D) drew a black 3D view (v0.196.1): a
      save made in a 3D area loaded no map (the resume path wanted wall
      blocks its saves don't keep) - black view, steps going nowhere.
      Now its map again; checked against DOSBox (the original on the same
      save: the forest, F -> 4,2 at 11:01 - the engine the same), every
      other game's save D loads its map, Queen saveload / monkey clean.

- [x] Round 12 rule fix (2026-11-09, Tom: "the original does Bandage
      last"; with Quick Actions, not pushed yet): the computer's
      bandaging, found in each original's code - the bandage routine by
      the fight menu's 'B' (al = 1), its other al = 1 caller the AI's
      move-and-attack step, first thing there (party side, then moves 0):
      Curse GAME.OVR 0xC829 in 0xC7EC (the AI 0xBA77 calls it at 0xBC69
      after its target, moves and in-fight tests; routine stub ed:101,
      menu 0xB6DC), Secret 0xDC94 (151:10B, menu 0xCC01), Gateway 0xDEC1
      (FE:FC, menu 0xC097), Treasures 0xB575 (105:101, menu 0x9B17),
      Darkness 0xA71B (EC:101, menu 0x8E7E) - as Death Knights 0xAF23,
      Champions 0xD2A9, the Dark Queen 0x2E98B. PoolRad: the routine
      (GAME.OVR 0xA806 = overlay 0x9807 +0xFFF; its stub seg 0x35 exports
      only +0x7A / +0x127A) is called only near - 0xA7CD (the menu's B,
      al 1), 0xA11E / 0xA6D0 (al 0: the dying checks) - so its computer
      never bandages. The engine bandaged at the turn's start in the six
      DAX games: now the move step everywhere, none in PoolRad
      (play_fight.inc ai_step); rules4 bandorder on all nine (9/9).
      For the Project's rules facts: claude/rules_round12_facts.md (to write).
- [x] Memory audit of the screens that load rule tables (Tom, 2026-11-07:
      "Not enough memory sounds like a real problem"; not pushed yet):
      every screen in all nine games measured from save A (playsim
      SCREENMEM + heapprof) - none past 145 KB of the 150 KB budget, none
      left memory behind; the loads now make room first (make_room 8 KB,
      the magic tables may go at the party menu), log the numbers when
      they still fail, never keep two copies (the fight's drain table
      leaked the magic tables when they were loaded). Figures in
      CLAUDE.md's memory rules.
- [ ] Maybe later: the Dark Queen's creation Modify holds two Makings
      (134 KB, +24.5 KB) - Modify could borrow creation's tables.

- [ ] Side-steps (2026-11-06, Tom: Pool's city hall / Kuto's Well asked
      and held the party on a move away): a side-step's step script now
      sees the step's direction (SPEC 12). Pool checked by autodiff
      around both places; every other game's play test must check
      side-steps beside facing-dependent step scripts (doors "locked",
      wells, "you can't go that way", NO-MOVE 0x7EC9 squares) - a
      side-step away must not ask, a side-step into one must ask as a
      step forward that way does.
- [x] Rest messages (v0.196.2, from PoolRad's / Curse's code): every
      message of a step shown, each with its game delay (the engine showed
      a step's first only); the name (party-list colour) and words on row
      19, the spell on row 20 - read in all seven DAX games but Death
      Knights (v0.196.5, oracles/restmsg.py: the same routine in each).
      v0.196.7: Death Knights - Champions' routine byte for byte (GAME.OVR
      0x2F136; its words DS 0x257F / 0x2594, called from 0x280DC /
      0x28208); the Dark Queen in DOSBox (save A, camp at the Palanthas
      inn, a cast Detect Magic memorized again): "JORIS THE WISE HAS
      MEMORIZED" on row 19 in the text colour, the spell on row 20, each
      member's message in turn.
- [ ] Game speed (2026-11-06): the rest messages' pause - DONE v0.196.2:
      the game delay, read in PoolRad's and Curse's rest routines (SPEC
      12 "Game speed"; the later games taken as the same); Gateway's / Treasures' /
      Dark Queen's letter wait (taken as speed x 3 ms) and Dark Queen's
      game delay (taken as x 100 - its program isn't Turbo Pascal).
      Cheap draw win if needed: dirty columns as well as rows (a flight
      step pushes 320 / 480-wide rows for a 24 px picture).
- [ ] Cheats' Trainer / Max Hit Points (2026-11-06): playsim-checked in
      all nine games (TRAINTEST / MODTEST / MAXHP); the Cheats tab's two
      columns of keys (5 keys) only built, not seen on a board.
- [ ] Cheats tab (2026-11-01): checked in the playsim for Pool, Curse,
      Secret, Darkness, Treasures, Gateway, Death Knights (Healer /
      Merchant / Sanctuary) - not yet for Champions (its save A loops in
      the Solace camp / city menu) and the Dark Queen (its save starts in
      Palanthas's city menu; its Merchant reads the SHOP lists but not
      the scripts' price words - 16 assumed). The 320x240 / 480x320 tab
      and keys only built, not seen on a board (QEMU has no screen).

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
      Round 3 (Tom, 2026-11-01: "did you check if they were correct?"):
      `behave levels` parts values / multi check every value at every
      level against each game's own program tables (scratch oracles
      oracle_lv_<game>.txt from lv_oracle.py) - single classes (Krynn
      gods / robes / orders too) to their last level: THAC0, saves,
      attacks, hit points (dice, Constitution 18 / 16 / 12 / 5, past the
      hit dice), slots by Wisdom / Intelligence, thief skills by
      Dexterity, experience, last level; multi-classes a class at a time.
      Clean on the nine games. Fixed (Game Fixes): thief skills below 0
      (wrapped to 251%), the Dark Queen's empty save rows past 18th, its
      hit points (signed, at least 1), the Krynn paladins' hit points.
      [x] Curse's / Gateway's / Pool's multi-class hit points (the
      Constitution for every class held, never the fighter's 17+ bonus) -
      decided by Tom 2026-11-06: each class's hit points as a single
      class's, summed, divided by the classes - Game Fixes, every game
      (`create::multi_hp`, SPEC 12; after v0.170.1). Max Hit Points at
      creation too (Tom, 2026-11-06). Not in round 3: turn undead, spells learned
      at training, Krynn moons, demihuman race limits past Curse /
      Gateway.
      the party can hurt) - not bugs, the player would act. Round 3
      (2026-11-06): `behave economy` (weight, armour / load movement,
      Sell, Id, Buy, Share, Take, Appraise, Cheats, wear / unwear, rest
      healing and effect clocks) and `sweep campfuzz` (random camp
      actions, CF1-CF3 + the party checker) - clean on all nine. Fixed
      from each program: Curse's family sells every pile count / 20,
      Secret's type 0x49 one's price; Sell's coins as each pays them
      (Champions steel); Id 100 in Secret / Champions; Curse / Gateway
      pay rounded as Pool; Champions' coin worths (DS 0x0F46); plain
      armour gets no +3 movement in any game (Curse did; coab's facts
      leave the test out); Gateway's item facts had slid (types read 16
      a row, effect 0x18 doubling movement); Champions' arrows are 30 /
      12; elves' / kender's weapon bonus in Champions, Death Knights,
      Darkness, Treasures; the Dark Queen's Sell / Id; View's coin
      Trade / Drop Esc and a late coin number opened the last shop's
      counter in camp. Open: (2) wands / scrolls / potions' use outside
      fights, poison / disease over a rest, level drain / Restoration,
      ageing - not checked by rule yet. Done 2026-11-09 (round 9): (3)
      Darkness's save A camps in a town, where its Rest is off the menus
      (correct); with CFSANCT=1 the camp fuzzer rests (38 rests, clean),
      and it now leaves the icon editor / Alter soon; (4) Champions'
      Appraise paid value / 5 platinum and Curse's gem values - its own
      (GAME.OVR 0x2D21B) are halved (appraise_div 2) and paid in steel
      (0x2BAE4: room, the rest to the pool) - fixed; economy's Id now
      puts all its coins in the purse (0x2A18F's worth, 0x2BA47's
      payment) and G1 checks the steel; (5) Curse's armour movement read
      in its code (GAME.OVR 0x378C3: plain chain mail 9, +3 only with a
      plus) - as here, no DOSBox run needed.

- [ ] Money with Game Fixes (Tom, 2026-10-31; SPEC 12 "Money"): done for
      Pool, Curse, Secret, Gateway. Open: (1) Champions' coin worths
      (found 2026-11-06: DS 0x0F46 - copper 5, bronze 10, platinum 25,
      steel 50 to the 50; payments use them now) before its
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
      member slots (1.7 KB each), Data's words read on demand (the spell
      names' read buffer: already in 1 KB pieces - load_magic, checked
      2026-11-13).
- [x] The plain C5's scan: "The Unlimited Adventures journal couldn't be
      processed (card full, or not enough memory)" (the Companion's) - moot
      since 2026-10-24: the scan passes Unlimited Adventures' folder over
      (library.cpp), so its journal is never made.
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
- [x] Check: pdfview's State (FileSource cache, >4 KB) lands in PSRAM on
      the S3 - its card reads must go through the internal bounce.
      v0.196.21: it did (fs::File reads straight into the FileSource's
      cache inside the State); the FileSource now in internal memory
      (heap_caps_malloc MALLOC_CAP_INTERNAL). viewer.cpp's are static,
      on the stack or under 4 KB (internal) - fine.
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
- [x] Darkness's sounds, with its fights (the burst: done before; v0.196.19
      the shots: the pointed types 12, every other shot - a sling's too -
      9, Darkness GAME.OVR 0x20E42, Treasures 0x28FDC the same): the area spells' burst plays
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
- [x] Claude: Secret's Barkskin - the original runs Heal's routine for it
      (nearly full hit points, no effect). v0.196.17: with Game Fixes no
      healing, effect 0x0D for 4 + level rounds; 0x0D's own rule (+1 to a
      save not against magic - Secret / Darkness / Treasures, their
      handlers) in the engine for all three (behave secret: 5415 / 0).
      v0.196.18 the Dark Queen's too (DQK.EXE 0x4E0AD in its save event
      0xC - [0xB2E7] the save's d20, [0xBEBD] the kind); rules4
      ONLY=barkskin. v0.196.22 Death Knights' (its effect 0x30, handler
      GAME.OVR 0x1CB11 in events 0xB / 0xC): AC 1 better against blows
      and a save + 1 unless the spell cast is of class 4. PoolRad, Curse,
      Gateway, Champions have no Barkskin (no such spell / name).
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

## Sequels (later, not now - Tom, 2026-11-01)

- [ ] A PortMaster version and an Android version of the engine. Route: an
      SDL2 front end (desktop first - Linux / Windows / PortMaster - then
      Android via SDL's Android project: storage access, touch layout,
      lifecycle, packaging). The engine / game code is already portable;
      src/ui (~8.6K lines, LovyanGFX + Arduino SD) and src/hal are the
      board-bound parts. Keep new code portable meanwhile.

## Testing method (Tom, 2026-11-06 - CLAUDE.md "Testing method")

- [ ] Blind-spot checks: the cursor-key / tap layer on every list and menu
      of every game; waits against the originals' delay rules; the frames
      within an attack; the cheats and comforts against Tom's specs.
- [x] Tom (2026-11-06): Modify's menu line - its highlight never moved,
      Exit only by a tap. v0.173.2: Up / Down go round the items and the
      menu line (between the last item and the first); there Left / Right
      move the word and Select picks it; on an item Left / Right still
      change the value. Checked in the playsim (MODOPS) on Curse, Secret,
      Darkness: Exit reached and picked by the keys.
- [x] navcheck covers Modify (2026-11-07; rules M1-M5, NMODIFY - the
      monkey goes in from the Trainer's / a hall's party menu): all nine
      games, 0 failures after the fixes - Modify's word kept by the View
      under it came back on every redraw (Left / Right on the menu line
      did nothing once a word had been picked; `modify_menu_line`), the
      Dark Queen's party menu kept View's / Modify's words in hand (a tap
      on a button lit them), Pool's creation lists' Select a column left of
      the box, the Trainer's Keep forgets spells memorized past slots that
      a lower Wisdom / Intelligence took away (party check P5).
- [x] Rule-check gaps, round 4 (2026-11-07; `behave rules4`, facts:
      claude/rules_round4_facts.md): the turn table against each program's
      own bytes (found by content), each game's turn routine (Curse / Pool
      listing, Gateway / Champions GAME.OVR, Darkness R13) and who may turn;
      Restoration / training's lost levels (coab); Haste's ageing (coab);
      poison / disease over a rest (spell_facts / coab handlers); the Krynn
      moons' caster level and saves (Champions / Death Knights programs).
      Fixed: Champions' turning (13-column table at DS 0x444, its own rules,
      Majere + 2, turns by undead loaded), Gateway's paladins turning,
      Curse's former clerics turning (+ AI turn before spells; column 0 the
      original's, Game Fixes the level's), the moons' caster level
      (Champions, Death Knights) and Death Knights' moon saves.
- [x] Dark Queen: "Not in the engine yet." in the NMODIFY runs (2026-11-07,
      unversioned): the camp's Load (not the Trainer). DOSBox: "Game not
      saved. Load anyway? Yes No" unless saved in this camp, then the party
      gone and the party menu's "Load from where?"; the camp save asks "Exit
      to DOS?" - both done. Also its creation's Reroll menu Modify (the same
      Modify screen on the new character, Keep / Exit back - DOSBox), and
      every reachable "Not in the engine yet." in the nine games: PoolRad's
      camp Animate Dead / Blink and Death Knights' camp Blink (their fight
      handlers), the Dark Queen's 0x5F (Death Knights' scroll of
      protection); the rule tables' load failures say "Not enough memory."
- [ ] "Not in the engine yet." left (2026-11-07 gaps sweep, every game):
      spells with no handler only where no player can cast them - Secret's
      row 0x6C (no name, MU 9th), Curse's / Champions' / Gateway's Animate
      Dead rows (monster class) - an item carrying one would still say it.
      PoolRad's Animate Dead in camp skips the original's sight check
      (ovr23 +0x20E9 call) and its effect-allowed check (+0x21B6) as the
      fights' version does. Death Knights' camp Blink taken as Gateway's
      (its camp code not read). Not compared in DOSBox: the Dark Queen's
      party menu Exit asks "Exit to DOS?" (the engine leaves at once - all
      games: check the others' originals).
- [x] Death Knights: "fighter 1's turn without Move (moves 0, rec move 6)"
      (2026-11-07): the check's blind spot, not the engine - the turn began
      while navcheck's probe settled and the next monkey action (its own
      Move) spent the 12 moves before the check looked (traced: round 3's
      menu with 12 moves, then Move to under 2). sweep.cpp checks a turn's
      start after the probe too; seed 2 replayed clean (same 2004 probes).
- [ ] Rule-check gaps left from round 4: items used outside fights for
      Pool / Darkness / Gateway / Treasures / Krynn (Curse and Secret have
      checks - 2026-11-08: done, below); spells offered at training (Curse's list stops at 5th level -
      later games' own code not read); dual-class beyond Curse's checks;
      the Dark Queen's moons (its C program not read); Death Knights' mage
      caster level takes the ranger's level unchanged (max, unsigned - not
      done); Haste's ageing and the rest's poison / disease in the later
      games checked against Curse's rules only (their code not read);
      Gateway's / Champions' AI turning (only Curse's / Pool's known); the
      turn routines of Death Knights / the Dark Queen assumed Darkness's.
      2026-11-07 (rules4 ONLY=caster, turn): DONE - the Krynn caster
      levels read (Champions GAME.OVR 0x38FA4, Death Knights 0x34DCA):
      both count the KNIGHT (class 7), not the paladin (6 for one with no
      cleric / mage, knight < 9, ranger < 8); Champions a cleric's spells
      max(cleric, knight - 8) skill levels; Death Knights max(cleric,
      knight) and a mage's max(mage + moon, ranger) - levels, unsigned
      (fixed; 20 cases). Found with it: Champions' spell codes are Death
      Knights' (0 cleric, 1 druid, 2 knights' specials, 3 magic-user, 4
      items - checked in START's table and its caster routine); the engine
      read 3 as the items' code - its mages' spells cast at 12th level and
      weren't usable (memorize / learn / slots); now mapped as Death
      Knights'. Death Knights' turn routine (GAME.OVR 0x1FEAA) read: =
      Darkness's with Majere's clerics (record 0x23 = 2) 2 levels higher
      (fixed, turn_rule 3; 176 cases). Gateway's caster level = Curse's
      (0x418C8, read). Left: Gateway's / Champions' / Darkness's AI turning
      (their AI routines reach the turn routine through overlay stubs -
      not traced; the engine's computer never turns there); Darkness's
      turn takes the skill level (its call 0x102:0x48), the engine the
      level (former clerics only); Death Knights' Turn menu condition and
      spell 0x2F's 15th level / the [0xC447] 6th-level case of its caster
      routine; the Dark Queen's turn and caster routines; Darkness's /
      Treasures' caster levels; training spells, items outside fights,
      dual-class, Haste / rest in the later games - not reached.
      2026-11-08 (rules4 ONLY=learn, every game but the Dark Queen): the
      training's new-spell list read in each original (its spell-list
      mode 4): PoolRad 1-56 with slots, not known (no can-learn check);
      Curse / Gateway 1-100, slots, can learn, not known (the engine's);
      Secret the magic-user's lists (DS 0x5851, levels 1-7, 11 a level -
      its 1st-level 11 / 18 / 21 never offered); Darkness / Treasures
      1-126, magic-user spells only; Champions / Death Knights the robe's
      lists (DS 0x4208 / 0x8189, White list 2 else 3, levels 1-4 / 1-8).
      The engine offered cleric / druid spells (Secret, Darkness, Krynn)
      and every robe's spells (Krynn): fixed (magic::learnable,
      Layout::mu_lists); 600 random set-ups a game, 0 differ (the old
      code: 121-573 differ). Left: PoolRad's spell 56 (cleric 7th level:
      its record's 0xB8 read as the slot count) not modelled; when the
      list is shown (the originals' training condition) not compared.
      2026-11-08 (rules4 ONLY=use, every game but the Dark Queen): items
      used outside fights - each original's UseMagicItem read (PoolRad
      GAME.OVR 0x22AC5, Gateway 0x2FC02, Darkness 0x2569C, Treasures
      0x2E0EF, Champions 0x29242, Death Knights 0x25A77): the same "used"
      test (not a scroll, 0x3D, 0x3E < 0x80), Must be Readied, the
      combat-only question and the use taken (count, then charges) as
      Curse's. The scroll's reader differs: PoolRad no test at all,
      Darkness / Treasures by skill levels (a paladin / ranger past 8th
      reads); the engine used Curse's rule everywhere and rolled the d100
      for everyone (a die the originals roll only for a thief past 9th):
      fixed (magic::scroll_reader). Left: Treasures' own items with 0x3E
      0x0F ("jumps" - fights), 0x10 and 0x11 ("is already affected" - an
      effect on the user) have routines of their own (GAME.OVR 0x2DBF3 /
      0x2DD75 / 0x2DB44) - the engine casts their 0x3D as a spell.
      2026-11-08 (rules4 ONLY=canuse): the Krynn games' can-learn read
      (Champions GAME.OVR 0x2DC8E, Death Knights 0x29C84): cleric spells
      for a cleric or a knight past 5th (Death Knights also a paladin past
      8th, Champions none), druid spells for a ranger past 6th (Death
      Knights 7th) - the engine had Curse's (paladins past 8th, no
      knights): fixed in magic::can_use, 800 set-ups a game. Death
      Knights' Turn menu condition read (0x94CC: cleric, or paladin past
      2nd, turns < the undead loaded) = Darkness's, the engine's: no
      change. AI turning: Gateway's computer (GAME.OVR 0xD266, reached
      from its AI at 0xD14A, before its spells: not turned yet, a cleric's
      skill level - no paladins - and an undead check 0x97:0x52, then the
      turn stub 0x97:0x4D the menu's Turn calls) and Champions' (0xC772,
      from 0xC6D3, before its spells: the menu's test - turns < the undead
      loaded, a cleric or a former one) now turn as Curse's computer does;
      fightfuzz seed 1 x 40 fights each: no rule broken. Left: Darkness's
      AI turning (its menu dispatch not found by the 'T' key - not
      traced); Champions' AI compares the old cleric level with record
      0xD7 (the engine: the menu's test); the undead check (0x97:0x52 /
      0x88:0x52) taken as Curse's (an enemy undead in range).
      Round 6, v0.178.0 (facts: scratch /tmp/claude-0/ag/round6_facts.md):
      [x] the Dark Queen's can-learn (DQK ovl 0xA 0x5A311: cleric spells
      and the gods' specials - Wisdom, a cleric, a knight past 5th or a
      paladin past 8th; druid spells a ranger past 7th; magic-user
      spells a magic-user or a ranger past 8th) and caster levels (res
      0xB4E0: max(cleric, knight), ranger - 7, max(magic-user + the
      robe's moon, ranger - 8), items' 12) - rules4 canuse 3200 / caster
      15000 cases; its training list (mode 5, 0x5AFD9) checked = the
      engine's (learn, 600 set-ups); its moons now reach magic::moon
      (q_moons / krynn_moons). [x] Death Knights' can-learn: magic-user
      spells need a magic-user or a ranger past 8th (0x29D16). [x] the
      Dark Queen's fight words: its boxed print 0xCD31 (from 0xB14E: the
      name, the words a row below, 23-38 x to 21; a word whose space is
      just past the box goes without it) - rules4 wrap, 3000 strings
      letter by letter; its weapon line the same way (res 0xA50B, rows
      7-9). [x] Treasures' own items (0x2E15B): 0x11 an
      effect on the user (Potion of Spell Enhancing: 0x9F, 1d4 x 10 + 40,
      "Is empowered", + 3 caster levels - 0x40DEF; already affected:
      nothing used), 0x10 the maps (GEO block 0x3D shown, nothing used),
      0x0F nothing outside fights - rules4 use (84 checks). [x] turning:
      Darkness / Treasures by skill levels (0x392EE), their computer
      turns (0x9D14 / 0xAB2C, not while fleeing) and Death Knights'
      (0xA46C); Champions' computer by its own test (0xC772: old cleric
      over 0xD7); Gateway's finder types 1-12 (0x17679). [x] PoolRad's
      spell 56 at training (0xB8, kept; none with the Game Fixes).
- [x] Round 7 (v0.179.0; facts: scratch round7_facts.md, to go to the
      Project): Treasures' boots' jump in fights (0x2DBF3: spell 0x7F's aim,
      "cannot go there" only when someone stands there - a wall too, kept;
      Game Fixes: Dimension Door's squares; 0x9C off "is free to move",
      "jumps"; a charge and the turn either way) and the maps in fights
      (shown, nothing used, the turn goes on) - rules4 use, 7 cases. The
      Dark Queen's moons: its clock steps them a day at a time (DQK.EXE
      0x5D6E: counters 0x1FD-0x1FF, limits 8 1 6, phases 0x1FA-0x1FC - the
      engine read game byte 0 for them: always new), a new game's at 2, a
      fight's mages carry their robes (saves 0x8576, caster level 0xB5E6),
      the moon's extra spells over levels 1-9 (0x4501C) - rules4 moons 906
      + 72. The computer's items: Darkness's / Treasures' item routine is
      an empty body (GAME.OVR 0x9D9A / 0xABB4) - never; Death Knights'
      (0xA4D3) before the pending spell, Turn, spells - rules4 use, 8 games.
      The armoured human magic-user in a fight (the can-learn's fight
      clause: Secret 0x2C76B, Darkness 0x29DED, Treasures 0x32CCE, Death
      Knights 0x29D23, the Dark Queen 0x5A402; a ranger past 8th may) -
      rules4 canuse.
- [x] Round 8 (2026-11-09, facts: the Project's claude/rules_round8_facts.md):
      Death Knights' computer item routine (GAME.OVR 0xA4D3) rolls its d7
      rounds and walks the items but its chosen-item pointer is only ever 0 -
      it never uses one (as Darkness's / Treasures'; the Dark Queen's
      DQK.EXE 0x2E105 is an empty body too); in those four games the
      computer's pending spell goes off after its morale test and items
      (the turn routine sends computer fighters straight to the AI: Death
      Knights 0x9384, the AI's order +0x1260 morale, +0x2B8 items, +0x141
      pending) - rules4 aiorder / use. The Dark Queen's Turn Undead found:
      0x25E77 (menu 0x22DC7, computer 0x2E078 - not fleeing, turns under
      the undead loaded [0xC2ED], the class test; finder 0x260BE: highest
      type up to the limit, the limit made that type): plain levels
      (0xBE14), Majere's + 2 (record word 0x19B = 2 - the engine's f.god
      was Champions / Death Knights only), game byte 0x0F less - rules4 turn
      66 cases, former, aiturn. Its fight pages: the fall after a hit
      (0x25020) / a damage text (res 0x8F04) goes on at the row after the
      words (it started at row 10 here), "Goes Down" (0x19B) and "%s and is
      Dying" (0x1A5) for the damage text's (here "goes down" / "and is
      Dying"); the box's page-full wait (0xCEEA) - FSt::More; the weapon
      name's trailing space (0x9FDC) - rules4 qpages, wrap (448 full boxes
      against a model of 0xCD31). The moon's extras: any level, slots or
      none (0x45519; list mode 1 0x5AF2D has no level test) - rules4 moons.
      Champions: its computer's finder limit (0xC772 [bp-6]) holds the AI's
      last dice call's side count (the dice routine's pushed argument at
      that stack place: 4, 2 after a d8 of 8) - types 1-4 here; its
      can-learn has the fight clause (0x2DD6B) - rules4 aiturn, canuse.
      Training's spell list: Champions (0x25666) a magic-user's new level
      or any training of a ranger past 8th (as here); Death Knights
      (0x199DE, ovl 0x61 +0x49DE) a ranger's own new level past 8th, as the
      Dark Queen (here: any training) - fixed; rules4 trainspell. Both
      skip it while creation levels a new character ([0x4B58] / [0x8C5C]).
- [x] Round 8 left (done 2026-11-09, round 9 - facts in the Project's
      claude/rules_round9_facts.md): training's spell list read in every DAX
      game (GAME.OVR: Curse 0x25534, Secret 0x22837, Gateway 0x2BEFC,
      Darkness 0x18DF6, Treasures 0x1E6E7 - a magic-user's new level or
      any training of a ranger past 8th, as here; PoolRad 0x1C8DE: a
      magic-user's new level only - fixed; rules4 trainspell on all
      nine). The Dark Queen: "lost a spell" now at the hurt (row 12, its
      wait clearing the box; after a blow 0x2522F the fall at the blow's
      end, after a damage text 0x9106 below it; the spell marked spent,
      nothing at the turn); "reflects it!" at 12 under the gaze, "engulfs
      %s" / "constricts %s" at 12 on a blank box (their words were missing
      from its table - added); an undead's fall (record 0xC8 & 8) "goes
      down" / "Goes Down" only; the full box's key is the prompt box
      "Press [Return] to continue." (res 0xD400 -> 0xE645, its arguments
      unused) - rules4 qlost (6 checks). Death Knights' / the Dark Queen's
      bandaging: in the move-and-attack step after morale, items, the
      pending spell, Turn, spells (GAME.OVR 0xAEE7 +0x3C - 0x151D was the
      weapon choice; DQK.EXE 0x2E94D) - fixed; rules4 bandorder. Death
      Knights' fuzz "boot": the save's own opening fight lost while booting
      (the dice) - the fuzzer passes it over. Champions' d8-of-8 limit 2:
      still open (below).
- [x] Round 9 left (done 2026-11-09, round 10 - facts: scratch
      round10_facts.md, for the Project's claude/rules_round10_facts.md):
      Champions' coin 0 (record 0xEB, Silver, worth 1 - DS 0x0F46) now the
      engine's money place 2 (electrum's): worth (GAME.OVR 0x2A18F) and
      payment (0x2BA47) count / zero it, pooling (0x2BE03), sharing
      (0x2BF2C, 6 down to 0, weighed as given - 0xB945), a fight's coins
      (0x63D4) and the NPCs' share (0x798E) move it; the weight's
      recalculation (0x37484) and View (0x273D2) leave it out (coins 1-6).
      The counter's coin list (0x2C61C) is the record's order up from
      coin 0; View's Trade / Drop (0x29D75) jewelry down to copper - both
      were the engine's 6-0 order (platinum before steel). Its scripts'
      coin codes 0xBB-0xC3 are the record's 0xEB-0xF3 (0x9330 / 0x96AD) -
      they read copper / bronze / steel / platinum one place off before.
      Death Knights' coins: steel 0x3F, gems 0x41, jewelry 0x43 only (no
      indexed coin code in its GAME.OVR); the Dark Queen's 0x4C-0x50, its
      weight 0xA971 three - as the engine. economy L1 / K1 (2100), W1 / I1
      from the record and DS 0x0F46. Appraise: Death Knights' (GAME.OVR
      0x295A7 / 0x297FD) Champions' halved values - it had Curse's whole;
      the Dark Queen's (DQK.EXE case tables 0x49D87 / 0x4A03C) the same
      but its 41-50 jewel 250 + random(1500); oracles/appraise.py, economy
      G2 (300 a game). The Dark Queen: Feeblemind says "loses intellect"
      (it said nothing) and loses the spell under way at once ("lost a
      spell" at 12 - 0x53EDE / 0x53F17); one coughing in a stinking cloud
      (its save made) loses it too (0x816B; nausea not) - rules4 qlost 7 /
      8. Champions' computer: combat byte 0x15's dice at each turn's start
      (0xC5B0-0xC614) kept per fighter, the last die's sides its Turn
      Undead limit (2 after a d8 of 8) - rules4 aiturn (3584 dice cases,
      the limit 2). Queen fightfuzz seed 13 fight 26: not a stall - the
      last member's bow hits the AC -10 white heads only on a 20 and wins
      near round 150; R10 now reports only fights whose hit points stand
      still 60 rounds past FIGHTCAP (or past 4 x FIGHTCAP).
- [x] Save / load round trips, every game (2026-11-09, v0.182.1): sweep
      saveload 0 problems on all nine. Found: the check planted Curse's
      script places (0x4C05 / 0x7E10) in Darkness-type games, which don't
      have them (their script memory is the 1 KB game block at 1-0x400) -
      now 0x3F0 / 0x3F8 there; the Dark Queen's loaded effects kept the
      original's next-effect pointers (bytes 5-8) while its save writes the
      engine's marks - now cleared on load, as an effect the engine makes;
      the bytes a camp's own script changes (the check saves from camp, a
      load runs the area's start again) are left out; PoolRad's wall-set
      words 0x4CFA-0x4CFF (written at each save for the original's load,
      not read back by the engine's) excepted. The check prints the values.
- [x] Round 10 left, done in round 11 (2026-11-09; facts: the Project's
      claude/rules_round11_facts.md): the to-hit roll found in every
      program by its shape (tools/devkit/tools/tohit.py; DQK.EXE 0x8461,
      called from the blow 0x26407): d20 1 a miss, 20 counted 100 (always
      a hit), 17+ the high-roll flag - the engine's already; the side's
      bonus was missing in Darkness, Treasures and the Dark Queen (their
      game block's 0x2F / 0x01, the engine read Curse's 0x7F71 / 0x7F70 -
      nothing there) and every difficulty adjustment: Darkness-type and the
      Dark Queen 1 +2, 2 +1, 4 -1, 5 -2 (byte 0x27), Gateway 0 +2 ... 4 -2
      (word 0x4CF4). Secret / Death Knights / Champions compare a constant
      (0x1F4) - none (Secret's and Death Knights' given by Game Fixes, SPEC
      12). play::side_bonuses; rules4 tohit against oracles/tohit.py (the
      switch walked for every value) - 9 games, 0 wrong. The Dark Queen's
      coughing: 1d4 + 1 minutes (a minute a round), data 0xFF, as nausea;
      nothing for the undead, the nauseous, effect 0x8F, flags 2 & 0x18
      (DQK.EXE 0x805D) - rules4 qlost 8 / 9. Its Feeblemind (spell handler
      0x58108): effect 0x44 kept (no time), Int / Wis in use 3 at once, the
      spells being memorized forgotten - rules4 qlost 7. Champions' byte
      0x15: every reader found - its step rows (DS 0x396 by mood, 0xCC6A /
      0xCF53; the flee's d2 is its mood, the way the party's facing as
      PoolRad's; a step back: the next row, then the target dropped, then
      no more moving) - rules4 aimood (240 cases + the step back); its AI
      order as Death Knights' (the pending spell after morale and items,
      0xC699; bandaging in the move step 0xD270) - rules4 aiorder /
      bandorder now on Champions. The Dark Queen's saveload: 0 problems
      since v0.182.1.
- [x] Round 11 left (round 15, 2026-11-10): the step rows by mood in every
      DAX game (rows PoolRad DS 0x2AE, Curse 0x2B8, Secret 0x32C, Gateway
      0x390, Darkness 0x8E8, Treasures 0x8F4, Champions 0x396, Death Knights
      0x312; the same AI dice in each), the flee's way from the party's
      facing (+1 in Darkness / Treasures / Death Knights), the try's cost
      test (strict; PoolRad's own) and its cloud squares (effects, driven,
      flags 2, 7+ HD, a save - per game), the second step back's new target
      (none: it guards) - tools/devkit/oracles/aistep.py, rules4 aimood on
      all eight. Feeblemind: Darkness / Treasures as the Dark Queen's (their
      generic save rule, not Secret's; 3 kept by the recalculation), Death
      Knights' drain-2 as the original behind Game Fixes - rules4 feeble.
      The Dark Queen's DAMAGE (0x4256F): its saves' branches, bonus & 0x0F
      (swapped branch behind Game Fixes); its one-target roll 0x83D0 already
      as the engine's (natural 1 misses, 20 = 100, roll + bonus > AC).
- [x] Round 15 left (round 16, 2026-11-10): the Dark Queen's to-hit hooks -
      its handlers are DS 0xBEE8 + 4 x effect (dispatcher 0x53775; overlay
      w9, segment 0x2B3A at file 0x4DE90, a second segment 0x30CC), filled
      at 0x52E42-0x5362x: event 0xA / 0x10 / 0xB read and matched (hooked
      roll under 0 a miss; giants -4 always; no gnome +1; Invisible to
      Animals on the holder's own flags; no bite for 0x45; unnamed +2 / -2
      / -1; Shield front only, lost to coughing; coughing / blind / dance /
      highlight / iron skin AC; the dragons' fear 0x64 without a save);
      DAMAGE's one-target roll with the target's hooks; its clock steps
      each round's START (0x2336C -> 0x61AD -> 0x5EFF). Darkness's /
      Treasures' recalculation is Secret's (stat_fx all six; 18/xx for
      current fighter levels only). The count-2 finder followed (pass 2
      without the sight test; Darkness / Treasures scan from a random
      place) in all eight DAX games. Death Knights: the party's own
      feebleminded drains whoever it hits, as the engine did - but only a
      damaging blow runs the on-hit specials. rules4 qhooks / statfx /
      finder / khit; oracles qhooks.py statfx.py finder.py knights_hit.py.
- [x] Round 16 left (round 17, 2026-11-10): the Dark Queen's effect 0x83
      (0x543D8: the to-hit in use from record 0x87 + Strength's to-hit next
      to the target / Dexterity's missile bonus further off; its damage
      words lost - the blow works them out after the roll); event 0xA / 0x10
      side-effects (0x19's [0xAE66] a dead store - only the seen test
      0x25D90 reads it, zeroed before; the blind (-4) / 0xA2 (+2)
      attacker's own stored AC until a blow on it recalculates it); DAMAGE's
      cloak made fresh on a roll of exactly 0 before any fight (and its
      0xCF -1); every other game's on-hit after a blow of no damage (Secret,
      Darkness, Treasures, Champions, the Dark Queen need damage - the Queen
      not for a 0xB8 holder; PoolRad, Curse, Gateway don't); the computer's
      target at its turn as each DAX original's finder (rules4 turnfind,
      onhit, qhooks).
- [x] Round 17 left (round 18, 2026-11-10): at moves 0 the computer
      guards in all nine (each AI loop's moves test - Curse GAME.OVR
      0xBC50 ..., DQK.EXE 0x2E043), and the guard routine's tests (Curse
      0xCC32 ..., DQK.EXE 0x2EDFC: held, a readied weapon of range over 1,
      no moves - the turn just ends, no guard); the Dark Queen's finder
      (0x28AFE: tries = the list, a roll first, its taunted 0x8C - not
      0x5B, the lightning breath - moved to the list's front, taken free
      only from a list of 20; the turn's first 0x2DE27 and its AI dice);
      0x83's damage behind Game Fixes (SPEC 12); 0xB8 is carried by the
      Gas Spore (MONCHA.GLB 6) alone, 0x83 by item type 67 (Dawnshine's,
      ITEM.DAT 45 / 214 / 215). rules4 guard0, qhooks; oracles guard.py,
      carriers.py, qhooks.py.
- [x] Round 20 (2026-11-10, v0.195.0; facts: the Project's
      claude/rules_round20_facts.md): the computer's spell choice and
      targets from each original's code - the d7 games' judge (Curse
      GAME.OVR 0xBDEC: heal fall-through, every candidate's area by row
      byte 15, PoolRad's friends, byte 8 = 1, place 0, being-memorized
      places), the by-priority games' chooser and use tests (Darkness
      0x9DAD / 0x2249A, Treasures 0x2A74B, Death Knights 0xA787 / 0x22CE4,
      DQK.EXE 0x2F92E / 0x2902C), the finder at the spell's reach,
      "Spell Aborted" with no target; Secret / Gateway / Champions now
      follow row byte 14 as Curse / PoolRad did. Checks: rules4 aicast
      (the four games' use tests against their routines run in Unicorn -
      tools/devkit/oracles/aicast.py, emu16.py - 18-22 k states each,
      their spell rows, the chooser; the d7 games' judge), verify20.sh.
      The items' chooser (round 8) re-read: Curse's 0xBEE5 as the engine's.
      Round 19 left closed: the step routine's "moves / 2" is combat +6
      (the moves in half squares) / 2 > 0 = the engine's f.moves >= 2 -
      the same; the Dark Queen's morale figure now kept across fights.
- [ ] Round 20 left (2026-11-10): the by-priority games' area finder
      (Darkness GAME.OVR 0x22976, 550 lines - where an area spell is
      centred; the engine centres on the finder's pick); the Dark Queen's
      spells 138-140 (class 4, 8th level rows past the engine's 138) -
      read (round 22): their name pointers (DS 0x25CB) are null, the last
      named is 137 - no spells there; the computer's weapon choice
      per game - DONE round 21 (v0.196.8): each DAX game's rating routine
      RUN in Unicorn (oracles/wrate_emu.py, 3000 random items a game;
      rules4 ONLY=wrate: 0 differ in all eight, the bare rating 414-1123)
      - holy water 8 against undead (PoolRad / Curse / Gateway 0x55,
      Champions / Knights 0x36), the lightning item of another alignment
      and the paralysing gaze 0, byte sums, Secret's hands rule (>= 3),
      Darkness / Treasures' target (the plus it needs, halving by blow);
      the choosers read (oracles/wrating.py: the same decision in all
      eight). Gateway's +0x3EE = script word 0x4CF7 = UNDER WATER (SAVE 1
      before ECL2's fights 2 / 82; "The water clears a little" 0x12987) -
      v0.196.13 (b.gw_water, rules4 ONLY=gwater): its chooser no missile
      weapon (0xEB39), nothing shoots (0x42318), Fireball / the item
      fireball "Nothing Happens." (0x385FB); v0.196.14 colour 8 blue in
      its fights (0x49714 / 0x49769: EGA 1, VGA 0,0,0x29 - fight_colours
      3); v0.196.15 a cloud gone "The water clears a little..." (0x129B6);
      v0.196.16 no tables and chairs (0xF852 skips the rooms' furniture
      0xEF50 - the engine's draw_block step 5); the Dark
      Queen's rating (DQK.EXE 0x2F081, chooser 0x2F1A6) RUN too (3000
      items, 0 differ; the bare rating 1137): Darkness's target rules, no
      item effect tests, the plus unsigned, Secret's hands rule, and a
      weapon with a range 0 while game byte 0x3C - UNDERWATER - is set.
- [x] The Dark Queen's draconians dying (v0.196.23-25) - NOT in the engine before (found round
      22, 2026-11-13; Champions' are: mon.explode / stone_death /
      acid_pool). DQK.EXE's own (qhooks.py handler map; carriers.py):
      Enchanted Baaz (MONCHA 87) 0xBB "loses his weapon" (handler
      0x51DE8; 0x99 the same word, 0x507D9 - no carrier); Enchanted Kapak
      (88) 0x9C "dissolves into acid!" (0x50BC8, read: its own square and
      each of the 8 round it on a d2 of 1, inside the field and on a ground
      of move cost 1 (DS 0x6197 byte 0), get ground 0x46 for 1d6 rounds -
      a cloud record on DS 0xB3D4's list, 0x50A0E; ground 0x46 is one of
      its cloud grounds with 0x1B / 0x1C / 0x1D / 0x1E - the square reader
      0x2AFE0 flags it; its harm not read yet); Bozak (78) 0x9A (queued
      on death - below) and 0xA6 given by code (0x50D62 /
      0x50F81), handler 0x5100F "explodes!" / "is stunned"; Gas Spore 0xB6
      (0x51A6A "explodes." / "dies from"). v0.196.23: the Enchanted Baaz
      (mon.stone_death 0xBB; its extra tests - the last blow's damage a
      multiple of 3, the weapon not cursed). v0.196.24: the Enchanted
      Kapak's acid (mon.acid_pool 0x9C, Facts::acid_ground 0x46 /
      acid_spread: lay_acid_spread; 1d6 no save, kind 0x100 - the cloud
      harm routine 0x7F9B's flag, 0x83A2; the AI's try - StepCloud kind 4,
      under 7 Hit Dice and not driven no step; "The acid drains" DS
      0x1E1; kGroundValues 0x47) - rules4 ONLY=qacid. v0.196.25 the
      Bozak (mon.explode 0x9A, Facts::bozak_1d6): queued as it dies, then
      after the blow / spell (0x48F:0x2AB8 = 0x97A8) "explodes." (DS
      0x208), 1d6 to each within a square (not those exploding too), no
      save, no stun - rules4 ONLY=qbozak. Not this monster: 0xA0 "rises in
      a new form" (given at 0x5074C - the dragon's?) and 0xA5 / 0xA6 /
      0xA7 (the Chromatic Dragon, MONCHA 74) - a later check. The Bozak's 0x9A (0x509AE,
      death event 0xD): not active (+0x18B 0) -> status word +0x64 = 10, put
      on a list (DS 0xC284, count [0xBD4C], up to 20), 0x9A taken off; the
      list's routine 0x50CF0: status word cleared, active again, a save
      (0x48F:0x26C1 with "rises in a new form" DS 0x846D) - failed: gets
      0xA5, 0xD2, 0xA6 (3), 0xA7, loses 0xA0 0x97 0xA1-0xA3; then 0x50DF0
      "immolates" (DS 0x8481) round it ... (not read further; "explodes!" /
      "is stunned" in 0xA6's handler 0x5100F). Death event 0xD lists 0x9A 0x99
      0x9C; 0xE 0x46; 0xF 0x44; 5 0x43. To port: read each handler's
      dice / saves / durations, then map onto the Champions machinery
      (bozak_burst, baaz_weapon, lay_acid) or Queen-specific variants.
      v0.196.26 the Aurak (MONCHA 77 / 91; QueenFx mad ...; corrects the
      reading above - events 0xD / 0xE hold 0x98 0xA0, 0xF 0xA1 0xA7, 5 /
      6 / 9 0xA5): its death (0x5070D) - up in its square with 20, "goes
      mad!", 0xA0 0xA1 0xA2 for ever, 0xA3 6 rounds with the call flag;
      0xA1 at its turn's start "immolates", 1d6 fire, no save, each foe
      within a square; 0xA3 run out (0x50F52): 0xA5 (blows' and spells'
      damage 0), 0xA6 3 rounds called, 0xD5 (the computer's turn ends -
      0x2DE46); killed again (0x50CF0): "rises in a new form", 0xA5, 0xA7
      (the turn's start: the first foe in the scan "gets zapped!", 2d6 /
      8d8 with 0xBF, the turn over - 0x1D99:0xEE1 = 0x247E1), 0xA6 for 3
      (its 0xA3 taken off with the flag runs 0x50F52: the 0xA6 called);
      0xA6 run out called (0x5100F): gone, "explodes!", 3d6 / 16d8 each
      within a square, no save, then a save vs spells or "is stunned"
      (0x35 for ever) unless holding 0x98 0xA0 0xA6 0x35. The remove
      routine 0x48F:0x17D calls a handler only for an effect given with
      the flag; 0x19C2 appends (no replace). rules4 ONLY=qaurak. The
      Chromatic Dragon's body (MONCHA 83: 0xA5 0xD2 0xD5 innate) is now
      unharmable and idle as the original's (fight_check R10 skips a side
      that can't be harmed). Not followed: 0xD5's other readers (the
      icon's facing 2; the free-attack list 0x259F0 with 0xD2); 0xA2's
      record 0x18E / 0x18F + 2.
- [ ] The Dark Queen's monsters' own effects (round 22, carriers.py + the
      qhooks handler map; events 2 / 3 on-hit by attack, 4 the attacker's
      damage, 5 a blow's damage taken, 6 a spell's, 8 the set-up, 9 a spell
      effect given, 0xD death, 0xE the move step, 0xF turn start, 0x12 /
      0x13): v0.196.27 the mummy (MONCHA 38: 0xA8 magic fire + a die - 0x5145A,
      0xA9 a magic weapon half - 0x51478, 0xAA the rot - 0x514B0: not with
      0x32 or 0xD7, data d6 << 5 + 0x1F, the month clock 0x5163A "Rots..." /
      "Dies rots away" via rules::rot_clock shift 5, 0xAB the set-up fear -
      0x5156B: save +2 human +1 for a side of 6 or more, 1d4 rounds of 0x34,
      "is Unaffected" when made, 0x9E holders immune) and the zombie (37:
      0x9E immune to 0x34 - 0x50CE3; 0x9F a marker). v0.196.28 the Black
      Pudding 0xBA (0x51CC6, event 4: each worn armour of the target's - type
      row byte 0 = 2 - a plus less, "is Disolving %s's armor!", or at its
      floor (0, -1 for types 0x23 0x24 0x26) "Dissolves %s's armor!" and
      gone; any such armour: the blow's damage 0 - party members' armour
      only: a monster's items are its group's in the engine). v0.196.29 the
      Gas Spore (0xB6 at death unless spent 0xC2: "explodes.", 6d6 to each
      within 2, a save of type 2 for half - 0x2214's third argument 2 -
      0xB7 holders none, the spore dead; 0xB8 on any hit, its list's other
      specials first: the one hit unless 0x4C gets 0xB9 for a day - its end
      "dies from disease", rules::death_clock - "is Diseased", the spore
      spent and dead), the Enchanted Bozak 0xBC / Sivak 0xBD (queued at
      death, then "explodes" and spell 0x57 Ice Storm / 0x7C Meteor Swarm
      cast by the fallen one at all within its row's aim & 7 - 2 / 0: the
      Sivak's reaches only its own square, nobody). Cure Disease and 0xB9:
      not read. v0.196.30 the shambling mound (39: 0xAD electricity + 8 as
      absorb_elec - 0x516D2; 0xAE fire / cold half, none on a save against
      spells from a spell that can be saved - 0x516FC; 0xAF blows half -
      0x51754; 0xB0 engulfs on both hits unless 0x96 - 0x51765: 0xB2 held,
      0xB1 suffocating 2d4 turns - 0x518AF, the mound 0xB3; its turn a blow
      at the one inside, the turn over - 0x518DD; the engine's Curse-type
      engulf / holding machinery; the original's special blow dice - record
      0x190-0x194 - not followed). v0.196.31 its breaths (the breath
      handler 0x54A51 is Darkness's: 0x53 the big fire 24d10 + 12, 0x54
      fire, 0x55 cold - the white heads, 0x60 acid of kind 0x20; its own
      0xB4 the Death Dragon's death - a line of 8, kind 0x60 - and 0xCA
      Baldric's: one in 2 paralysis - 10 rounds, a save against breath,
      "is Unaffected" - its shape 4 of 5 made 3 lines - else cold; none
      with its own side on the path, as Secret's; "breathes %s" with the
      kind - the Queen's breath words were empty before), the troll /
      anemone 0xB5 (a hit point at each round's start and each turn's
      start - events 0x13 / 0x12 - while monsters stand, "is fully /
      partially healed"). v0.196.34: the rot's clock 0xD7 - nothing that
      heals with the heal routine's flag 0 (cure spells, rest, the temple -
      0x48F:0x25F7) heals its holder, a point at each 10-day renewal
      (0x52D77); its Cure Disease (0x56480) takes 0x22, 0x2B with 0x2C /
      0x1F, the spore's 0xB9 and 0xD7 with the rot 0xAC (spells::Facts 4
      diseases, the temple's list too); the troll's heal ignores 0xD7 (flag
      1). Every Queen monster effect is read now. The Aurak's 0xA5 also blocks spell effects (event 9, 0x4DF31 with 0).
- [ ] Monsters' effects with no engine fact (v0.196.35, sweep mode
      `effects`: every monster's effect list against every byte of the
      fight / magic / cure facts). The Dark Queen: 36 found; mapped as
      Darkness's (same handlers): 0x06 no electricity, 0x5D no fire, 0x3C /
      0x7D a +3 / +2 weapon, 0x42 edged half, 0x4F the iron golem, 0x39 the
      major globe, 0x7E gazes saved, 0x6C 2d6 fire on a hit, 0x5A poison
      breath and 0xCC "breathes steam", 0x29 missiles of plus 0 avoided,
      0x84 paralysis on a hit (10 rounds); its own: 0x7B / 0x8A / 0x8D
      effects not taken, 0x86 cold double, 0xC0 cold + a fifth, 0x87
      magic fire heals (its "is healed" not said), 0xCD fire four fifths
      and + 4 saves, 0xC1 no blow harms. Left (DQK.EXE handlers): 0x75
      (0x4EF83, event 0xE, 0x22 not taken), 0x85 (0x4FFE2, no event), 0x89
      the stench (0x50221 at its turn: each foe within a square a save
      against spells or 0x1D99:0xF09 "Affected", unless 0x76; it gets 0x94),
      0xC3 (0x527D9, set-up), 0xC5 (0x52426), 0xC6 / 0xD1 (event 0xE),
      0xC7 / 0xCE / 0xD0 / 0xD3 / 0xD4 (on-hit), 0xC9 (attacker's damage);
      0x84 on the first attack only (the engine's kreen bite). The other
      eight games swept (v0.196.36): Secret none; PoolRad's mapped from
      GAME.OVR's handlers (tools/devkit/tools/fxtab.py, table DS 0x682C):
      0x5D fire / 0x72 electricity / 0x76 cold halved, 0x6F poison proof,
      0x73 edged half, 0x74 magic weapons half, 0x75 holy water 1d6+1,
      0x78 boulders dodged, 0x63 / 0x5F the boar fighting on; Gateway's
      0x5D fire half (0x13F81) - rules4 `halves`. v0.196.37: PoolRad 0x7E
      the vampire never targets one with a readied mirror (word 0x76) or
      word 0x98 + 0xFC (0x11A92 in the may-target test 0x13880); Curse 0x7E
      inert (the dracolich gaze never finds a target - monster_fx_facts);
      Darkness 0x2E = Protection from Good (0x09's handler); Champions 0x60
      charm / sleep proof, 0x61 paralysis proof, 0x53 the white dragon's
      cold breath and its breaths' path check (0x33079: one of its side on
      it - no breath), 0x4F the ghast's stench (0x13747) and the Queen's
      0x89 (0x50221): "emits an evil stench", a save against spells or "is
      affected" - the -2 to hit put on the HOLDER by both originals (their
      mistake; Game Fixes: on the one affected - SPEC 12). The 10' radius
      Protection from Evil's +2 to the saves of those next to it now only
      with Game Fixes (the originals' 0x2D runs 0x08's handler - its
      holder's). E1 (FX fuzz): Champions' "gets zapped", "lost an image",
      "Gains an item"; "stands up and grins" / "gets back up" said only
      where the program has them (not the Krynn games). v0.196.38 Death
      Knights (table DS 0xBE82): 0x41 no electricity, 0x42 the iron golem's
      +3 (or a monster of 4+ Hit Dice), 0x4B cold double, 0x4C fire none /
      magic fire heals, 0x4F the ghast's stench, 0x50 no confusion, 0x51
      the vampire's charming gaze, 0x53 electricity / cold half, 0x54 its
      3 hit points a round, 0x57 the iron golem (Secret's), 0x5D blows of
      type byte <= 1 half, 0x60 charm / sleep proof, 0x63 poison /
      paralysis proof; the Queen's 0x87 (and Knights' 0x4C) took no damage
      of ANY kind - fire only now (rules4 dmg22). v0.196.46: the fire
      minion's 0x46 (at its turn 1d6 fire to all next to it, its own side
      too - 0x1D430) and 0x4A (2d6 more on a hit, a save against spells
      for half - 0x1D7C1; when its event runs not read: taken as attack
      1's hit); Death Knights now has no effect without a fact. v0.196.45: 0x47 the death knights reflect a spell
      that isn't an area's back at its caster ("reflects the spell" - the
      original's d10 decides nothing, both branches test the same; rules4
      reflect), 0x4E their one spell 0x2F at their turn (as the computer
      casts; the effect then gone); v0.196.44: 0x48 their fear (every
      foe not 0x5C-proof saves against spells or is terrified - Fear's
      effect, fleeing; rules4 terror) - run at the fight's set-up: its
      event not read, so maybe each round in the original. v0.196.39 Treasures (table DS
      0x9E34): 0x29 missiles of plus 0 avoided, 0x5E the dwarves'
      Constitution bonus with poison too (also Darkness's 0x5E), 0x84 the
      harpy's charm (Gateway's 0x9A), 0x85 / 0x8D the stone guardian (magic,
      cold, fire, electricity half; piercing a quarter, flag-0x10 weapons
      nothing), 0x86 inert, 0x87 / 0x88 / 0x8B Gateway's glacier / name-word
      doubles (words 0x80 / 0x7F / 0x81 - a word, not a byte), 0x99 the
      yetis' fire half again, 0x9B the remorhaz's swallow (rules4 dmg22).
      Left in Treasures: 0x80-0x82 (the Zhentil captain's, the Hosttower
      sorceress's, the kraken master's: game block +0x3C set - +0x3D = 0 /
      1 / 2, +0x3B bits 1 / 2 / 4, [0xF2A0] = 1, the effect off, 0x23CDE;
      when, not read), 0x8C (the green hag, 0x245F6 - field / sight
      changes), 0x8E (immune helper 0 in an event not found), 0x90 (the
      giant kampfult, 0x24AEA), 0x93 (the stone golem, 0x24E74), 0x94 (the
      white gorgimera, 0x25025), 0x98 (the yetis' fear - 0x6F for 2 rounds
      round them, 0x254D1). v0.196.43: 0x91 the giant rat (a hit's 5 in 100:
      a save against poison or diseased), 0x92 the great vilstrak (its
      moves doubled in the first round). v0.196.41: 0x9A the
      ice hound and 0x8A the hell hound (one next to it: 7 cold / fire, a
      breath save for half - "Breathes Ice" / "Breathes Fire"), 0x89 the
      giant slug (Curse's spit, 1d13 + 3); v0.196.42: 0x8F the manticore's
      six spines (its pick within 10, each a to-hit roll for 1d6 - the
      original's roll hooks left out). v0.196.40: the yetis' hug 0x95 /
      0x96 / 0x97 (0x251A0: a blow's roll of 20 "has been grabbed" - 0x9C,
      no moves -, its turn "is squeezing NAME" 2d8 / 4d5 / 6d4 and over,
      its fall lets go; rules4 yeti). Not exact yet: the original's 20 is
      the d20 with the roll's modifiers (a Blessed 19 grabs), the engine's
      the d20 alone; "is released" isn't said (fall() has no pages); the
      Queen's 0x75, 0x85, 0xC3, 0xC5, 0xC6, 0xD1, 0xC7, 0xCE, 0xD0, 0xD3,
      0xD4, 0xC9.
- [x] Empty fight words (v0.196.32, fx_check's new E1: a page of the names
      alone): Champions' turn effects (runs away, is confused, goes berserk,
      is enraged - GAME.OVR 0x11936 on), "is Paralyzed" (0x10BE8), "Avoids
      it" (0x11F0E), "is charmed" (0x31EF3), "Breathes!" (0x32698), "breathes
      acid" (0x32E24), "breathes " + Fire / cold (0x3306F); Death Knights'
      "breathes " + fire / acid / death and its lightning's "breathes " alone
      (START.EXE image 0xDFB8 on; GAME.OVR 0x2ED06); Secret's "paralyzed by
      poison" (0x10CCB); the Dark Queen's breaths (above), "Suffocates", "is
      weakened", "dies from poison", "sweeps". Words a game has none of (no
      page, as its program says nothing): Secret's "is silenced", Darkness's
      / the Queen's stinking-cloud line, the Krynn DAX games' "ages"
      (profile::duel_word's kMore list; FXCHECK fuzz of all nine clean of E1).
      v0.196.33: "is weakened" (Cause Disease's weakness) in Darkness
      (0x2C55B), Champions (0x12045), Death Knights (image 0xDD6B) and
      Treasures (0x35713) - Secret has none: no page.
- [x] Darkness FXCHECK fuzz (seeds 32 / 33): F1 - a big undead destroyed by
      Turn Undead kept no down_size, its skull on one square (v0.196.33).
- [x] Queen fuzz (QAALL=1, seeds 12 / 13, 60 fights; there before v0.196.26):
      R10 against the heads (MONCHA 65 / 71 - AC -10 / -11, moves 2) and
      a lone Quick archer needing a natural 20 (need 35 at +11): 60 rounds
      without a 20 is 5% - R10's window is 100 rounds now (0.6%); a fight
      once unharmable (the dragon's body, fleeing but idle) is skipped; R3
      skips an Aurak getting up. Queen seeds 1-35 clean.
- [ ] The Dark Queen underwater (found round 21): game byte 0x3C set =
      under water; DQK.EXE tests it in 24 places. v0.196.10 (round 22,
      the Project's rules_round21_facts.md 3): the fire spells (Burning
      Hands, Fireball, Flame Strike, Delayed Blast Fireball, Fire Touch,
      Meteor Swarm, the Necklace of Missiles) "That has no effect
      underwater!" (a player's pick refused, the spell kept - 0x2731D;
      otherwise spent); Lightning Bolt a burst within 2 of the aim; Ice
      Storm 1d10 over 3; no missile fire; moves a third without effect
      0x96; the AI's rating (v0.196.9) - rules4 ONLY=qwater. v0.196.11:
      the party's blows (effect 0x95, the attacker's damage event 4,
      handler 0x503A5): a weapon with a range 0, a blow type other than 0
      damage / (the first blow's bonus + 1 - record 0x196, the bonus, as
      the original's code has it). v0.196.12: Fire Shield's hot flame "no
      effect" (0x57DE0 - in fights and camp, the spell spent; the cold one
      as on land); an item of spell 64 "Nothing Happens", not used
      (0x5EDBD). v0.196.13: the melee refusal's "Can't use that
      underwater" before "Not with that weapon" (0x239CC). Left: 0x5BA58 / 0x5BA89 - read (round 22): the area spells'
      picture routine (spells 0x22 0x2F 0x33 0x57 0x5B 0x73 0x85) picks
      another burst picture set under water (3 for 2) and adds one (9,
      0x14, 2) - pictures only, not followed. The scripts do
      set it: sweep events VMWATCH=3c (Queen, 44 of its blocks swept) -
      ECL.GLB blocks 1, 2, 3, 14, 15. the Dark Queen's ground 0x46 - laid by effect 0x9A's
      handler (DQK.EXE 0x50A77) as a cloud record of 1d6 at the holder's
      square; MONCHA.GLB's carrier: the Bozak Draconian (78) - when it
      runs (its death?) and what the square does besides stopping steps
      (under 7 Hit Dice) not read, so not laid; Gateway's magic-user's
      distance for multi-square fighters (its area list's distance /
      2 - the list builder's for big fighters not read); a computer spell
      with a casting time aborts at its start, not as it goes off; Darkness
      / Treasures memorized places 84-140 (the engine's fighters hold 84) -
      read (round 22): every MON*CHA record with spells keeps them under
      place 84 (Darkness 41, Treasures 20), and the party's are packed into
      the first 84 (party.cpp pack_memorized) - only an 85th memorized
      spell is out of the computer's reach (Quick);
      Secret's chooser gate on effect 0x44 - read (round 22): the AI's
      chooser 0xD4C6 (0xD4F0) returns no spell for one with 0x44, as its
      cast (0x2DB00) - v0.196.20: can_cast off at each round's start
      for a feebleminded one in Secret (rules4 ONLY=sfeeble).
- [x] Round 19 left (round 20, 2026-11-10: "moves / 2" the same test;
      the Queen's morale figure kept; ground 0x46 and Gateway's distance
      moved on to Round 20 left): the Dark Queen's ground 0x46 (laid by
      DQK.EXE 0x50A77 - effect 0x9A's handler; its try 0x2E479: under 7
      Hit Dice and not driven, no step) - the engine lays no such ground,
      that ability not followed; the Queen's morale figure [0xC2DE] is
      never set to 100 (BSS, kept across fights) - the engine's 100 at each
      fight's start, Curse-type (0x10245); whether the step routine's
      "moves / 2" test is the engine's f.moves < 2 (now the guard there, as
      its routine's end) - not compared in DOSBox; Gateway's magic-user's
      distance is its scan list's length / 2 - range()'s here (multi-square
      fighters may differ by a square); the Dark Queen's packed caster
      levels' cap of 31 (charms 10 / 63 / 81 / 96 / 120, clouds, Prayer,
      Mirror Image, Power Word Stun, Otto's) not in the engine - moot while
      no caster past 31 casts them (MONCHA.GLB: Sir Discore's knight 34
      casts Heal / Cure Critical only); its 0xFF-class garbage level.
- [x] Round 18 left (round 19, v0.193.0): the Dark Queen's caster level is
      res 0xB4E0 for EVERY spell (29 call sites with the overlays' - the
      effect setter 0x5B3D9 / its duration 0x5B2FB, the damage handlers;
      round 18 saw the resident three) - queen_power already is it; its Yell
      skips the held (taunt, all three Krynn games); ITEM.DAT 218's 0x8C and
      0x5FDB0's (a once-a-day marker: "feels better", 0xB9 gone, 0x8C for
      1440 minutes) - moot: 0x8C's only fight reader is the finder's taunt
      test (the yelling kender is never in its own side's list, no monster
      carries 218, the computer never yells); its step by its mood rows
      (qstep.py, rules4 aimood). Also found: the Queen's clouds laid ground
      0x1E (Stinking Cloud) and 0 (Blade Barrier - off the field!) - now
      Darkness's 0x1D / 0x1C / 0x1B and the knives' 8d8.
- [x] Round 18 left: the Dark Queen's caster level - DQK.EXE res 0xB4E0
      is called only by the magic resistance roll (0x6FDD), spells 9 / 64
      (0x558A6) and 79 (0x26D93): where its other spells' levels come from
      (0x2FFC:0x10F9?) not read - the engine uses queen_power for all; its
      Yell (0x6CF5) skips the held (the guard list) - the engine's taunt
      doesn't; ITEM.DAT 218 (type 47) carries 0x8C while readied, and
      0x5FDB0 gives 0x8C for 0x5A0 outside fights - not followed; the
      Dark Queen's computer step (its move step 0x2E94D's step routine)
      still the engine's own, its mood rows not read.
- [x] Round 7 left (round 17, 2026-11-10: Treasures' 0x3B moot - none of
      its items or monsters carries it, no handler to follow; the training
      spell list offered - done by round 9's rules4 trainspell, all nine
      games; round 18: the Dark Queen's 0xFF-class spells' caster level -
      res 0xB4E0 leaves it unset (stack garbage, read only by the magic
      resistance roll 0x6FDD) - not reproducible, the engine's 12 kept). Treasures' effect 0x3B's handler (none of its items
      has it - round 10 left it: nothing in the game carries it). The Dark Queen: its weapon name's trailing space (0x9FDC not
      read); its fight pages' rows after a hit (each page from row 10 here:
      its print routine 0xB14E takes the row from its 137 callers - the
      damage text 0x8F04 not followed through) and the box's page-full wait
      (0xCEEA: a key, cleared, on from the top - rows past 21 dropped
      here); turning (menu and computer: taken as Darkness's - its routine
      not found yet; its turn table at DS 0x5A6A, 11 a row, 99 first); its
      0xFF-class spells' caster level (uninitialised in res 0xB4E0: 12
      here). (Round 8 did the weapon space, the pages' rows and full box,
      turning, the moon's extras, Death Knights' items, Champions' finder
      limit and can-learn.) When each original shows the training
      spell list at all - not compared. Gateway's test save A remade
      2026-11-09 (make_saves.sh).
      [x] 2026-11-09: Death Knights' Add Character list was always empty
      (the NPC byte read at Darkness's 0x147, past its 0xD8 record - now
      0x3E), its saved games' members are .GAM, and an added character's
      .WIZ spells were never read; Champions' list read Curse's 0xF7 (its
      NPC byte is 0xE7). make_saves: Knights B, Champions C.
- [ ] Tom: Darkness's, Treasures', Death Knights' and the Dark Queen's
      computer never uses an item (their item routine is compiled out or
      never keeps one - round 8 - kept as the originals; a Game Fix to
      give monsters their wands back? not added: not an AD&D mistake).
- [ ] Pool's story beyond the nomads' camp: targeted differ runs only
      (scripted places), fights judged by the rule checks. 2026-11-07
      (part 17, claude/playtest_poolrad.md): every event square of
      Valjevo (5:3-7), the Buccaneers' base (6:1), Zhentil outpost (6:28),
      Sorcerer's island (7:22/23), the kobold caves (8:13), the lizard
      men's keep (8:16), Stojanow Gate (2:9) run against the original
      (saves made in the block, a scenario a square, the choices of its
      first menu each). Fixed (Pool): the NPCs' share of treasure coins;
      the step leaves the menu line drawn; size-0 NPC icons small; the
      field's scattered pieces (ovr11 +0x0, d100 a square) and no
      tables-in-rooms d10s; no Cast in a no-magic area (fight menu, the
      computer, camp - Curse / Secret too); PROTECTION runes in the box
      only after a CLEAR BOX; a text's hanging last space; PIC 0x78 and up
      not big pictures; a script's move after the step keeps the old view
      (SPRITE OFF redraws at the new square); C04F written by scripts and
      CALL 2C90 taking the square's flags again (Stojanow's ettins' Flee).
      Left: the silver dragon's
      cave (6:19), the wildernesses 25 / 27, the graveyard's north, the
      finale (Tyranthraxus); the computer's first fight turn (known, its
      d8 / d4 / d7 rolls); the wyvern fight's first turn order (8:13,
      after a surprise); Dirten's small icon 2-3 pixels at the head
      (fixed v0.176.x, below).
- [x] Tom: Dirten's graphics (PoolRad). The original builds a character
      icon by loading head / body with a see-through plane made from
      their colour 0 (ANDed, ovr18 +0x32F), then recolours (START
      0x649C) - a part recoloured to 0 stays drawn. The engine took
      colour 0 as see-through after recolouring: Dirten's record (size 0,
      every colour 0 - made by the part 13 tools' saves; a real ADD NPC
      gives size 2 and d6 colours, checked in DOSBox at the bishop's
      study) lost his outline over the wall behind him. Now an icon with
      a colour nibble 0 keeps its own see-through plane (Pic4::own_mask,
      every 16-colour game - Curse's loader is the same, alter_icon_facts
      1.6; also the icon editor's black parts): battlefield, View's Icon
      Portrait, the Companion's Party view, the editor's pair and big
      preview. View's Icon Portrait drew Pool's NPCs from CPIC (not
      Pool's rule) - now their own icon. Differ: Valjevo 12,15 (/C, the
      first fight screen) 0 px at Dirten (DOS memory: slot 6's pixels and
      plane = the model; WARRIOR, Bakshi, Phineas too); the bishop's
      join: the record identical with the d6s told.
- [x] Claude: the fight panel's weapon line - PoolRad's original wraps
      "TWO-HANDED SWORD" (16 letters) to a second line (SWORD at column
      23 of row 8), the engine fits it on row 7 (panel_text's 16
      columns). Find the original's margin (and Curse's / Secret's).
      (2026-11-07) The originals' boxed print (cols 23-38, rows 7-9, the
      same box in all eight games) takes a word WITH its trailing space
      and the names keep their last word's space (" " / "s " added):
      PoolRad / Curse / Secret / Darkness / Champions / Knights (one
      routine - PoolRad START 0x77F8) put a word on the line only if its
      space fits too (and split after - ! , . : ; ?); Gateway / Treasures
      (GAME.OVR 0x51EC5) drop a space just past the box and end the row
      after a word on the last column (status a row lower). Now
      `panel_box_text` (also Kalistes' weapon line). Differ: Dirten's
      scenario unmasked 0 px (558 px before). Left: the Dark Queen's
      print not read (kept the old fit); the fight messages (pages) keep
      panel_text - whether the originals print them through the same box
      not checked. 2026-11-08: they do (PoolRad ovr14 +0x2FA; the same
      routine in Curse / Secret / Darkness / Gateway / Treasures /
      Champions): the name printed as it is, the words through the box
      from the next row, the next print at the box's last row + 1 (+1
      after the last column); the target's name in its colour before the
      blow; a fall after a hit goes on below the hit's words ("goes down"
      / "and is Dying", or "goes down" + the name + "is killed"), the
      rows above kept. Now show_page / down_page(after_hit). Proved with
      the differ's new frame capture (DOSBox AUTODOS_FRAMES, the
      playsim's PAGESHOTS): every fight-page row layout (colour + width a
      row) the engine showed in the kobolds / slums fights that the
      original also showed matched (18 / 3 shared layouts, the rest other
      names / dice); pixel-identical pages need the computer's dice (not
      told yet). The Dark Queen's message print still not read.
- [x] Claude: does Curse's / Secret's step also leave the exploring menu
      line drawn under a VERTICAL MENU, keep the old view after a script's
      move, and lack the field's d100 pieces (Pool's fixes, Pool only)?
      (2026-11-07, the originals' code; facts in the Project's
      claude/crossgame_pool_fixes_facts.md.) Field: the d100 scatter is
      Pool's alone (no such routine in Curse / Secret / Darkness /
      Gateway / Krynn); tables and chairs in rooms (d10s) are in Curse's
      (coab sub_370D3, GAME.OVR 0xD936), Secret's (0xEC85), Gateway's,
      Champions' and Death Knights' builders, NOT in Darkness's (0xBD45)
      or Treasures' (0xCC56) - no dice there: the engine rolled them for
      both, fixed (rooms off for the VGA games). Menu line / old view:
      Curse's main loop (coab ovr003 / ovr015) is Pool's - no clear on a
      step, the view drawn before the square's script - but every move
      in Curse's, Secret's and Darkness's scripts is followed by its
      redraw (CALL 2E10 / 2DCB / CALL 1 or END) and every vertical menu
      by a wait that writes the menu line (PRESS BUTTON / PRESS KEY), so
      neither difference can show there: no change. Not run in DOSBox
      (no reachable case); Curse's DOSBox starts with "start STING
      STING" (no title, no code wheel).
- [ ] Tom (2026-11-06): the 480x320 Game / Look keys are too large - that
      space under the Companion panel was meant for Companion features once
      the games run well; revisit then.
- [x] Tom: the party list beside a menu line (exploring, camp, Magic,
      Alter) shows the chosen member in white letters, as the originals do;
      should it get the white box too ("lists' lines too")? Krynn's Modify
      marks the stat being changed in magenta. Tom, 2026-11-06: the party
      list keeps the white letters, no box (no change); the Cheats
      Trainer's Modify was unmarked in v0.173.0 by a misunderstanding;
      Tom: you can't tell what you're changing - the marking is back in
      both (v0.173.1).
- [x] Tom (2026-11-06): Right Hand / Left Hand setting for the Play
      Test's keys (Settings key; next version).
