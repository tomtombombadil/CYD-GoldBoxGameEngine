#include "settings.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <cstddef>
#include <cstring>

#include "hal/storage.h"

// A board whose usual way up is the engine's Rotate 180 (Tom, 2026-10-31:
// the 5.0" board, its USB port the other way round otherwise) - the
// setting's start before the player sets it (platformio.ini)
#ifndef BOARD_FLIPPED_DEFAULT
#define BOARD_FLIPPED_DEFAULT 0
#endif

namespace {

constexpr const char* kPath  = "/settings.bin";
constexpr uint32_t    kMagic1 = 0x53455431;  // "SET1": 4 + 15 bytes + 1 spare (to v0.208.x)
constexpr uint32_t    kMagic2 = 0x53455432;  // "SET2": 4 + 64 bytes - the Settings, then 0s to grow into
constexpr size_t      kSize1 = 15, kSize2 = 64;

static_assert(sizeof(Settings) <= kSize2, "the settings file's room");
static_assert(offsetof(Settings, rules_off) == kSize1 - 1, "SET1's fields first, in their places");

} // namespace

Settings settings_load()
{
    Settings s;
    s.flipped = BOARD_FLIPPED_DEFAULT;
    if (!storage_begin() || !LittleFS.exists(kPath)) return s;
    File f = LittleFS.open(kPath, "r");
    uint8_t d[4 + kSize2] = {};
    const int n = f ? f.read(d, sizeof d) : 0;
    f.close();
    uint32_t magic = 0;
    memcpy(&magic, d, 4);
    // (SET1: its 15 bytes are the Settings' first fields - the rest keeps its defaults)
    if (magic == kMagic1 && n >= static_cast<int>(4 + kSize1)) {
        memcpy(&s, d + 4, kSize1);
        // (SET1's one Game Fixes switch off: every fix off now - each its own since v0.209.0)
        if (s.comforts_off & kOffItemFixes) settings_set_fixes_off(s.fixes_off, ~0ull);
        // (the bits of switches gone since: Tap to Stop Demo, Journal Pop-Up -
        // their places are Coin Exchange's and Flasks of Oil's now, both on)
        s.comforts_off &= static_cast<uint8_t>(~(kOffItemFixes | kOffDemoTap | kOffJournal));
    }
    else if (magic == kMagic2 && n >= static_cast<int>(4 + sizeof s)) memcpy(&s, d + 4, sizeof s);
    if (s.brightness < kMinBrightness) s.brightness = kMinBrightness;
    if (s.sound == kSoundOff) {                 // (older files' Off: Mute since 2026-10-24)
        s.sound = kSoundTandy;
        s.volume_muted = 1;
    }
    if (s.sound == kSoundUnset || s.sound > kSoundTreasures) s.sound = kSoundTandy;     // (Tom: Tandy unless chosen otherwise)
    if (s.volume < kMinVolume) s.volume = kDefaultVolume;
    if (s.volume_muted > 1) s.volume_muted = 1;
    if (s.left_hand > 1) s.left_hand = 1;
    if (s.no_strip > 1) s.no_strip = 1;
    if (s.music_off > 1) s.music_off = 1;
    if (s.original > 1) s.original = 1;
    // (the switches gone with the menus of v0.209.0 - Tom: always on now -
    // never off, whatever an older file says)
    s.comforts_off2 &= static_cast<uint8_t>(~(kOff2Cheats | kOff2ReadTime));
    s.scale_15x = 0;
    return s;
}

void settings_save(const Settings& s)
{
    if (!storage_begin()) return;
    uint8_t d[4 + kSize2] = {};
    memcpy(d, &kMagic2, 4);
    memcpy(d + 4, &s, sizeof s);
    File f = LittleFS.open(kPath, "w");
    if (!f) return;
    f.write(d, sizeof d);
    f.close();
}
