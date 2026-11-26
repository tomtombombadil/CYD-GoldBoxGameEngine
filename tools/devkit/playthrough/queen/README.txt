The Dark Queen of Krynn play-through kit (2026-11-25): the Death Knights kit (../knights) moved over -
play folder /tmp/claude-0/qplay (card/The Dark Queen of Krynn = the GOG copy, card/KNIGHTS = the Death
Knights winners' save, slot C, for "Load from where? Death Knights" - POOLDIR=/GOLDBOX/KNIGHTS/SAVE),
playsim GAME=7, saves SAVE/SAVGAMx.QSV (+ VAULTx.DAT). No game bytes here.

Changed from the Knights kit:
- p.sh: GAME 7, the folder name. bk.sh: the QSV and VAULT files.
- adv.sh: also saves at the Queen's exploring menu ("Area Cast View Encamp Search Look Inv").
- events.sh / evsq.py: the Queen's square dispatch ("op 2F: 1:002D 0:007F 1:00BF" then "op 25: 1:00BF 0:N");
  maps up to 17 x 17 (any width).
- walkans.sh: MAPLOCKED / MAPSECRET only with WLOCKED=1 / WSECRET=1 - with them on a walk went through
  Caergoth's warehouse door 3 (7,13) and the engine couldn't leave it. MAPAVOID="7,13" keeps it out.
- place3.sh: the slot from SLOT (default B).

Lessons (Caergoth):
- The dragons' fight (two blue, one green) wipes a Knights-level party on most seeds: SEED=19 won with no
  deaths (17 lost one). Auto fights that lose stop at the party menu (screen 1).
- Daenor's ship 8,14: Explain, then Yes to join - NOBASE="BUY\|DRINK" so the JOIN question is answered Yes.
- The temple list has no Resurrection: dead elves can't be raised there.
- To check a save in the original: tools/devkit/quickmagic/dosrun.py with OVERLAY=<qsv>=SAVE/SAVGAMD.QSV and a
  script Enter, P, L, T, D (load D from the party menu); the party list must show.

Lessons (the cave, 2026-11-26):
- The cave (script 10) is 28 x 28: until v0.217.3 every fight past column / row 15 was walls but the party's row
  (the umber hulks never came - "won" with 0 experience). The playsim's MAPPATH takes any map size since then.
- One-way sides: MAPWALLS draws each square's own north / west side; the engine's "open N E S W" line is the truth.
- Its chain: the purple worms (12,9 lair), the decoy spider (14,2), Sensilan at 16,7 (journal 39), the umber hulks,
  Crysia at 20,22 (journal 57), the villagers at 0,19 after the Auraks' fights (journal 47 - 18,17 is the
  beetles), Crysia and the blue dragons at 23,7, the exit 24,0 -> the Silver Shark, the storm, script 14.
- The Auraks' fight at 5,15 (a dark wizard's fireballs first) wiped the party on five seeds: WINFIGHT=1 (since
  2026-11-26 it acts while the fight ticks too - before, the foes' first turns came before it).
- fix.sh: the Queen's Healer list - Heal 6th, Raise Dead 8th, Stone to Flesh 10th (a beholder's stoning). Elves
  can't be raised there (no Resurrection): avoid their deaths (WINFIGHT).
- Script 14 (Naulidis) dispatches its squares by an ON GOTO on 1:009B with the addresses inline: evinline.py
  lists them; tour.sh walks a list of squares (STOPRE ends it at a text).
- Naulidis (20 x 39): the party arrives at 11,17 in the refugees' halls (before v0.219.2 a 4-bit mask put it at
  11,1 inside the palace). The safe halls' barred doors (4,9 / 6,17) open to SHOUT (PICK=Shout - Bash loops);
  the hinge for the cracked door is in the workshop at 17,25; the portals (14-16,7) lead up into the palace.
  After the hinge: walk the safe hall's refugee squares in ONE playsim run (a talk is a 1 in 7 at a step - each
  run of a walk restarts from the save; p.sh now gives each run its own dice); answer the talks (spears: Yes,
  then No to more). Then the families: the event 9 rooms (12,15 15,11 15,20 ...) and back to a safe hall,
  avoiding the dragons' common room at 13,20 / 14,20 (a fight there leaves them behind). Then more hall
  squares: Lyzian's scene (byte 0x123 = 1); the royal bedroom 5,5 with Search on (a step out and back in):
  Daenor (0x123 = 2, Carry Him); a safe hall: the crylokon (0x123 = 3, 0x6F = 2); the upshaft 8,28 / 9,28
  with the crylokon commanded (Yes) - no suffocation; the air rooms of Celanost (0x123 = 4).
  Never SAVENOW inside a talk (script 15 / 16): the save resumes mid-talk with no map events.

Hizden (script 18, map 18) and the overland (script 5):
- The tavern's Talk (PICK=Talk) gives the lead; the door at 8,6 east asks who sent the party - list line 1
  (@1), then Yes: Ezra (journal 37, 0x12D = 1); a walk after it brings the raid (the fight at ~7,11,
  0x12E = 1); the exit is a step SOUTH off 7,15 (Yes).
- The overland's map: playsim CODEDUMP=A97D,1140 (30 rows of 38, x = the byte's place in its row, 0F the left
  border; high nibble F a place - F0 Hizden at 3,4, F1 the Luminari lighthouse at 4,1 ...; E a city; D
  blocked). Steps: F north, B south, < west, > east, L north-west, R north-east (play::move_keys()).
- The lighthouse: R F F from Hizden's square, Yes (script 22, map 22 24 x 27; its floors script 23).

