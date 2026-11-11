Round 7 scratch (2026-11-08) - how each run is invoked. Scripts kept in the repo (tools/devkit) so a replaced machine loses nothing; game files, unpacked programs and outputs stay in scratch.

Games: /tmp/claude-0/games/<short>/<GOG folder>  (unzip_games.sh from Tom's zips)
  short     GAME  folder used                              env (run.sh sets it)
  poolrad   1     Pool of Radiance/POOLRAD (DAX one down)
  curse     2     Curse of the Azure Bonds
  secret    3     Secret of the Silver Blades
  darkness  4     Pools of Darkness
  champions 5     Champions of Krynn                       MAREA=2 MBLOCK=34 FIGHTAT=8441
  knights   6     Death Knights of Krynn                   BOOTSLOT=A MAREA=1 MBLOCK=36 FIGHTAT=815B
                  (the old test save B is gone: save A starts a new game; MAREA/MBLOCK put the
                   party in block 36, whose LOAD MONSTER is at 815B)
  queen     7     The Dark Queen of Krynn                  FIGHTAT=8C11
  gateway   8     Gateway to the Savage Frontier           (NO SAVES: its SAVE folder is empty - the
                  earlier rounds' test saves are gone; rules4 / fights can't boot)
  treasures 9     Treasures of the Savage Frontier         CACHE=/GOLDBOX/_CYD/<folder>

Scripts:
  build.sh <name> "<flags>" <main.cpp> [extra .cpp]   objects per flag set in obj/<name>, binary bin/<name>
    rules4:  build.sh r4 "-fsanitize=address,undefined -DCYD_TEST_HOOKS -DBEHAVE_MATRIX -DBEHAVE_ROUND4" tools/playsim/sweep.cpp
    budget:  LDEXTRA="-Wl,--wrap=malloc,--wrap=free,--wrap=calloc,--wrap=realloc" build.sh sim "-fno-omit-frame-pointer" tools/playsim/sim.cpp tools/playsim/heapprof.cpp
    (header changes: REBUILD=1 to rebuild every object)
  run.sh <short> <binary> <args...>     a fresh copy of the game folder in run/<short>[-TAG] with _CYD beside it,
                                        the game's env, ORACLE_DIR=/tmp/claude-0/bt (empty now)
  r4all.sh [games]                      behave rules4 on each game, logs/r4_<game>.log (2 at a time)
  budget.sh                             fight budgets, logs/bud_<game>.log: SAVE=A RUNAT=... FIGHT=1 HEAPPROF=1 BUDGET=150
                                        (Curse 8AC2, PoolRad 85F9, Darkness 85BB CHOICES=0, Champions AREA=2 BLOCK=34 8441,
                                         Knights AREA=1 BLOCK=36 815B, Queen 8C11, Treasures 9C1D)
  host_tests.sh                         CI's tests job (ht/)
  syntax2b.sh                           CLAUDE.md's gnu++2b check
  pio_setup.sh / board_build.sh         PlatformIO venv (pio-venv) + proxy CA, pio run -e cyd28_ili9341_res, .pio/build removed
Tools (tools/): unpack (EXEPACK image), stub.py (overlay stub seg:off -> GAME.OVR offset), ovls.py (overlay list /
  which overlay holds an offset), dis.sh (ndisasm from a file offset, overlay-relative addresses).
  Unpacked images: tr_exe.bin (Treasures), dk_exe.bin (Darkness), dkk_exe.bin (Knights START.EXE), sec_exe.bin (Secret
  START.EXE); GAME.OVR copies tr = ovr.bin, dk_ovr.bin, dkk_ovr.bin, sec_ovr.bin.

Baselines this round (v0.179.0 work):
  rules4: PoolRad 14/0, Curse 16/0, Secret 12/0, Darkness 12/0, Champions 19/0, Knights 15/0, Queen 14/0,
          Treasures 14/0; Gateway not run (no save).
  budgets (bytes, limit 153600): Curse 133545, PoolRad 135385, Darkness 149482, Champions 135815, Knights 135762,
          Queen 143068, Treasures 151742.
  behave (oracle-based per-game runs): NOT reproducible - the scratch oracles (/tmp/claude-0/bt/oracle_*.txt) and
          ITEMS_BIN are gone and the repo has no generator for them (pool_oracle.py etc. were scratch scripts).
          Curse's behave without them: 74 synthetic checks pass, 6 parts fail "no oracle".
  DOSBox differ: not rebuilt (its custom build is gone).

Round 8 rebuild (2026-11-08) - the behave oracles, ITEMS_BIN and test saves:
  oracles/<name>.py      one generator per oracle, reads the player's program (tools/unpack -> /tmp/claude-0/prog)
                         at profile.cpp's addresses, writes /tmp/claude-0/bt/oracle_<name>.txt. gb.py: shared readers
                         and the AD&D Strength / Dexterity tables. Rebuilt: items spells monsters classes (Curse), pool,
                         secret secret_monsters secret_spells secret_items secret_situations, gateway (creation only).
                         NOT rebuilt (facts only in the Project docs): events (Curse's triggers - an independent ECL
                         reader), Secret's sp_word rows, Gateway's list_limit / menus, darkness(+_fights, _left),
                         treasures(+_fights, _left), champions, deathknights, queen, lv_<game> (behave levels).
  items_bin.sh [games]   ITEMS_BIN (sweep testchars ALLITEMS=1) -> /tmp/claude-0/sw/items_<short>.bin (+ items.bin)
  make_saves.sh <g> [slot] [race,sex,class,align:NAME ...]   a test party made with the playsim (CREATE x6,
                         ADDGUYS, SAVENOW) into the scratch game's SAVE and /tmp/claude-0/saves/<g>. Gateway A works;
                         Death Knights B and Champions C made 2026-11-09 (the Add list fixed).
  behave.sh [games]      the per-game behave runs (oracles, build b<game>, run) -> logs/behave_<game>.log + summary.
  Baselines this round: Curse 6002/1 (the 1: no oracle_events.txt), PoolRad 430/0/0, Secret 5312/0/0,
  Gateway rules4 11/0 (save A made).

Round 9 (2026-11-08) - more oracles rebuilt from the programs' code (no Project docs were readable this round):
  oracles/x86.py         ndisasm-based readers of the originals' code: Code (a stretch disassembled, find / imm),
                         run (a tiny interpreter for cmp / jcc chains and byte inc / dec / add / sub / mov to
                         named places - used to run a routine for every input), cases (a cmp / jz / jnz switch).
  oracles/krynn.py       the Krynn creation roll's rows (statlim agebr classmin ageeff raceadj adjtested wis13).
  oracles/champions.py   Champions: tables from START.EXE, createPlayer / Modify / petition / acid constants from
                         GAME.OVR's code.                                                  behave champions 8/0/0
  oracles/deathknights.py  Death Knights: tables, createPlayer's experience / steel / spells, petitions.  4/0/0
  oracles/queen.py       the Dark Queen: DQK.EXE's tables and createPlayer / Knight New Order constants,
                         MONCHA.GLB's experience and flags (spell 0 left out: no such spell).   5/0/0
  oracles/gateway.py     + list_limit (its race-limit routine run for every case) and the shop / temple /
                         treasure menus' words with their guards (Detect left out).            3/0/0
  oracles/secret_situations.py  silent training's lists now read from GAME.OVR 0x22921.  secret 5312/0/0
  Still NOT rebuilt: darkness / treasures (+ _fights, _left), events (Curse's triggers - an independent ECL
  reader), lv_<game> (behave levels), Secret's sp_word rows; the DOSBox differ. Death Knights' test save B:
  make_saves.sh can't make it - an engine bug: the Add Character list reads a Death Knights .PC with
  Darkness's NPC byte (0x147) from a 216-byte record, so no .PC is ever listed (play.cpp, the add list).

Round 10 (2026-11-08) - the last oracles, behave levels' oracle, the DOSBox differ:
  oracles/darkness.py    oracle_darkness / _fights / _left (darkness.py treasures, or treasures.py: Treasures'
                         - its tables at its own places, treasures_port_facts.md). The program's tables, ITEM0.DAX
                         block 1 (the kit), the spell handlers' words read from GAME.OVR (the handler table's
                         filling run found by its shape, each handler to its first ret: the Pascal strings it
                         loads and the "is " + name it joins), the facts' rules worked out (43.1-43.4 hit points
                         and Modify's bounds, 43.2 the Constitution, L1 / L2 the drain, R11 Restoration).
                                                                  behave darkness 154/0/0, treasures 155/0/0
  oracles/secret_words.py  Secret's sp_word rows (secret_facts_2.md 14) appended to oracle_secret_spells.txt
                         (behave.sh runs it after secret_spells.py).                      behave secret 5413/0/0
  oracles/lv_oracle.py   oracle_lv_<game>.txt for behave levels' values / multi: curse (every value: Curse's
                         tables + behaviour_facts.md's class rules), darkness / treasures (experience, the last
                         level, hit points - 43.1; no THAC0 / save / attack / slot / thief rows: not in the facts).
  levels.sh [games]      behave levels on curse darkness treasures (build blv).   11/0/0 each
  differ/                the DOSBox differ (PoolRad):
    fixseed.py           START.EXE un-EXEPACKed (relocations from EXEPACK's table) with Randomize seeded 0
    autodiff.py          base / run / batch: the original in dosbox-auto (headless, a key script, shots) and the
                         playsim (simh0: build.sh simh0 "-DCYD_TEST_HOOKS -DCYD_MENU_HIGHLIGHT=0") on the same
                         tokens, each step's screens compared by EGA index (the engine's lit menu word forgiven)
    mkscen.py            a block's scenarios from the engine alone: event entries from the map ('M'), a save at
                         each (TELE0 + SAVENOW), tokens step by step from the engine's state ('?')
    runall.sh            base + blocks -> /tmp/claude-0/differ/batch_report.txt
  NOT in the repo (GPL, kept in the Project): build_dosbox.sh + autodos.inc (DOSBox 0.74-3 patched: the key
  script, quiet / shot / poke, AUTODOS_MEM, AUTODOS_FRAMES) -> /tmp/claude-0/differ/bin/dosbox-auto.
  Still NOT rebuilt: Curse's oracle_events.txt (needs the Project's ecl_stub_facts.md - Curse's opcodes and
  operand counts - for an independent reader); lv rows past Curse's / Darkness's hit points.

