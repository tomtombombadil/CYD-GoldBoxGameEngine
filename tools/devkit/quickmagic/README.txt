Quick's spells in the originals (2026-11-09, Tom: "I could have sworn that Quick casted spells").
Scratch: game copies, saves, shots and memory dumps in /tmp/claude-0/ag13 (never in the repo).

What the originals do (every game): the AI's spell step casts for one of the player's own characters only while
the fight's Magic flag is on - "cmp byte [rec+control],0x7F; ja/jnc cast; cmp byte [flag],0; jz no-spell".
Alt-M (scan code 0x32) in the fight menu and in the Quick-play key check toggles it ("Magic On" / "Magic Off");
the fight's set-up (after "A battle begins...") clears it.
  game        gate (file offset)         flag (DS)   toggles                 reset
  PoolRad     GAME.OVR 0xB187 (ctl 0x84)  0x6E30      0x9CAA, 0xBBB0          0xE457
  Curse       GAME.OVR 0xC0C8 (ctl 0xF7)  0x75F4      0xAB5D, 0xCCD3          0xF3C8
  Secret      GAME.OVR 0xD58F (ctl 0xFF)  0x8E14      0xBFC8, 0xE148          0x10090
  Gateway     GAME.OVR 0xD6FD (ctl 0xF7)  0x8901      0xC101, 0xE37C          0x10C8D
  Champions   GAME.OVR 0xCBBC (ctl 0xE7)  0x7198      0xB49F, 0xD756          0xFE5E
  Darkness    GAME.OVR 0x9FEA (ctl 0x147, jnc: >= 0x7F)  0xAEDA  0x8F23, 0xABC5  0xD3CE
  Treasures   GAME.OVR 0xAE2C (ctl 0x147, jnc)          0xA5CB  0x9BF6, 0xBA39  0xE477
  Knights     GAME.OVR 0xA830 (ctl 0x3E, jnc)           0xC4BE  0x96C0, 0xB3C6  0x1B89B
  Queen       DQK.EXE 0x2E15B (ctl 0x9B, jnc)           0xB1CB  0x22FD7 (menu), 0x2EE98 (Quick play, key 0xCC)  0x2DADC
magic_find.py finds the DAX games' rows (GAME.OVR); the Dark Queen's from the listing (ndisasm -e 0x2400 -o 0x2400
DQK.EXE): grep "0xb1cb]" - its "Magic On" string is DS 0x5E26 (DS = file 0x63A20).

DOSBox runs (dosbox-auto, Curse and PoolRad):
  Curse (ROOT=1, "START STING STING"): save A's PHILIPPE / LEDERA / SHARA given Magic Missiles, Sleep, Bless, Hold
    Person in CHRDATA4-6.SAV (bytes 0x1E on, OVERLAY=); path to The Curse tavern ("PUNCH BARKEEP"), 'p' = the bar
    fight: mkfight.py out "l,a,b,enter,enter,right,right,up,enter,enter,enter,right,up,enter,enter,enter,right,up,
    enter,enter,enter,right,right,up,up,right,up,up,enter,enter,enter,right,up,p" <poke|-> 8 2000 120000
    (8 'q' presses = everyone on Quick). Poke for Magic On: 0C5F:75F4=1 (DS 0C5F in this boot).
    Result: Magic Off - all memorized lists unchanged to the end; On - "PHILIPPE Casts a Spell" (frame), the
    Magic Missiles of PHILIPPE / LEDERA and SHARA's Hold Person gone from memory (memchk.py ... 75F4 PHILIPPE ...).
  PoolRad (ROOT=1, sub POOLRAD, "START"; the differ's fixed-seed START.EXE so the walk is the same each run):
    save A, 'up' at 14,4 W then 'right' and up/enter pairs: goblin guards ("COMBAT WAIT FLEE PARLAY") at the 13th
    up; 'c', then 30 'q' presses 1.5 s apart (TAKE TIME: monsters' pages eat keys). RHIANNON / DARKSTAR given 3
    Magic Missiles (CHRDATA3 / 5 .SAV 0x18 on). Poke 0E1D:6E30=1 for Magic On.
    Result: Off - lists unchanged; On - each magic-user's list one Magic Missile shorter.
rwalk.sh <short> <tries> <len>: random walks from save A in the playsim until a fight starts (finds the tavern).
The engine's check: behave rules4 ONLY=quickacts (qa_spells: ticked casts and the spell leaves memory, unticked /
switch off never, a party NPC always).
