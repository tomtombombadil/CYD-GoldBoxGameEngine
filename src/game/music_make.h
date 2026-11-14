// The games' music turned into sound files on the card, once, by the card
// scan (Tom, 2026-11-14): each song of a game's MUSIC.DAX (MicroMagic's -
// Pools of Darkness, Death Knights of Krynn, Gateway and Treasures of the
// Savage Frontier) rendered by engine/music + engine/opl to
// <cache_dir>/MUSIC<n>.WAV (8-bit mono, music::kRate Hz, the trailing silence
// cut). Play then streams the file (play.cpp). Made from the player's own
// files on the card; nothing shipped.
#pragma once

#include <cstddef>
#include <cstdint>

namespace musicfiles {

// The songs the game in data_dir has (bit n = song n); 0: no music the
// engine plays
uint32_t songs(const char* data_dir);

// <cache_dir>/MUSIC<n>.WAV
void path(const char* cache_dir, int song, char* out, size_t cap);

// Every song's file there (a start whose card was scanned by an older
// version makes the missing ones - as the pictures)
bool ready(const char* data_dir, const char* cache_dir);

// (replace: the line takes the place of the one before - "Rendering ..." ->
// "Rendered ...", as the scan's list does)
using Say = void (*)(const char* line, bool replace, void* user);

// Makes the songs' files (force: all again; else the missing ones), saying
// a line for each; the number that failed
int make(const char* data_dir, const char* cache_dir, const char* game_name, bool force, Say say, void* user);

}  // namespace musicfiles
