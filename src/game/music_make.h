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
#include <cstdio>

#include "engine/games.h"
#include "engine/music.h"
#include "platform/file.h"

namespace musicfiles {

// (songs() and path() are here, inline, as play.cpp uses them: the
// PortMaster port builds src/engine + play.cpp only; make() / ready() are
// the scan's, in music_make.cpp)

// The songs the game in data_dir has (bit n = song n); 0: no music the
// engine plays
inline uint32_t songs(const char* data_dir) {
    char p[200];
    games::path_of(data_dir, "MUSIC.DAX", p, sizeof p);
    plat::File f = plat::open(p, "r");
    if (!f) return 0;
    plat::FileSource src(f);
    const uint32_t m = music::micromagic_songs(src);
    f.close();
    return m;
}

// <cache_dir>/MUSIC<n>.WAV
inline void path(const char* cache_dir, int song, char* out, size_t cap) {
    snprintf(out, cap, "%s/MUSIC%d.WAV", cache_dir, song);
}

// That file is there and of the current kind (16-bit 22 kHz: a card
// rendered by v0.197.0 has 8-bit 11 kHz ones, made again)
inline bool have(const char* path_) {
    plat::File f = plat::open(path_, "r");
    uint8_t h[music::kWavHeader];
    const bool ok = f && f.read(h, sizeof h) == static_cast<int>(sizeof h) && music::wav_ours(h);
    f.close();
    return ok;
}

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
