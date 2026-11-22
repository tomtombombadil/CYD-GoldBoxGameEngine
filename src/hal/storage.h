// LittleFS on the internal flash partition. Shared by every module that
// saves something (touch calibration, display fixes, games, settings).
#pragma once

// Mounts LittleFS on first call (formatting a blank partition). Safe to call
// repeatedly. Returns false if the filesystem is unavailable; callers should
// then carry on with defaults rather than fail.
bool storage_begin();

// Make sure a top-level directory exists (e.g. "/games"). True if it does.
bool storage_mkdir(const char* dir);

#include <FS.h>

// The engine's own small files (settings, display fixes, touch calibration):
// on the internal flash - except on a board whose display can't stand a flash
// write (BOARD_STORAGE_ON_CARD: the 5.0" RGB panel reads its frame from the
// PSRAM, which a flash write cuts off - the picture jumped at every Settings
// key, Tom 2026-11-22), there on the card's /GOLDBOX/_CYD, a copy in the
// flash still read until the card has one. Mode "r" or "w".
fs::File storage_open(const char* path, const char* mode);
bool storage_exists(const char* path);
