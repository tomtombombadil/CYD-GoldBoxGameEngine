#include "settings.h"

#include <Arduino.h>
#include <LittleFS.h>

#include "hal/storage.h"

// A board whose usual way up is the engine's Rotate 180 (Tom, 2026-10-31:
// the 5.0" board, its USB port the other way round otherwise) - the
// setting's start before the player sets it (platformio.ini)
#ifndef BOARD_FLIPPED_DEFAULT
#define BOARD_FLIPPED_DEFAULT 0
#endif

namespace {

constexpr const char* kPath  = "/settings.bin";
constexpr uint32_t    kMagic = 0x53455431;   // "SET1"

struct File1 {
    uint32_t magic;
    Settings s;
    uint8_t  reserved[6];         // (the file stays 20 bytes: new fields come out of here as 0)
};

static_assert(sizeof(File1) == 20, "the settings file's size");

} // namespace

Settings settings_load()
{
    Settings s;
    s.flipped = BOARD_FLIPPED_DEFAULT;
    if (!storage_begin() || !LittleFS.exists(kPath)) return s;
    File f = LittleFS.open(kPath, "r");
    File1 d{};
    if (f && f.read(reinterpret_cast<uint8_t*>(&d), sizeof d) == sizeof d && d.magic == kMagic) s = d.s;
    f.close();
    if (s.brightness < kMinBrightness) s.brightness = kMinBrightness;
    if (s.sound == kSoundOff) {                 // (older files' Off: Mute since 2026-10-24)
        s.sound = kSoundTandy;
        s.volume_muted = 1;
    }
    if (s.sound == kSoundUnset || s.sound > kSoundTreasures) s.sound = kSoundTandy;     // (Tom: Tandy unless chosen otherwise)
    if (s.volume < kMinVolume) s.volume = kDefaultVolume;
    if (s.volume_muted > 1) s.volume_muted = 1;
    return s;
}

void settings_save(const Settings& s)
{
    if (!storage_begin()) return;
    File1 d{};
    d.magic = kMagic;
    d.s = s;
    File f = LittleFS.open(kPath, "w");
    if (!f) return;
    f.write(reinterpret_cast<const uint8_t*>(&d), sizeof d);
    f.close();
}
