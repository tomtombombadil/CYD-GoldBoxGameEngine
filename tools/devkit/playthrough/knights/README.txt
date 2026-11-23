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
