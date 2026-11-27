// The Dark Queen of Krynn's import of a Death Knights character: the numbers
// its program converts on the way (DQK.EXE 0x3497B / 0x34858 - an item's name
// words and spells, an effect's number) by three C switches of its own, RUN
// here from the player's DQK.EXE in engine/x86 - nothing of the game in this
// code: the window of program bytes comes from the card (profile::QImport).
#pragma once

#include <cstddef>
#include <cstdint>

#include "engine/party.h"
#include "engine/profile.h"

namespace qimport {

struct Maps {
    uint8_t word[256];          // an item's name word
    uint8_t effect[256];        // an effect's number (also an item's 0x3D with 0x3E = 0x80)
    uint8_t spell[128];         // a spell number (7 bits)
};

// The three switches run for every value, from the program bytes window[0 ..
// q.hi - q.lo) (file offsets q.lo..q.hi). False: a run that didn't come back.
bool make(const uint8_t* window, size_t n, const profile::QImport& q, Maps& m);

// A Death Knights character as the original's import leaves it: its items'
// words, spells and specials, their type 0x34 -> 0x33, its effects' numbers
void convert(party::Character& c, const Maps& m);

}  // namespace qimport
