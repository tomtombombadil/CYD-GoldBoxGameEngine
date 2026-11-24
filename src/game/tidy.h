// Clean Up Game Files (Tom, 2026-11-24): the player copies whole GOG / Steam
// installs onto the card or device; most of what comes with them is for
// DOSBox, the installers, the Steam launcher or the Gold Box Companion and
// is never read by the engine. This takes it away, folder by folder, leaving
// everything the engine reads, every saved game and character, and EVERY PDF
// (Tom: the player may read them - Android especially).
//
// The rules (tidy.cpp `verdict`), for each file in a found game's folder:
//   kept always - a PDF; anything under a folder named SAVE* (SAVE, SAVE_CH,
//     SAVE_KN, Steam's "Default files/Saves") or _CYD; saved games and
//     characters wherever they are (CHRDAT*, SAVGAM*, VAULT*, .GUY .SWG .FX
//     .CHA .QCH .QSV .WHO .STF .SFX .ITM .SPC .GAM .JNK .WIZ .EFF .THG .EFX
//     .PTY .SAV); the game's icon (goggame-*.ico / .dll, Steam's game.ico);
//     the Gold Box Companion's journal (GBC/Games/<game>/Game.dat, JE_*.jpg);
//   in the game's own data folder (where its DAX / TLB / GLB files are) -
//     removed: DOS drivers and tools (.COM .BAT .DRV .SMS .VOC, every .EXE
//     but the game's program START.EXE / GAME.EXE / DQK.EXE), the Dark
//     Queen's music for other sound cards (.ADV but ADLIB.ADV, .XMI but
//     ADDQ*.XMI); kept: everything else (unknown files too);
//   elsewhere - removed: what DOSBox, GOG's installer / Galaxy, Steam's
//     launcher scripts and the Companion's program bring (.EXE .DLL .BAT
//     .COM .LNK .CONF .INI .TXT .LOG .HASHDB .INFO .SCRIPT .MSG .INF .GZ,
//     gog.ico, Support.ico, unins000.dat, GBC.dat, metadata, the Companion's
//     .BMP pictures); kept: anything else (a .zip of the player's, say -
//     listed as left alone).
// The Steam collection's Launcher folder: only the engine's box art
// (games::box_art_file) stays. Folders emptied by it go too - never the
// game's folder, its data folder, a SAVE* folder or _CYD. Folders the scan
// doesn't know as a game (the GOG Companion's /GOLDBOX/GBC, Unlimited
// Adventures, anything else) are not touched.
//
// Plain C++ on the platform layer (platform/file.h): the boards, the PC
// tools and the ports call the same code.
#pragma once
#include <cstddef>
#include <cstdint>

#include "engine/games.h"

namespace tidy {

struct Report {
    int      files = 0;              // files removed (or that would be, on a dry run)
    uint64_t bytes = 0;              // their size
    int      folders = 0;            // folders removed
    int      left = 0;               // unknown files left alone
    int      failed = 0;             // removals that didn't go through
};

// Each line it does (a removal, an unknown file left, a failure) - for a
// log or a screen; ctx is the caller's
using Say = void (*)(const char* line, void* ctx);

enum class Verdict : uint8_t { Keep, Remove, Unknown };

// What happens to a file, by its path inside the game's folder (`rel`,
// '/'-separated, no leading '/') and the data folder's path inside it
// (`data`, "" when the game's folder is it)
Verdict verdict(const char* rel, const char* data);

// One found game: its folder and its data folder, both relative to
// /GOLDBOX (the scan's GameDir folder / data_dir). apply false: a dry run
// (nothing removed, the report says what would go).
void clean_game(const char* folder, const char* data_dir, bool apply, Report& r, Say say = nullptr, void* ctx = nullptr);
// The Steam collection's Launcher folder, if there
void clean_launcher(bool apply, Report& r, Say say = nullptr, void* ctx = nullptr);

} // namespace tidy