Round 8 (2026-11-09) - the Dark Queen's code read from DQK.EXE itself (no EXEPACK):
  ndisasm -b16 -e 0x2400 -o 0x2400 DQK.EXE > /tmp/claude-0/ag8/dqk_all.lst   (addresses = file offsets)
  tools/dqk_addr.py <DQK.EXE> seg:off ...   a far call -> file offset (resident, or a stub -> its RTLink overlay)
  tools/lst_win.py <listing> <addr> <before> <after>   the lines round an address
  tools/lst_callers.py <listing> "<call text>" [n]     every call site with the n lines before it (the pushed rows)
  rules4 checks added: aiorder, aiturn, qpages (+ turn's rule-4 / penalty cases, moons' queen_moon_room, wrap's full
  boxes, canuse's Champions clause). Binaries this round: bin/r4w, bin/simw (built from the worktree).

Round 9 rules (2026-11-09):
  tools/lst_b14e.py <DQK.EXE> <listing>   every call of the Dark Queen's fight-page print res 0xB14E with its
                                         pushed wait / row / text (the DS words read from DQK.EXE)
  rules4 checks added: qlost (the Dark Queen's "lost a spell", undead falls, fixed rows, the full box's
  prompt), bandorder (Death Knights / the Dark Queen bandage after Turn / spells); trainspell on all nine.
  economy: Champions' Id with all its coins, Appraise paid in steel. campfuzz: Darkness's save A camps in a
  town (no Rest) - CFSANCT=1 to rest there. fightfuzz: a lost opening fight while booting (Death Knights'
  save A) is passed over, not a problem.
  Baselines: rules4 PoolRad 16/0, Curse 18/0, Secret 14/0, Darkness 14/0, Champions 22/0, Knights 19/0,
  Queen 30/0, Gateway 13/0, Treasures 16/0; economy 12/0 each; budgets (bytes) Curse 133643, PoolRad
  135483, Darkness 149580, Champions 135913, Knights 135860, Queen 143166, Treasures 151840.

Round 9 run scripts (kept from scratch, 2026-11-09; logs in $LOGS, default /tmp/claude-0/ag9):
  beh.sh <bin> <mode> <ONLY> <tag> games...   behave <mode> on each game, one at a time
  r4only.sh <bin> <ONLY> <tag> games...       behave rules4 ONLY=... a summary line a game
  r4full.sh <bin> <tag> games...              full behave rules4, 3 at a time
  fuzz.sh <bin> <game> <fights> seeds...      fightfuzz, a line a seed (rule breaks, sanitizer errors)
  budgetx.sh                                  budget.sh with the simx binary (logs budx_<game>.log)

Round 10 rules (2026-11-09):
  oracles/appraise.py <champions|knights|queen> <GAME.OVR | DQK.EXE>   Appraise's gem / jewel value for every d100
                         from the original's code (GAME.OVR's compare chains; DQK.EXE's two 100-case tables)
                         -> /tmp/claude-0/bt/oracle_appraise_<game>.txt, read by behave economy G2
  economy adds: G2 (Krynn: each d100's value), L1 / K1 (Champions' coin lists - the counter's 0-6 up, View's 6-1 -
                and its scripts' coin codes 0xBB-0xC3), Champions' W1 / I1 from its record (to_krynn) and DS 0x0F46's
                worths (ONLY=coins for L1 / K1)
  rules4 adds: qlost 7 / 8 (the Dark Queen's Feeblemind and stinking-cloud coughing lose the spell under way),
               aiturn (Champions' turn-start dice for every mood and face; the Turn limit 2 after a d8 of 8)
  fight_check: FCROLLS=1 (each blow's rolls under FCACTLOG), FCACTN=n (how many actions FCACTLOG prints), the AC
               in each fighter's line; R10 reports a fight past FIGHTCAP only when no hit points changed in 60
               rounds (or past 4 x FIGHTCAP) - the fuzzer gives up at that point
  budgetx.sh: SIMBIN=<binary> for another build.
  Header changes (combat.h's Fighter): REBUILD=1 for every binary, or the objects disagree (a crash in place()).
  verify10.sh: round 10's whole verification batch (rebuilds with REBUILD=1, rules4 / economy / fuzz / budgets / saveload); tools/refs.py <listing> <text> [before] [after]: a listing grep.

Round 11 rules (2026-11-09):
  tools/tohit.py <GAME.OVR | DQK.EXE> ...   every to-hit routine found by its shape (a d20 kept, 20 made 100), each
                                         disassembled
  oracles/tohit.py [games]               oracle_tohit_<game>.txt: the attacker-against-target roll's side bonuses
                                         (the bytes it loads, as script addresses) and its difficulty switch walked
                                         for each value 0-6 (fixadj: Secret's / Death Knights' / Champions' switch as
                                         if it tested the difficulty word it names) - read by rules4 tohit
  tools/dqk_addr.py                      now also prints a seg:off as a call from inside an overlay (no stub: the
                                         overlay's own segment; size = row word 6)
  rules4 adds: tohit (all nine: play::side_bonuses for every difficulty, Game Fixes off and on), aimood
               (Champions: its step rows by mood - the table found from GAME.OVR's code, read from the program -
               and the step back), qlost 7-9 (the Dark Queen's Feeblemind effect / Int / Wis / memorizing, its
               cloud's 1d4 + 1 and who breathes nothing), aiorder / bandorder on Champions
  verify11.sh                            round 11's batch (oracle, rebuilds, rules4 / economy all nine, budgets, fuzz,
                                         saveload all nine) -> /tmp/claude-0/ag11

Quick Actions + round 12 bandaging (2026-11-09):
  rules4 ONLY=bandorder    the computer's bandaging as each original's code has it (all nine): in the move step, after
                           Turn Undead, before the blow; PoolRad never (GAME.OVR 0xA806's callers)
  rules4 ONLY=quickacts    each column on / off in a cleared-field scene (Melee, Missile - a thrown weapon from the
                           game's ITEMS, Move, Turn Undead, Guard, Items / Spells, Bandage First before the blow and
                           before Turn), the defaults = the switch off, NPCs ignore the grid, QUICK.TXT round trip
                           (12 odd names, a missing file); QADEBUG=1 prints each scene
  rules4 ONLY=quickpage    the page in exploring and in a fight: cursor keys, Select, taps (lit as they touch), a
                           heading, the "?" help, Exit, the game screen back byte for byte; QASHOT=dir: its canvases
  fightfuzz QAFUZZ=1       random grids (half the fights all on Quick), fight_check R13: no action an unticked switch
                           forbids; QAOFF=1: the switch off
  qa_verify.sh <bin> [games]   the fuzz (3 seeds x 40) + the defaults-vs-off action-log comparison (2 seeds x 25)
  saveload.sh <bin> [games]    sweep saveload on each game
  playsim CAMPRUN ops: '|' the Quick tab's rows (quick_begin), ',' toggle row 0's next column; 'v' '^' Up / Down
  fuzz.sh TAGP=x, budgetx.sh LOGS= / TAGP=, host_tests.sh HTOUT= (parallel agents); host_tests.sh now pipefail
  Baselines: rules4 full PoolRad 30/0, Curse 33/0, Secret 27/0, Darkness 29/0, Gateway 28/0, Treasures 31/0,
  Champions 41/0, Knights 33/0, Queen 51/0; budgets (bytes) Curse 133679, PoolRad 135330, Darkness 149976,
  Champions 135886, Knights 135896, Queen 143202, Treasures 151876.

Quick's spells = the originals' Magic On (2026-11-09): tools/devkit/quickmagic/ (README.txt there) - each game's
Alt-M flag, AI gate and fight-start reset (magic_find.py), the DOSBox runs that showed it (dosrun.py, mkscr.py,
mkfight.py, memchk.py; Curse's bar fight, PoolRad's goblin guards), rwalk.sh. rules4 ONLY=quickacts now has
qa_spells (all nine). QUICK.TXT "# Quick Actions 2:" (v0.184.0 files read with Spl unticked).

The Quick tab as an engine page + the party's NPCs (2026-11-09, round 14):
  rules4 ONLY=quickpage    now the tab's model (play::quick_begin / quick_name / quick_npc / quick_get / quick_toggle):
                           the whole party's rows, each toggle written at once and read back, the game screen and the
                           fight untouched, not while typing; ONLY=quickacts adds the NPC cases (kComputer = the switch
                           off, Melee / Move / Spells unticked limit an NPC) and the NPC file case
  fightfuzz FUZZTAPQ=n      one pass in n a tap on the field during pages / the computer's play (with QAFUZZ=1):
                           fight_check R14 - the tap moves no page, leaves no party character on Quick, and the
                           computer never acts for one taken off (Tom, 2026-11-10)
  fightfuzz QANPC=1        the party's last member made an NPC for the fights (R13 checks NPCs too)
  r4all.sh                 BIN= LOGS= TAGP= (parallel agents); board_build.sh [env] (KEEP=1 keeps .pio/build/<env>)
  playsim MAKENPC=k        member k made an NPC before SAVENOW (a test save with an NPC; scratch only)
  qemu_shots.sh [dir]      real screenshots of the engine's screens from the firmware in QEMU - the S3 builds' frame
                           buffers (qemus3: boards/qemu_fb.hpp 480x320; qemus3_50: Panel_Double 320x240), TAPS.TXT
                           "shot <name>", run_qemu.py --shots dir (PNG); Home -> Curse -> Play -> save A (with an NPC)
                           -> Game -> Quick (open, Spells tapped, the NPC, Bandage First, "?"), Settings, Cheats.
                           QEMU=qemu-system-xtensa (Espressif esp-develop-9.2.2-20250817; apt: dosfstools mtools),
                           SIM=a playsim. The PNGs show game screens: scratch only.

Round 15 (2026-11-10): the computer's step and Feeblemind
  oracles/aistep.py [games]   oracle_aistep_<game>.txt (the eight DAX games): each GAME.OVR's try (a mood row's
                              column: the table read just after a routine's start) and step (the table read again after
                              the flee's way): the rows' DS place, the facing byte and the +1, the cost test (strict /
                              PoolRad's), the cloud squares' rules (effects, driven, flags 2, Hit Dice, a save)
  rules4 ONLY=aimood          all eight: every mood x way x column, the step back three times, the cost at the moves'
                              edge, a run's way (both sides), each cloud rule case by case (effects, driven, HD, flags 2,
                              the save made / failed by the d20 hook)
  rules4 ONLY=feeble          Darkness / Treasures (effect 0x44 kept, Int / Wis 3, memorizing forgotten, the spell left
                              for "lost a spell", "loses intellect"; the recalculation keeps 3), Death Knights (Game
                              Fixes off: drain-2 effect, the target's target drained, "gets drained"; on: Darkness's)
  host test test_queen_damage  the Dark Queen's DAMAGE save branches (ecl::queen_damage)
  Baselines: rules4 full PoolRad 38/0, Curse 41/0, Secret 35/0, Darkness 39/0, Gateway 36/0, Treasures 41/0,
  Champions 47/0, Knights 43/0, Queen 54/0; budgets (bytes) Curse 133721, PoolRad 135372, Darkness 149721,
  Champions 135928, Knights 135938, Queen 143244, Treasures 151918.

Round 16 (2026-11-10): the Dark Queen's hooks, the stats' effects, the step back's finder, Knights' on-hit
  oracles/qhooks.py <DQK.EXE>        oracle_qhooks.txt: DQK.EXE's hook switch (0x7202, by shape), its events 0xA
                                     (the attacker's), 0x10 (the target's), 0xB (the target's AC) - each effect's
                                     handler (DS table + 4 x effect, from the dispatcher "call far [bx+T]" and the
                                     setter's mov [T+2],cs|seg / mov [T],off pairs; the overlay base found as the one
                                     making them function starts) told as a line; and the clock's call at each round's
                                     start (0x2336C -> 0x61AD -> 0x5EFF)
  oracles/statfx.py <GAME.OVR> <g>   oracle_statfx_<g>.txt (secret darkness treasures): the stats' recalculation's
                                     Strength effects, the 18/xx levels, Constitution / Charisma / Int-Wis effects
  oracles/finder.py <GAME.OVR> <image> <g>   oracle_finder_<g>.txt (the eight DAX games; image = /tmp/claude-0/prog's
                                     unpacked program): the count-2 call's finder, its tries, scan (Darkness-type),
                                     the pass-2 sight skip
  oracles/knights_hit.py <GAME.OVR>  oracle_knights_hit.txt: Death Knights' events 2 / 3 lists and the damage test
  rules4 ONLY=qhooks (the Dark Queen: every hook line as a blow's smallest hitting d20, Game Fixes off; the roll
                      under 0, Shield + coughing, 0x45 with the fixes, the dragon fear, the clock at the round's start)
         ONLY=statfx (Secret / Darkness / Treasures)   ONLY=finder (the eight DAX games)   ONLY=khit (Knights)
  host test test_queen_target_hooks (DAMAGE's attack roll)
  Run the four oracles before rules4 (ORACLE_DIR, default /tmp/claude-0/bt).
  Baselines: rules4 full PoolRad 39/0, Curse 42/0, Secret 37/0, Darkness 41/0, Gateway 37/0, Treasures 43/0,
  Champions 48/0, Knights 45/0, Queen 57/0; budgets (bytes) unchanged from round 15 (Curse 133721, PoolRad 135372,
  Darkness 149721, Champions 135928, Knights 135938, Queen 143244, Treasures 151918).

Round 17 (2026-11-10): the computer's turn target, on-hit after no damage, the Dark Queen's 0x83 / cloak
  oracles/turnfind.py <GAME.OVR> <image> <g>   oracle_turnfind_<g>.txt (the eight DAX games): the AI loop's finder call
                                     (range 0xFF, 1, 0) and its place against the moves test (Curse-type: the finder
                                     then the moves; Darkness-type: moves first), the turn's first finder (Darkness,
                                     Treasures, Death Knights - only with no target), the finder's keep step (side,
                                     active byte, seen), a struck pick (a try spent), the first pass's walls (map +6 set
                                     only in pass 2), Champions' taunt (0x5B: the list's first entry on a pass's first try)
  oracles/onhit.py <GAME.OVR|DQK.EXE> <g>   oracle_onhit_<g>.txt (all nine): the blow's test between the target's
                                     in-the-fight test and the on-hit hook (slot + 1): needs_damage, the damage word,
                                     the Dark Queen's free effect (0xB8)
  oracles/qhooks.py                  + the 0x83 line: "tohit base 0x87 cut 2 near_str23 .. far_dex21 .." (the bonus
                                     routines run with x86.run), the damage dice it sets (lost)
  rules4 ONLY=turnfind (eight DAX games: kept behind a wall, pass 1 in sight lines, struck picks, pass 2 over walls,
         an invisible target dropped, Champions' kender, moves 0, the turn's first finder) ONLY=onhit (all nine: a poison
         holder's hit of 0 / 1d6, Darkness-type vorpal, the Queen's 0xB8) ONLY=qhooks (+ 0x83's to-hit near / far, the
         blind / 0xA2 attacker's stored AC after its blow and once hit); host test test_queen_target_hooks + the DAMAGE
         cloak's roll-0 reset before any fight and 0xCF
  Run turnfind.py / onhit.py / qhooks.py before rules4 (ORACLE_DIR). Binaries: bin/r17r4, bin/r17simx.

Round 18 (2026-11-10): the computer at moves 0, its guard, the Dark Queen's finder and 0x83's damage
  oracles/guard.py [games]    oracle_guard_<game>.txt (all nine; the Dark Queen's from DQK.EXE): the AI loop's moves
                              test and where it goes (the guard routine - the same as "not found"), the guard
                              routine's three tests (the DS held list's effects, the readied weapon's range byte over
                              1, the moves) and, for the Queen, a qfinder line (its finder 0x28AFE: tries = the list,
                              a roll before them, the taunted effect and its try 0x13, the list builder's swap of the
                              kender to the front, the turn's first finder)
  oracles/carriers.py <eff..> the Dark Queen: monsters (MONCHA.GLB effects / items), ITEM.DAT items and DQK.EXE's
                              gives that carry an effect - printed facts (0xB8: the Gas Spore; 0x83: item type 67)
  oracles/qhooks.py           + the 0x83 line's near_strdmg23 (Strength's damage at 18/00, run by x86.run) and wplus
  oracles/x86.py              run: 'mov <place>,al' and 'add / sub al,<number>' (the other oracles' outputs unchanged)
  rules4 ONLY=guard0 (all nine: think at moves 0 / fleeing at moves 0 next to a foe plans the guard, guard_takes false
         at moves 0, for each listed effect, with a missile weapon; the Queen's finder rolls, struck picks, the taunted
         list's front) ONLY=qhooks (+ 0x83's damage lost / with the Game Fixes, near and far); turnfind: a finder's
         taunt test in a game with no Yell (Secret) is moot
  verify18.sh                 the round's batch (oracles, REBUILD=1 builds ${TAG}r4 / ${TAG}simx, r4all, saveload,
                              budgetx, qa_verify) -> $V/summary.txt
  Baselines: rules4 full PoolRad 42/0, Curse 45/0, Secret 40/0, Darkness 44/0, Gateway 40/0, Treasures 46/0,
  Champions 51/0, Knights 48/0, Queen 59/0; budgets (bytes) Curse 133721, PoolRad 135372, Darkness 149829,
  Champions 135928, Knights 135938, Queen 143244, Treasures 151918.


Round 19 (2026-11-10): the Dark Queen's step and Yell, every original's step-routine tests
  oracles/qstep.py [DQK.EXE]  oracle_aistep_queen.txt in aistep.py's format: its rows (DS 0x5DB4, "stride 6",
                              rowbytes - the 7 rows' columns 1-5), the flee's way (+ 1), the cost test, its
                              clouds (0x1D / 0x1C / 0x1B: effects, driven, flags 2 - no saves), and the lines below
  oracles/aistep.py           + npc <control byte> fig <ds> health <ds> side1 (the step routine's NPC d100;
                              none in PoolRad), mage class <off> armour <0|1> then <guard|dual|keep> (Secret's
                              dual-class test, Gateway's distance), morale fig <ds> cap <0|1> (the 102 test)
  oracles/yell.py [games]     oracle_yell_<game>.txt (champions knights queen): the Yell's tests - Intelligence
                              over 3, the held routine (the guard routine's own: guard_same), the save after it
                              (run guard.py first)
  rules4 ONLY=aimood          now the Dark Queen too, + step19 (the NPC test at d100 40 / 55 with figure 10 and
                              health 60, a monster's, the magic-user's rule per game, the morale figure of a
                              control 0xC0); ONLY=yell (Champions, Death Knights, the Dark Queen)
  verify19.sh                 the round's batch (verify18's + qstep.py / yell.py) -> $V/summary.txt
  Baselines: rules4 full PoolRad 43/0, Curse 46/0, Secret 41/0, Darkness 45/0, Gateway 41/0, Treasures 47/0,
  Champions 53/0, Knights 50/0, Queen 66/0; budgets (bytes) Curse 133729, PoolRad 135380, Darkness 149747,
  Champions 135936, Knights 135946, Queen 143252, Treasures 151926.

Screenshots (2026-11-10, Tom: real firmware pictures for the README - he picks them; the PNGs show game
graphics and GOG / publisher art: scratch only, never the repo):
  shots_scan.sh qemu28:box qemu35:box qemu28:icon qemu35:icon
                         the card scan on a WRITABLE card (the classic QEMU builds; env qemu35 = the 3.5" 480x320)
                         with all nine games (+ Launcher for box art, + Curse's journal PDF) -> $SCAN/<env>-<style>/_CYD
  shots_saves.sh         slot D in every game, made by the playsim (bin/simshots: build.sh simshots "-DCYD_TEST_HOOKS"
                         tools/playsim/sim.cpp) - placed for a nice 3D view (shots_viewscan.py <short>: every square
                         and facing, a contact sheet to pick from) and in the block of the fight's LOAD MONSTER
  shots_all.py [games|engine|queen_a|mapkey:<game>|panelcheck] [--size 480|320] [--only explore,camp,fight]
                         the S3 QEMU builds (qemus3 480x320, qemus3_50 320x240) on a read-only card made of one game
                         folder + the scan's _CYD (LIBRARY.BIN cut to it): Load D, explore / Companion views / camp /
                         "runat <LOAD MONSTER>" (TAPS.TXT, CYD_QEMU only) for the fight; engine = Home (box art and
                         icons), the scan's log, a game's page, Settings, a journal entry (JOURNAL.DAT's first 2.6 MB:
                         the card is 13.5 MB), the Game menu's tabs. Shot names are cut at 23 characters by the
                         firmware's buffer. Not possible there: the Journal PDF book (the PDF + game past the card),
                         anything that saves.

Round 20 (2026-11-10) - the computer's spell choice:
  oracles/emu16.py       runs ONE routine of an original's 16-bit code in Unicorn (helpers stubbed by int 0x30 / retf);
                         needs a python with unicorn: python3 -m venv /tmp/claude-0/uvenv &&
                         /tmp/claude-0/uvenv/bin/python -m pip install unicorn
  oracles/aicast.py      the by-priority games' per-target spell use test (Darkness, Treasures, Death Knights, the Dark
                         Queen) run for every spell on ~160 states each -> oracle_aicast_<game>.txt (+ their spell rows);
                         rules4 ONLY=aicast compares the engine (combat::ai_use, the chooser, the d7 games' judge)
  verify20.sh            round 19's batch + aicast.py (UPY=the unicorn python)
