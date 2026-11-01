// The Gold Box Companion panel's Party and Battle views (Tom, 2026-11-01;
// CYD_COMPANION_VIEWS, app/features.h): the party - icon, name, hit points
// and a bar, status / effects - and a fight's whole battlefield - walls
// grey, the party and allies green, the enemies red, those asleep / held /
// fallen blinking, the game screen's 7 x 7 window outlined. Drawn straight
// to the screen through a Painter (the board: LovyanGFX; the playsim: an
// RGB picture) - no memory kept, nothing but small locals. Plain C++: what
// it shows comes from play::panel_* (game/play.h).
#pragma once

#include <cstdint>

namespace companion {

struct Painter {
    virtual ~Painter() = default;
    virtual void fill(int x, int y, int w, int h, uint16_t rgb565) = 0;
    // 6 x 8 cells (LovyanGFX's Font0), the background left as it is
    virtual void text(int x, int y, const char* s, uint16_t rgb565) = 0;
    // A game screen value's colour as the screen shows it now
    virtual uint16_t colour(int value) = 0;
};

struct Area { int x, y, w, h; };

constexpr int kCharW = 6, kLineH = 9;

// The views' names along the top: Map Party (Battle in a fight), the one shown lit
enum class View : uint8_t { Map, Party, Battle };
void draw_tabs(Painter& p, int x, int y, View v, bool fight);

// The Party view: all = the area cleared and the icons drawn (else only
// the rows' text and bars - hit points or effects changed)
void draw_party(Painter& p, const Area& a, bool all);

// The Battle view: the field fitted into the area (as large as fits, the
// part of the 50 x 25 squares the fight uses), the lines under it.
// blink_on: the blinking dots shown. big: the 320x240 Map key's page
// (the whole game screen: a heading line "tap to go back")
void draw_battle(Painter& p, const Area& a, bool blink_on, bool big = false);
// The blinking dots again (after draw_battle in the same area)
void blink_battle(Painter& p, bool blink_on);
// Whether any dot blinks
bool battle_blinks();
// The pixels a square took in the last draw_battle (for the report / tests)
int battle_cell();

} // namespace companion
