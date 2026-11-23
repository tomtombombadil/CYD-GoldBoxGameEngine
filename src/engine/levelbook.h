// The level book (Tom, 2026-11-23: the AD&D 1E Level Drain & Restoration
// rule, rules::kAdndDrain): what the games' saves don't keep and the 1st
// edition's drain and Restoration need - the hit points each level gave, and
// each level drained (its class, the level it was, those hit points) until
// a Restoration gives it back. Kept by the character's name in a text file
// of the engine's own beside the game (_CYD/<folder>/LEVELS.TXT), never in
// the games' files - the originals still load every save. A line a fact:
//   H <tab> name <tab> class <tab> level <tab> hit points
//   D <tab> name <tab> class <tab> level <tab> hit points <tab> former (0/1)
// Read and written whole at the moment it's needed (a level trained, a
// level drained, a Restoration) - nothing held in memory. No file (a game
// without one set, the host tests): every answer "not known", the callers'
// fallbacks then.
#pragma once
#include <cstddef>
#include <cstdint>

namespace levelbook {

// The file's reading and writing (the game layer's - the engine itself
// touches no files): load the whole text (malloc'd, 0-ended, its length in
// *n; nullptr: none), save it whole, add a line. Store null: no book
struct Store {
    char* (*load)(size_t* n);
    void (*save)(const char* text, size_t n);
    void (*append)(const char* line, size_t n);
};
void set_store(const Store* s);
bool have_file();

// The hit points `level` of class `cls` gave the character called `name`;
// -1 when not written down
int  hp(const char* name, int cls, int level);
void set_hp(const char* name, int cls, int level, int hp);

struct Drain {
    uint8_t cls = 0, level = 0;         // the class and the level it was before the drain
    uint8_t hp = 0;                      // the hit points taken with it
    bool    former = false;              // a dual-classed human's former class
};
void push_drain(const char* name, const Drain& d);
// The latest drain of `name` that `valid` accepts, taken off the book; false: none
bool pop_drain(const char* name, bool (*valid)(const Drain&, void*), void* user, Drain* out);
// Forget every drain of `name` (its levels given back another way)
void clear_drains(const char* name);

} // namespace levelbook
