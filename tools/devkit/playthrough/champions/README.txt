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
