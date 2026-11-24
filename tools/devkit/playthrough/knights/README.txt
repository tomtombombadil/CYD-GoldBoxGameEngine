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
