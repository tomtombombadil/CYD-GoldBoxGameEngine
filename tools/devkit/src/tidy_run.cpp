// tidy_run <folder standing for /GOLDBOX> [apply] - Clean Up Game Files
// (game/tidy.*) on the PC: the games found as the scans find them (the
// shallowest folder with .DAX files, up to two down; else where DISK1 is),
// each one's removals listed ("Would remove ..." on a dry run), the
// Launcher folder's too, then the totals. For checking the rules on copies
// of the players' installs (scratch only - never game files in the repo).
// Build: g++ -std=c++17 -Itools/host -Isrc -o tidy_run tools/devkit/src/tidy_run.cpp
//        src/game/tidy.cpp src/engine/games.cpp tools/host/*.cpp src/engine/dax.cpp
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <strings.h>
#include <vector>

#include "game/tidy.h"
#include "host.h"
#include "platform/file.h"

namespace {

std::string card(const std::string& rel) { return std::string(games::kRootDir) + "/" + rel; }

int list(const std::string& rel, const char* ext, std::vector<std::string>* dirs)
{
    plat::File d = plat::open(card(rel).c_str());
    if (!d || !d.isDirectory()) return 0;
    int n = 0;
    for (plat::File f = d.openNextFile(); f; f = d.openNextFile()) {
        const char* nm = f.name();
        const size_t k = strlen(nm);
        if (f.isDirectory()) {
            if (dirs) dirs->push_back(nm);
        } else if (k > 4 && strcasecmp(nm + k - 4, ext) == 0) {
            ++n;
        }
        f.close();
    }
    return n;
}

int dax_at(const std::string& rel, int depth, std::string& where)
{
    std::vector<std::string> dirs;
    const int n = list(rel, ".DAX", &dirs);
    if (depth == 0) {
        if (n) where = rel;
        return n;
    }
    std::sort(dirs.begin(), dirs.end());
    for (const auto& s : dirs)
        if (int k = dax_at(rel + "/" + s, depth - 1, where)) return k;
    return 0;
}

bool disk_root(const std::string& rel, int depth, std::string& where)
{
    if (plat::exists(card(rel + "/DISK1").c_str())) {
        where = rel;
        return true;
    }
    if (depth == 0) return false;
    std::vector<std::string> dirs;
    list(rel, ".DAX", &dirs);
    std::sort(dirs.begin(), dirs.end());
    for (const auto& s : dirs)
        if (disk_root(rel + "/" + s, depth - 1, where)) return true;
    return false;
}

void say(const char* line, void*) { printf("  %s\n", line); }

} // namespace

uint32_t g_now = 0;

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "tidy_run <card folder> [apply]\n");
        return 2;
    }
    pc::card_root() = argv[1];
    const bool apply = argc > 2 && strcmp(argv[2], "apply") == 0;
    std::vector<std::string> folders;
    {
        plat::File root = plat::open(games::kRootDir);
        for (plat::File d = root.openNextFile(); d; d = root.openNextFile()) {
            if (d.isDirectory()) folders.push_back(d.name());
            d.close();
        }
    }
    std::sort(folders.begin(), folders.end());
    tidy::Report all;
    for (const auto& f : folders) {
        if (strcasecmp(f.c_str(), "_CYD") == 0 || strcasecmp(f.c_str(), "Launcher") == 0 ||
            strcasecmp(f.c_str(), "GBC") == 0)
            continue;
        std::string data = f;
        int n = 0;
        for (int depth = 0; n == 0 && depth <= 2; ++depth) n = dax_at(f, depth, data);
        if (n == 0 && !disk_root(f, 2, data)) continue;
        games::Game g = games::from_folder_name(f.c_str());
        if (g == games::Game::Unknown && data != f) g = games::from_folder_name(data.c_str() + data.rfind('/') + 1);
        if (g == games::Game::Unknown || g == games::Game::UnlimitedAdventures) continue;
        printf("== %s (data %s)\n", f.c_str(), data.c_str());
        tidy::Report r;
        tidy::clean_game(f.c_str(), data.c_str(), apply, r, say, nullptr);
        printf("   %d files, %.1f MB, %d folders, %d left alone, %d failed\n", r.files, r.bytes / 1048576.0, r.folders,
               r.left, r.failed);
        all.files += r.files, all.bytes += r.bytes, all.folders += r.folders, all.left += r.left, all.failed += r.failed;
    }
    printf("== Launcher\n");
    tidy::Report r;
    tidy::clean_launcher(apply, r, say, nullptr);
    printf("   %d files, %.1f MB, %d folders\n", r.files, r.bytes / 1048576.0, r.folders);
    all.files += r.files, all.bytes += r.bytes, all.folders += r.folders;
    printf("TOTAL %s: %d files, %.1f MB, %d folders, %d left alone, %d failed\n", apply ? "removed" : "to remove",
           all.files, all.bytes / 1048576.0, all.folders, all.left, all.failed);
    return all.failed ? 1 : 0;
}
