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
                         Death Knights' Add list comes back empty (its .PC files aren't listed) - B not made.
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
