// Quick Actions (Tom, 2026-11-09; app/features.h CYD_QUICK_ACTIONS): what
// each of the player's own characters may do when Quick plays it in a
// fight - eight switches a character, kept by the character's NAME in one
// small text file on the card (_CYD/QUICK.TXT), so they follow the
// character across save slots and into the later games.
//
// The file: a comment line or two ('#'), then a line a character - the
// eight switches as '1' (ticked) / '0' in the columns' order, a space, the
// name to the line's end ("11101111 ALIAS THE BOLD"). The first comment
// line starts "# Quick Actions 2:"; a first version's file (v0.184.0,
// "# Quick Actions:", its Spl doing nothing) is read with Spl unticked and
// rewritten so at the next set. A name not in the
// file has kDefault: everything ticked but Bandage First and Spells (each
// game's own Quick: its Magic Off - below). Names are compared without case
// and without trailing spaces.
//
// Spells (2026-11-09, checked in DOSBox and in every game's code): the
// originals' Quick casts a party member's memorized spells only while the
// fight's "Magic On" is set - Alt-M in the fight menu or during Quick
// play toggles it ("Magic On" / "Magic Off"), every fight starts with it
// off (Curse GAME.OVR 0xF3C8 after "A battle begins..."), and the AI's
// spell step tests it beside the control byte (Curse 0xC0C8: control over
// 0x7F - NPCs, monsters - or the flag). The Spells column is that switch,
// a character at a time: ticked = Magic On for them.
//
// Plain C++ (no Arduino), host-tested in tools/host_tests/test_dax.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

namespace quickacts {

// The columns (bit k = column k; a set bit = ticked)
enum : uint8_t {
    kMelee = 1,         // melee attacks
    kMissile = 2,       // missile / thrown attacks
    kSpells = 4,        // casting any spell
    kItems = 8,         // potions, wands, other magic items
    kBandage = 16,      // Bandage First: a dying friend bandaged at the turn's start
    kMove = 32,         // steps toward enemies
    kTurn = 64,         // Turn Undead
    kGuard = 128,       // guards when it has nothing else to do
    kAll = 0xFF,
    // The computer's own fighters (monsters, NPCs): the games' AI unchanged
    // (a dying friend bandaged in its own order, spells always)
    kComputer = kAll & ~kBandage,
    // The start for the player's own characters: each game's own Quick -
    // Bandage First unticked (the game's own order) and Spells unticked
    // (the games' Magic Off, as every fight starts)
    kDefault = kComputer & ~kSpells,
};
constexpr int kCols = 8;
constexpr int kNameMax = 31;                // (a name's characters kept; the games' are 15 at most)
constexpr size_t kFileMax = 8192;           // (a longer file is read up to here)

// Each column's short heading ("Mel" ...) and its two lines of help (38
// characters at most each: the game screen's text width)
extern const char* const kHead[kCols];
extern const char* const kHelp[kCols][2];

// The switches the file text gives `name` (kDefault: not there)
uint8_t find(const char* text, size_t n, const char* name);
// The file text with `name`'s line set to `mask` (replaced, or added at the
// end; the comment line put first when the text has none), into out; its
// length, or 0 when out is too small
size_t set(const char* text, size_t n, const char* name, uint8_t mask, char* out, size_t cap);
// A name as the file keeps it: trailing spaces off, control characters as
// spaces, kNameMax characters at most (out: kNameMax + 1)
void clean_name(const char* name, char* out);

} // namespace quickacts
