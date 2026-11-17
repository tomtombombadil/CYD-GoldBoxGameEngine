// Our own table of the games' places (Tom, 2026-11-17: "our own table of
// game areas (maps) to their common names (used both by guides and by the
// game itself)"): the name a player knows a script block by - "Slums",
// "Pit of Moander" - for Save Slot Info (play.cpp peek_show). Written by
// us, a short name a block; nothing taken from the games' files.
#pragma once

#include <cstdint>

#include "games.h"

namespace places {

// The place for the game's ECL area (file ECL<area>, or 1 where a game has
// one) and script block; nullptr when the table has none
const char* name(games::Game g, int area, int block);

} // namespace places
