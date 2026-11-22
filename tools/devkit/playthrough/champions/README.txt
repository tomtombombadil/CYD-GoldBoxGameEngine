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
