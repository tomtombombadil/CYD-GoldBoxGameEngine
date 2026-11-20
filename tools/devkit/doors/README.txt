Locked doors, every game (2026-11-20: Secret's profile had no door words, so its locked doors stopped
the party without a menu). doorscan.cpp (g++ -std=c++17 -Isrc -Itools/host doorscan.cpp src/engine/dax.cpp
src/engine/geo.cpp) counts each GEO block's locked (passage 2) / unpickable (3) sides; doorwords.py finds
the "Bash" " Pick" " Knock" " Exit" "Locked. " Pascal strings in each game's GAME.OVR / program under
/tmp/claude-0/games. A game whose maps have locked sides AND whose program has the words needs
profile .door = those offsets. 2026-11-20: Curse, PoolRad, Champions, Gateway, Secret have both (all set
now); Darkness has 24 locked sides but no words in its program (its doors stay shut, as the original's
no-word menu); Knights / Treasures no locked sides; Queen's maps are HLIB (not scanned).
