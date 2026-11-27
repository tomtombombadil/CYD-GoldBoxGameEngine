// Where a game's files are in a folder of /GOLDBOX - the card scan's rules,
// on the platform layer (plat::), shared by the boards' scan
// (src/app/library.cpp) and the PortMaster port's (Tom, 2026-11-27).
// A whole install may be copied as it is: the .DAX files in the folder, one
// folder down (GOG: beside DOSBox's own folder) or two (the Steam / SNEG
// releases: <folder>/GAME/<SHORT>) - the shallowest wins; the HLIB games'
// .TLB / .GLB files up to three folders down, in DISK1-3.
// Fixed buffers only (the boards' RAM); a folder's listing is closed before
// going down into it (the card's few file slots). Header only: the engine's
// builds elsewhere (the port, the Android app) need no new file in their lists.
#pragma once

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <strings.h>

#include "engine/games.h"
#include "platform/file.h"

namespace game_find {

struct Found {
    char        data_dir[96] = {};  // where the game files are, relative to /GOLDBOX (an HLIB
                                    // game: the folder holding DISK1-3)
    bool        hlib = false;       // .TLB / .GLB libraries instead of .DAX files
    int         files = 0;          // .DAX files (.TLB + .GLB for an HLIB game)
    games::Game game = games::Game::Unknown;    // by the folder's name, else by the data folder's
};

enum class Look : unsigned char {
    NoGame,     // no game files in the folder
    Game,       // f filled (game may be Unknown: files of a game the engine doesn't know)
    NotOurs,    // Unlimited Adventures - not one of the engine's games (Tom, 2026-10-24): passed over
};

namespace detail {

inline bool ends_with(const char* name, const char* ext)
{
    const size_t n = strlen(name), m = strlen(ext);
    return n > m && strcasecmp(name + n - m, ext) == 0;
}
inline bool is_dax(const char* name) { return ends_with(name, ".DAX"); }
inline bool is_hlib(const char* name) { return ends_with(name, ".TLB") || ends_with(name, ".GLB"); }

// (some file systems' names are the whole path)
inline const char* base_name(const char* path)
{
    const char* s = strrchr(path, '/');
    return s ? s + 1 : path;
}

inline bool folder_path(char* path, size_t cap, const char* rel)
{
    return snprintf(path, cap, "%s/%s", games::kRootDir, rel) < static_cast<int>(cap);
}

// How many files of /GOLDBOX/<rel> pass `want` (-1: no such folder)
inline int count(const char* rel, bool (*want)(const char*))
{
    char path[160];
    if (!folder_path(path, sizeof path, rel)) return -1;
    plat::File dir = plat::open(path);
    if (!dir || !dir.isDirectory()) return -1;
    int n = 0;
    for (plat::File f = dir.openNextFile(); f; f = dir.openNextFile()) {
        if (!f.isDirectory() && want(base_name(f.name()))) ++n;
        f.close();
    }
    dir.close();
    return n;
}

// The sub-folder of /GOLDBOX/<rel> coming next by name after `after` ("" =
// the first) into name - the listing read afresh and closed again, so a
// walk down holds one folder open at a time and only a name a level (the
// boards' file slots and stack); the same folder wins on every card
inline bool next_sub(const char* rel, const char* after_in, char* name, size_t cap)
{
    char after[40];
    snprintf(after, sizeof after, "%s", after_in);      // (may be name itself)
    char path[160];
    if (!folder_path(path, sizeof path, rel)) return false;
    plat::File dir = plat::open(path);
    if (!dir || !dir.isDirectory()) return false;
    bool got = false;
    for (plat::File f = dir.openNextFile(); f; f = dir.openNextFile()) {
        const char* nm = base_name(f.name());
        if (f.isDirectory() && strlen(nm) < cap && strcmp(nm, after) > 0 && (!got || strcmp(nm, name) < 0)) {
            strcpy(name, nm);
            got = true;
        }
        f.close();
    }
    dir.close();
    return got;
}

inline bool join(char* out, size_t cap, const char* rel, const char* name)
{
    return snprintf(out, cap, "%s/%s", rel, name) < static_cast<int>(cap);
}

// The first folder exactly `depth` levels below rel (0: rel itself) holding
// .DAX files: its path into out, and how many
inline int dax_at_depth(const char* rel, int depth, char* out, size_t cap)
{
    if (depth == 0) {
        const int n = count(rel, is_dax);
        if (n > 0) snprintf(out, cap, "%s", rel);
        return n > 0 ? n : 0;
    }
    char name[40] = "", sub[96];
    while (next_sub(rel, name, name, sizeof name)) {
        if (!join(sub, sizeof sub, rel, name)) continue;
        const int n = dax_at_depth(sub, depth - 1, out, cap);
        if (n > 0) return n;
    }
    return 0;
}

// .TLB / .GLB files in rel and up to `depth` folders below it
inline int count_hlib(const char* rel, int depth)
{
    int n = count(rel, is_hlib);
    if (n < 0) return 0;
    char name[40] = "", sub[96];
    while (depth > 0 && next_sub(rel, name, name, sizeof name))
        if (join(sub, sizeof sub, rel, name)) n += count_hlib(sub, depth - 1);
    return n;
}

// The first folder up to `depth` levels below rel (0: rel itself) holding a
// DISK1 folder (GOG: the game's folder; the SNEG release: <folder>/GAME/DQK)
inline bool disk_root(const char* rel, int depth, char* out, size_t cap)
{
    char path[160];
    if (snprintf(path, sizeof path, "%s/%s/DISK1", games::kRootDir, rel) < static_cast<int>(sizeof path) &&
        plat::exists(path)) {
        snprintf(out, cap, "%s", rel);
        return true;
    }
    if (depth == 0) return false;
    char name[40] = "", sub[96];
    while (next_sub(rel, name, name, sizeof name))
        if (join(sub, sizeof sub, rel, name) && disk_root(sub, depth - 1, out, cap)) return true;
    return false;
}

} // namespace detail

// The folders of /GOLDBOX that hold no game: the engine's own (_CYD), the
// Steam / SNEG collection's Launcher (its box art) and the Gold Box
// Companion's GBC
inline bool passed_over(const char* folder)
{
    return strcasecmp(folder, "_CYD") == 0 || strcasecmp(folder, "Launcher") == 0 || strcasecmp(folder, "GBC") == 0;
}

// What /GOLDBOX/<folder> holds
inline Look look(const char* folder, Found& f)
{
    using namespace detail;
    f = Found{};
    snprintf(f.data_dir, sizeof f.data_dir, "%s", folder);
    int n = 0;
    for (int depth = 0; n == 0 && depth <= 2; ++depth) n = dax_at_depth(folder, depth, f.data_dir, sizeof f.data_dir);
    if (n == 0) {
        n = count_hlib(folder, 3);
        f.hlib = true;
        if (n > 0) disk_root(folder, 2, f.data_dir, sizeof f.data_dir);
    }
    if (n == 0) return Look::NoGame;
    f.files = n;
    f.game = games::from_folder_name(folder);
    if (f.game == games::Game::Unknown && strcmp(f.data_dir, folder) != 0)
        f.game = games::from_folder_name(base_name(f.data_dir));     // (.../GAME/CURSE)
    return f.game == games::Game::UnlimitedAdventures ? Look::NotOurs : Look::Game;
}

} // namespace game_find
