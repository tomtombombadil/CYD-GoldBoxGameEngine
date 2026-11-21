Secret of the Silver Blades play-through kit (2026-11-20): the Curse kit (../curse) adapted -
play folder /tmp/claude-0/splay (card/SECRET = the GOG copy, card/CURSE = the Curse party's .GUY
files for Add Character), playsim GAME=3, saves in the game folder's root (SAVGAMx.DAT).
New here: events.sh (the script's square events from its ON GOTO table, by PEEK), loot.py IDALL=1
(Secret's shops sell little, so unknown names are identified too), gear.txt / memset.txt (the party's
readied gear and memorized spells - fight.sh / p.sh use them). Camp casting: "%m/E/M/C" + v x the
spell's place in the sorted list + "/C..." + ^/v to the target + "/S..". No game bytes here.
playsim QASPELLS=1 (with RUNAT): the Quick tab's Spells ticked for the party (Magic On), so a
FIGHT=1 fight's casters cast. A block's entry script runs on AREA=/BLOCK= (Sir Deric's meeting is
block 68's entry) - start such events by loading the block, not by RUNAT.
