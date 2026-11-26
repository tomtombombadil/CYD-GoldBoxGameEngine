Death Knights of Krynn play-through kit (2026-11-23): the Champions kit (../champions) moved over -
play folder /tmp/claude-0/knplay (card/KNIGHTS = the GOG copy, card/CHAMPIONS = the Champions
winners' save for "Load from where? Champions"), playsim GAME=6, saves SAVGAMx.PTY. No game bytes here.

New here:
- evlist.py <dis file>: each "op 25: 1:7F79 ..." dispatch (the square's event number) - the event's
  address and its first texts. The square -> event number map: the playsim's MAPFLAGS=1.
- goto.sh: never saves while text waits for a press at the Move menu (wait 13 / 1) - a save there skips
  the rest of the event (the outpost's temple, 2026-11-23).
- adv.sh / goto.sh add the saved runs' TEXT / JOURNAL / [fight] lines to story.log.

Lessons (Gargath outpost):
- The keep's upper story is the SAME map with 4C1B = 1 (the spiral stairs at 8,7 toggle it). The
  commander is 11,8 upstairs; downstairs 11,8 is the well. Walk upstairs with PRE="FF/Y" or go2 to 8,7.
- The outpost's story: commander (Yes, Cemetery) -> temple 6,9 (the burial) -> commander (Patrol) ->
  Ayn on the wall squares (event 0E: every second step there counts, the 4th meeting) -> Patrol ->
  relief squares (events 13 / 14: 9,3 then 8,2; a fight on the way) -> commander (the foes have left,
  journal 8) -> Kalaman.
- Check the story's flags with PEEK= (the save's values) / PEEKEND= (after the tokens).
- The outpost's exit: the west gate (3,7 / 3,8 facing west - the step script jumps two squares through wall
  types 5 / 7), then off the map's edge west; answer No to the overland's "enter?" that follows.
- kovpath.py <x,y>: the overland walk (block 17's code grid in ov17.txt, read by PEEK at 9965 + 38 y; keys F N,
  R NE, > E, B S, < W, L NW; 0x14 mountains and 0x19 water refused). Kalaman 25,1.
- Kalaman's bazaar stalls ask "cut through?" - Yes walks through (a vandals line), so walks there answer by the
  question's text (Yes to cut through, No to peddlers / bars / waiting). The Dream Pavilion is event 0x16 (2,3).
- walkans.sh <slot> <x,y> [PRE]: a walk answering each question by its text (NOBASE / NOANS: the No patterns),
  then adv.sh. kovgo.sh <slot> <x,y> [PRE]: the overland walk with encounters fought (Quick, casters' spells).
- lvl.sh: Death Knights asks "do you wish to train?" in lower case (the grep is case-blind now).
- Kalaman: the commander's company (15,3) - shut at night; the bazaar's ambush (stall squares by day = the centre
  event 0xD: "wait here?") was lost on Quick at 6th-9th level - trained to 10th first (lvl.sh 600000), won with
  QASPELLS. Rest in town: "go to an inn!" - the Sanctuary cheat ('"2/R/H' + '/A' x hours + '/R' + '_' waits + '/E').
- Vingaard (block 33): the inn is entered across the map's west edge (0,2 west -> 15,2); the dream merchant's
  shop event 0x22 (8,14); Sir Thom's dream comes by resting in an inn room (12,0) - the pre-camp sets the rest
  words 1 / 100 (an encounter at once).
- Vingaard's red-door trail (after the dream, 0x4C53 = 3): step south from 4,9 (0x4C0F = 1), then only through
  red doors (types the table at 9D6B marks 1) and open sides - a door marked 2 ends it ("whispers fade away").
  redpath.py <slot> <x,y,facing> <goal> plans it, drive.sh walks it a step at a time (presses, Leave, No); the
  door is revealed at 12,11 - answer Talk, then Good.

Lessons (Dragon Pit, Vingaard's second visit, 2026-11-23):
- evsq.py [codes]: each event code's squares from MAPFLAGS=1 (run p.sh with it first).
- The Dragon Pit (overland 5,2): the rope bridge 12,4 refused at first (Maya flies over) - Sebas's door 10,2, the wights,
  the wolf, the altar's treasure, Spark the chained red dragon 2,9 (PICK=Free), south and back east, the barracks,
  Sir Karl (4,12 then 3,10: the wolf's 13-monster fight), the courtyard 6,8 (Karl's lance takes Maya), the lair 11,8,
  the bridge 12,6; out by 15,8 east ("leave the cave?" Yes, then the overland's "enter?" No: drive.sh YES=CONTINUE).
- A long fight on auto can end the playsim's AUTOTOK run before it is over (it reads as "stuck at the fight menu"):
  give another SEED= - not an engine fault (2026-11-23, the wolf's 13 monsters).
- Vingaard again: 0x4C57 2 after Sebas's talk; the trail needs exactly table[0x4C57] red doors (9D60: 9 9 10 10) and the
  reveal event 0x23 is at 12,11 (first visit, 9 doors) AND 11,15 (second, 10 doors) - bfsn.py COUNT=10 to 11,15.
  The original (DOSBox, the engine's save) counts the same 9 at 12,11. Then the hideout 12,14: the sivak ambush, and
  again for Ariela's earring (0x4C57 5) -> Kalaman.
- Never walk through 2,5 (a temple: the walker loops) - bfsn.py BLOCK="2,5".

Lessons (Dargaard Keep, Dulcimer, Voice Wood, 2026-11-24):
- The "crypt" checkpoints (kn_crypt*) are Dargaard Keep's first floor (area 3 block 96), entered by the graveyard
  tunnel. Its gate: 7,15 south ("leave Dargaard Keep?"); the stairs up 14,0 need the Rod (block 97, then 98).
- Dulcimer (overland 31,5, block 81): the lich's guard room 9,0, the lich 5,0, the phylactery under the asparagus
  1,13 (Yes to DIG); Voice Wood through the east gate 15,8 (block 82): the offering 4,8 (0x4C01), the sprites'
  chamber MOVE (PICK=Move), the Voice 12,7 (the Rod: 0x4CB8 = 1); out by 0,8 west into Dulcimer, its exit 7,15 S.
- Keep's second floor (block 97): sneak past the vampires (PICK="Sneak,Attack"); the portcullis stages 0x4CCC
  (1 -> lever 1,1 -> 2, flesh golems 3,5 -> 3, lever 3,3 -> 4, cell guards 5,9 -> 5); the evocation 14,2, the tome
  13,9, Sebas 8,7, the cleric's cache 7,3 (one time only), Sturm 14,13; up by 8,14 (NOANS="GO DOWN" - walkans
  answers the third floor's "go down?" Yes otherwise).
- Third floor (block 98): Lenore 1,4; the spiked doors (6,7 / 7,7 north): Open door (WHO = a strong member) - the
  secret passage was not found; the statues 10,11 (Ignore -> they join Durfey's fight 11,11 - the script's 0x4C02);
  Soth 12,6.
- "take refused" for everyone: the party's coins are the weight. At the treasure: Pool, take the items, then Share
  (it hands out only what each can carry) - takeall.py doesn't do it.
- An adv / walkans run that ends at a fight menu with "screen 1" and "0 characters" is a party wiped out (the auto
  driver stops at the party menu), not a long fight: another SEED or a stronger party.
- A save's experience: read the CHRDAT file (offset 0x62) or ITEMDUMP after a load into a real slot (A-J).

Lessons (Cekos, Turef, the beached ship, 2026-11-26):
- place.sh "X,Y" TAG [PICK]: the overland trip, the place's events listed (ev_TAG.txt, TAG.sq) and every event square
  walked (TAG.out); place3.sh TAG [PICK] the walk alone (TAG.sq made by hand: events.sh, MAPFLAGS=1, evsq.py);
  leave.sh "X,Y": out by the entry square onto the overland, saved there. A place that asks something at the door
  (Turef's "Help Skomp", the ship's "Ladder Rope") stops kovgo.sh: enter by adv.sh with the answer, then place3.
- Read the events before walking a puzzle place: walkans answers every Yes/No and the first PICK word of a menu, and
  a menu that loops (Turef's cook: Eat Stew) eats the run. NOANS="..." for the questions to answer No.
- Typed answers: the token "=88;" types 88 at a number question (TYPE= is only the AUTO driver's).
- MAPAVOID="x,y;x,y" exported reaches every playsim run (walkans passes the environment).
- Cekos (overland 17,12, block 34): the fist fight 11,5 (Sneak = a fight), the sivaks' table 11,1 (the silver key,
  0x4C5F 8) opens the wall west of 5,7; the stairs 4,7 (Go down: 0x4C5A) to 15,0 below; the five blue dragons 14,3
  (Kal / Sirrion are elves: the temple can't raise them - replay with another SEED); the old woman 7,13; then the
  town's freed table (4C5C): the copper man 7,5 (Give), the statue 8,8 at night (Wait: a scroll). The prisoners 2,8
  only before the dragons.
- Turef (overland 22,13, block 67): Help Skomp; barracks 7,1 Shield (the book); scholar 12,1; the bar 7,10 (Order a
  drink, then Inquire: key 88; Wander stays on the square); Fromm 7,12 ("/N" to selling keys, Yes, "=88;", No); the
  trapdoor 6,9 from 7,9 (west) with "=88;" -> 5,9; round by the north to 7,8, then east into 8,8: behind the ambush;
  the dwarf 2,1 Wrestle, his mother 9,11; Skomp's boar 9,4 only from 8,4 (east); the cook 13,4; the relics 6,5.
- Fix.sh stops at a dead elf ("cast cure anyway?"): the members after it are not healed.
- adv.sh / kovgo.sh answer EVERY Yes/No with Y: on the way to the strange house they took the faeries' "save the
  Father of Trees?" and then "do you want to leave the tree?" - Yes, the forest lost for good (0x4C3D 0x10, 0x4C44
  0). Overland legs past a place with a one-time question: by hand (p.sh) or NOANS. The tree (overland 17,13, block
  83 with 0x4C3A 1): in at 12,7 ("/Y./N"), wights 14,7, the pale figures 14,10, the tentacles 12,13, the mage 8,14
  (the wand, 0x4C44 1), the spider 9,11; out at 15,15 (gratitude, the faeries' thanks). The same block holds the
  ship (0x4C3A 0: deck, the hold by the ladder 4,2 / up at 0,2, out at 7,5), the village (3: shopkeeper 14,12, the
  sorcerer 14,10, the herbalist 12,13, the impostor 8,14, the meteorite 9,11; out off the edge at 15,15 south) and
  Gluten's maze (2: riddles at 7,12 "=WATER;", "=HANDS;", "=RING;", the villain 1,12 - MAPAVOID 4,2;0,8 - the prize
  1,14; out by 6,15 "RRF/N").
