#include "storage.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <cstdio>

#include "boards/board_select.h"
#include "sdcard.h"

bool storage_begin()
{
    static bool tried = false;
    static bool ok    = false;
    if (!tried) {
        tried = true;
#if defined(CYD_QEMU)
        // (the emulator build: no internal flash writes - QEMU panics when a
        // flash erase / write disables the cache while the other core idles)
        return false;
#endif
        ok = LittleFS.begin(true);   // true = format if never formatted
        if (!ok) Serial.println("[storage] LittleFS mount failed - using defaults");
    }
    return ok;
}

bool storage_mkdir(const char* dir)
{
    if (!storage_begin()) return false;
    if (LittleFS.exists(dir)) return true;
    return LittleFS.mkdir(dir);
}

#if defined(BOARD_STORAGE_ON_CARD)
namespace {
// The card's place for a file ("/settings.bin" -> "/GOLDBOX/_CYD/settings.bin")
bool card_path(const char* path, char* out, size_t n)
{
    if (!sd_begin()) return false;
    snprintf(out, n, "/GOLDBOX/_CYD%s", path);
    return true;
}
} // namespace
#endif

fs::File storage_open(const char* path, const char* mode)
{
#if defined(BOARD_STORAGE_ON_CARD)
    char cp[64];
    if (card_path(path, cp, sizeof cp)) {
        if (mode[0] == 'w') {
            fs::FS& sd = sd_fs();
            if (!sd.exists("/GOLDBOX")) sd.mkdir("/GOLDBOX");
            if (!sd.exists("/GOLDBOX/_CYD")) sd.mkdir("/GOLDBOX/_CYD");
            fs::File f = sd.open(cp, "w");
            if (f) return f;
        } else if (sd_fs().exists(cp)) {
            fs::File f = sd_fs().open(cp, "r");
            if (f) return f;
        }
    }
    // (no card: the flash - a write then makes the picture jump once, better than losing it)
#endif
    if (!storage_begin()) return fs::File();
    if (mode[0] == 'r' && !LittleFS.exists(path)) return fs::File();
    return LittleFS.open(path, mode);
}

bool storage_exists(const char* path)
{
#if defined(BOARD_STORAGE_ON_CARD)
    char cp[64];
    if (card_path(path, cp, sizeof cp) && sd_fs().exists(cp)) return true;
#endif
    return storage_begin() && LittleFS.exists(path);
}
