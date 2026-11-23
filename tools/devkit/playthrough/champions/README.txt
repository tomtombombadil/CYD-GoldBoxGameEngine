Champions of Krynn play-through kit (2026-11-22): the Darkness kit (../darkness) with the play folder
/tmp/claude-0/chplay (card/CHAMPIONS = the GOG copy), playsim GAME=5; saves in CHAMPIONS/SAVE (bk.sh). The rest of
the Darkness kit is copied to /tmp/claude-0/chplay/bin with the paths changed (sed dplay -> chplay, $P/DARKNESS ->
$P/CHAMPIONS, GAMEN 4 -> 5). The party: GOG save A's six (Sir Strongsword, Istan, Garin, Molly, Kal, Sirrion) in a
new game (NEWGAME=1 SAVE=A), saved as B.
New here:
  lvl.sh <slot> <xp>      every member given xp (SETEXP) and trained by the Trainer cheat until none may
  buyn.sh <slot> "m:g ..."  one Merchant-cheat visit: Pool, member m buys goods g (GOODSLOG=1's numbers, 17 a page),
                          Share at the end
  drop.sh <slot> "m:Item,...;..."  named items dropped from camp
The overland (area word 0x4BE6 = 0): the menu is Move / Encamp; View is in the camp (rdy.sh RDYPRE=/E RDYPOST=/E);
"/M" then R / L (45 degrees) and F (a step). The outposts are menus in that mode ("WHERE DO YOU WISH TO GO?").
Snapshots: p.sh needs card/out; a long fight's snapshots fill the scratch allowance - keep it emptied.
  post.sh <slot>          after a fight: gear.txt's gear readied again (Quick readies the bows and leaves the swords
                          and shields unreadied - sellplain would sell them), plain loot sold, the Healer
  go2.sh <slot> <x,y>     goto.sh through locked / secret doors, each event on the way played by adv.sh (Y), post.sh
  tourc.sh <slot> x,y ... go2.sh to each square, a backup after each
  The Healer (fix.sh): the temple's own menu first ("/H" opens the list): Heal the 6th line, Raise Dead the 8th.
  sellplain.sh: "Ring of" kept, Ring Mail sold. adv.sh: a one-word "Press <enter>" menu pressed on.
The playsim's auto tokens: Champions' fight menu has no Quick - Done, then Auto (Guard Delay Quit Auto Speed Exit).
Lessons (2026-11-22): Quick readies darts / bows and leaves the sword and armour unreadied - rdy.sh no longer keeps
darts readied (they took the weapon hand), and sellplain.sh never sells a name in gear.txt (it had sold Sir
Strongsword's plate and sword, Molly's hoopak). The Merchant cheat's first goods page is 22 lines (buyn PS=22).
A lost fight ends at the party menu (screen 1 with the fight's menu still shown) - the story fights the party
can't win on Auto: EXTRA="QASPELLS=1 WINFIGHT=1" for that event only.
Champions' Hoopak (ITEMS type 67) has flags 0x10 alone - a thrown weapon: thrown in a fight it lands in the
treasure (combat_rules_facts: thrown weapons to the pile) and goes to whoever takes the loot - so the kender
carries a long sword and shield in gear.txt. The catacombs (script 34) are sections joined by script steps
at the map's edges (15,0 east -> 0,1): MAPREACH per section, then the edge step.
Overland: R / L / F / B step east / west / north / south. AUTOTAKE with %3 (Molly) - the selected member may be overloaded.
Champions' "WHAT DO YOU DO?" choices are often a LIST menu (wait 3, "LIST prompt=..." in the log; the bottom
line stays empty): adv.sh picks the first line starting with a PICK word, else LISTPICK (0). memset.txt: Curse's
spell ids work (01 Bless, 03 CLW, 0C Enlarge, 0F Magic Missile, 13 Shield, 15 Sleep, 17 Hold Person, 18 Resist
Fire, 22 Stinking Cloud, 2F Fireball) - without it the casters have nothing memorized and Auto only swings.
go2.sh: a story fight lost on Auto (the party menu) is played once more with WINFIGHT ("LOST on Auto").
sellplain.sh (2026-11-22): gear.txt guards one copy of each name for its own member (readied, or one left
unreadied by Quick) - every other copy is sold. Before, any name in gear.txt was never sold, so the third outpost's
guards' plate and swords filled every pack and Jadefang's 16 items (the Mace of Disruption) were left behind.
Champions' last levels are its own experience rows (-1 past them): ranger 7, cleric 8, magic-user 8 - training
stops there with "Not Enough Experience", not a race limit.
The overland's places (script 16, tables at 8E72 / 8E7E, 4BC3 / 4BC4 the marker): Throtl 2,1, outpost 1 1,3,
outpost 2 13,5, Gargath 14,3, Jelek 12,7, Neraka 17,8, the tomb 11,3, the Ogre Base 16,3, Sanction 10,13, Kernen
31,3, outpost 3 15,11. Overland steps: F north, B south, R north-east, L north-west.
Lessons (2026-11-23): a treasure's money (gems, jewelry, coins) was never taken - adv.sh now passes the playsim's
AUTOMONEY=1 (Take - Money, each coin line the most there is) before AUTOTAKE's items. walkf.sh (from ../darkness)
walked on and saved where a script's treasure screen was starting (screen 24 under the exploring words) - Huerzyd's
statue treasure was lost; it stops there now, and Knocks a wizard-locked door when someone has Knock memorized
(memset.txt: 1F for Garin and Sirrion - Duerghast's locked rooms). sellplain.sh sold the Footman's Dragonlance (a
plain-looking name): ITEMDUMP=2 shows each item's plus, and an item with a plus is never sold. The playsim's Auto
rule took Champions' Done menu's Quit (ends the turn) for Quick after the Treasures kit's change - fights went
nowhere and were "LOST on Auto"; a Delay word tells that menu apart. Hidden treasure (Huerzyd's bedroom, the book
room) needs Look on the square (script 0x7ECA = 1). Huerzyd's tunnels: each passage's west end steps on to the next
section (0,15 W -> 9,14; 0,14 W; 0,13 W -> 7,10); the Ancient Revered One at 4,10, out at 0,10 W into Duerghast's
basement (15,0).
