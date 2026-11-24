#include "game/tidy.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <strings.h>
#include <vector>

#include "platform/file.h"

namespace tidy {
namespace {

const char* base_of(const char* p)
{
    const char* s = strrchr(p, '/');
    return s ? s + 1 : p;
}

// The extension without its dot ("" for none)
const char* ext_of(const char* name)
{
    const char* d = strrchr(name, '.');
    return d ? d + 1 : "";
}

bool any_of(const char* s, const char* const* list)
{
    for (; *list; ++list)
        if (strcasecmp(s, *list) == 0) return true;
    return false;
}

bool starts(const char* s, const char* prefix) { return strncasecmp(s, prefix, strlen(prefix)) == 0; }

// A path component (a folder on the way) that keeps everything under it
bool keeping_folder(const char* rel)
{
    for (const char* p = rel; *p;) {
        const char* e = strchr(p, '/');
        if (!e) break;                                  // (the last component is the file)
        const std::string part(p, static_cast<size_t>(e - p));
        if (starts(part.c_str(), "SAVE") || strcasecmp(part.c_str(), "_CYD") == 0) return true;
        p = e + 1;
    }
    return false;
}

// Saved games and characters, wherever they sit
bool save_file(const char* name)
{
    static const char* const kExt[] = {"GUY", "SWG", "FX", "CHA", "QCH", "QSV", "WHO", "STF", "SFX", "ITM", "SPC",
                                       "GAM", "JNK", "WIZ", "EFF", "THG", "EFX", "PTY", "SAV", nullptr};
    return starts(name, "CHRDAT") || starts(name, "SAVGAM") || starts(name, "VAULT") || starts(name, "TMPVAULT") ||
           any_of(ext_of(name), kExt);
}

// rel inside data (or data itself's files); data "" = the game's folder
bool in_data(const char* rel, const char* data)
{
    const size_t n = strlen(data);
    if (n > 0 && (strncasecmp(rel, data, n) != 0 || rel[n] != '/')) return false;
    const char* rest = n ? rel + n + 1 : rel;
    // its own files, and the Dark Queen's DISK1-3
    const char* slash = strchr(rest, '/');
    if (!slash) return true;
    return starts(rest, "DISK") && !strchr(slash + 1, '/');
}

} // namespace

Verdict verdict(const char* rel, const char* data)
{
    const char* name = base_of(rel);
    const char* ext = ext_of(name);
    // Kept always
    if (strcasecmp(ext, "PDF") == 0 || keeping_folder(rel) || save_file(name)) return Verdict::Keep;
    if (starts(name, "goggame-") && (strcasecmp(ext, "ICO") == 0 || strcasecmp(ext, "DLL") == 0)) return Verdict::Keep;
    if (strcasecmp(name, "game.ico") == 0) return Verdict::Keep;
    if (starts(rel, "GBC/Games/") && (strcasecmp(name, "Game.dat") == 0 ||
                                      (starts(name, "JE_") && strcasecmp(ext, "JPG") == 0)))
        return Verdict::Keep;

    static const char* const kDosTools[] = {"COM", "BAT", "DRV", "SMS", "VOC", nullptr};
    static const char* const kPrograms[] = {"START.EXE", "GAME.EXE", "DQK.EXE", nullptr};
    if (in_data(rel, data)) {
        if (any_of(ext, kDosTools)) return Verdict::Remove;
        if (strcasecmp(ext, "EXE") == 0) return any_of(name, kPrograms) ? Verdict::Keep : Verdict::Remove;
        // (the Dark Queen's music: the engine plays the AdLib version)
        if (strcasecmp(ext, "ADV") == 0) return strcasecmp(name, "ADLIB.ADV") == 0 ? Verdict::Keep : Verdict::Remove;
        if (strcasecmp(ext, "XMI") == 0) return starts(name, "ADDQ") ? Verdict::Keep : Verdict::Remove;
        // (GOG's own files beside the game's: below)
        static const char* const kGogHere[] = {"LNK", "CONF", "INI", "HASHDB", "INFO", "SCRIPT", "MSG", "TXT",
                                              "DLL", nullptr};
        static const char* const kGogNames[] = {"gog.ico", "Support.ico", "unins000.dat", nullptr};
        if (any_of(ext, kGogHere) || any_of(name, kGogNames)) return Verdict::Remove;
        return Verdict::Keep;
    }
    static const char* const kJunk[] = {"EXE", "DLL", "BAT", "COM", "LNK", "CONF", "INI", "TXT", "LOG", "HASHDB",
                                        "INFO", "SCRIPT", "MSG", "INF", "GZ", "DRV", "SMS", "VOC", nullptr};
    static const char* const kJunkNames[] = {"gog.ico", "Support.ico", "unins000.dat", "GBC.dat", "metadata", nullptr};
    if (any_of(ext, kJunk) || any_of(name, kJunkNames)) return Verdict::Remove;
    if (starts(rel, "GBC/") && strcasecmp(ext, "BMP") == 0) return Verdict::Remove;   // (the Companion's own pictures)
    return Verdict::Unknown;
}

namespace {

struct Walk {
    bool apply;
    Report* r;
    Say say;
    void* ctx;
    const char* data;               // the data folder inside the game's
    bool launcher = false;
    void line(const char* fmt, const char* a, const char* b = "")
    {
        if (!say) return;
        char buf[320];
        snprintf(buf, sizeof buf, fmt, a, b);
        say(buf, ctx);
    }
};

bool box_art(const char* name)
{
    for (int g = 1; g < games::kGameCount; ++g) {
        const games::Game gm = static_cast<games::Game>(g);
        if (gm == games::Game::UnlimitedAdventures) continue;
        const char* f = games::box_art_file(gm);
        if (f && strcasecmp(f, name) == 0) return true;
    }
    return false;
}

// One folder: its files decided, its sub-folders walked (the names read and
// the listing closed first - the board has 3 file slots); true when it is
// empty afterwards
bool walk(Walk& w, const std::string& card, const std::string& rel, int depth)
{
    std::vector<std::string> files, dirs;
    std::vector<uint32_t> sizes;
    {
        plat::File d = plat::open(card.c_str());
        if (!d || !d.isDirectory()) return false;
        for (plat::File f = d.openNextFile(); f; f = d.openNextFile()) {
            const char* nm = base_of(f.name());
            if (f.isDirectory()) dirs.emplace_back(nm);
            else {
                files.emplace_back(nm);
                sizes.push_back(static_cast<uint32_t>(f.size()));
            }
            f.close();
        }
        d.close();
    }
    int left = 0;
    for (size_t i = 0; i < files.size(); ++i) {
        const std::string r = rel.empty() ? files[i] : rel + "/" + files[i];
        Verdict v;
        if (w.launcher) {
            v = starts(r.c_str(), "Sprites/Backgrounds/") && !strchr(r.c_str() + 20, '/') && box_art(files[i].c_str())
                    ? Verdict::Keep
                    : Verdict::Remove;
        } else {
            v = verdict(r.c_str(), w.data);
        }
        if (v == Verdict::Keep) {
            ++left;
            continue;
        }
        if (v == Verdict::Unknown) {
            ++left;
            ++w.r->left;
            w.line("Left alone (not known): %s/%s", card.c_str() + strlen(games::kRootDir) + 1, files[i].c_str());
            continue;
        }
        const std::string path = card + "/" + files[i];
        if (w.apply && !plat::remove(path.c_str())) {
            ++left;
            ++w.r->failed;
            w.line("Couldn't remove %s/%s", card.c_str() + strlen(games::kRootDir) + 1, files[i].c_str());
            continue;
        }
        ++w.r->files;
        w.r->bytes += sizes[i];
        w.line(w.apply ? "Removed %s/%s" : "Would remove %s/%s", card.c_str() + strlen(games::kRootDir) + 1,
               files[i].c_str());
    }
    for (const std::string& sub : dirs) {
        const std::string r = rel.empty() ? sub : rel + "/" + sub;
        const std::string path = card + "/" + sub;
        const bool empty = depth < 8 && walk(w, path, r, depth + 1);
        // (a folder kept even when empty: a save folder, the engine's own,
        // the data folder and the folders on its way)
        const size_t dn = strlen(w.data);
        const bool on_data_way = !w.launcher && dn > 0 && strncasecmp(w.data, r.c_str(), r.size()) == 0 &&
                                 (w.data[r.size()] == 0 || w.data[r.size()] == '/');
        const bool keep_dir = starts(sub.c_str(), "SAVE") || strcasecmp(sub.c_str(), "_CYD") == 0 || on_data_way ||
                              (!w.launcher && in_data((r + "/x").c_str(), w.data));
        if (!empty || keep_dir) {
            ++left;
            continue;
        }
        if (w.apply && !plat::rmdir(path.c_str())) {
            ++left;
            ++w.r->failed;
            w.line("Couldn't remove the folder %s", path.c_str() + strlen(games::kRootDir) + 1);
            continue;
        }
        ++w.r->folders;
    }
    return left == 0;
}

} // namespace

void clean_game(const char* folder, const char* data_dir, bool apply, Report& r, Say say, void* ctx)
{
    if (!folder || !folder[0] || strchr(folder, '/') || strcasecmp(folder, "_CYD") == 0 ||
        strcasecmp(folder, "GBC") == 0 || strcasecmp(folder, "Launcher") == 0)
        return;                                         // (only a game's own top folder)
    // The data folder inside the game's ("" when it is the game's folder)
    const size_t n = strlen(folder);
    const char* data = "";
    if (data_dir && strncasecmp(data_dir, folder, n) == 0 && data_dir[n] == '/') data = data_dir + n + 1;
    Walk w{apply, &r, say, ctx, data};
    const std::string card = std::string(games::kRootDir) + "/" + folder;
    walk(w, card, "", 0);
}

void clean_launcher(bool apply, Report& r, Say say, void* ctx)
{
    const std::string card = std::string(games::kRootDir) + "/Launcher";
    if (!plat::exists(card.c_str())) return;
    Walk w{apply, &r, say, ctx, ""};
    w.launcher = true;
    walk(w, card, "", 0);
}

} // namespace tidy
