Treasures of the Savage Frontier - the play-through kit (the PortMaster session, 2026-11).
Scratch layout: /tmp/claude-0/tplay (card/TREASURE = a copy of the game folder, bak/ = save backups,
transcript.txt = every text of every leg). The playsim binary: tools/devkit/build.sh splay
"-DCYD_TEST_HOOKS" tools/playsim/sim.cpp. Nothing from the game is kept here - scripts only.

p.sh       tokens from a save (SAVE=J): the state, last.png
auto.sh    tokens + the playsim's own continue / Quick fight (AUTOTOK)
go.sh      walk to x,y in a 3D map (MAPPATH a step at a time), events by AUTOPICK, fights fixed after
tourx.sh   "x,y x,y ...": go.sh to each from slot J, saved after each, transcript kept; loot on
           (AUTOLOOT=1 AUTOTAKE=/S/T/I). Never let it stop on a script's menu: a save there drops the
           event's rest (Leilon's guards were lost that way) - give AUTOPICK every word the place asks.
ovwalk.sh  the overland on foot (terrain flags, low nibble; A = a river: boat). Saves + fix.sh after
           each fight (Quick fights there kill the casters otherwise), listens to dwarves, stops at Enter?
ovgo.py    its planner.  ev.sh: a script's events, squares and first texts (disev.txt = the disassembly).
fix.sh     Healer cheat + rest + memorize.  bk.sh save|load <slot> <name>.  tlevel.sh, trade.sh, rdy.sh,
           shop.py, sell2.py: levels, items.
Lessons: the overland is windows of one world (script 51, maps 51-57) - the town table is at the script's
9B3A / 9B50 / 9B66 (map, x, y), names by the ON GOSUB at 8D8B; edge exits need a step INTO the edge;
"Bash Pick Dispell" doors held by magic open only to Dispell; a vertical menu after text needs the text's
key first ("./L...").
