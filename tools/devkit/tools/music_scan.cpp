// music_scan: where each game's scripts start music (2026-11-14, Tom: "when
// does music actually play?"). Reads the player's script files (ECL*.DAX,
// the Dark Queen's DISK3/ECL.GLB), decodes every block from its first byte
// on and lists:
//   Darkness-type opcodes (Darkness, Treasures, Queen): SOUND (0x2C) with a
//     value 0x80 up = music (value & 0x7F = the song)
//   Curse-type opcodes: CALL (0x2D) of the sound call with the word 0x03DE
//     set to 0x80 up by the last SAVE before it (Gateway's music)
// and any SOUND / sound-call whose value isn't a plain number.
// Build: g++ -std=gnu++2b -O1 -Isrc -Itools/host tools/devkit/tools/music_scan.cpp
//        src/engine/*.cpp tools/host/*.cpp -o /tmp/claude-0/music_scan
// Run:   music_scan <game number 1-9> <folder holding the ECL files>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <string>
#include <vector>
#include <algorithm>
#include "engine/dax.h"
#include "engine/ecl.h"
#include "engine/hlib.h"
#include "engine/profile.h"
#include "engine/games.h"
#include "engine/ecl_vm.h"

static std::vector<uint8_t> slurp(const std::string& p) {
    std::vector<uint8_t> v;
    FILE* f = fopen(p.c_str(), "rb");
    if (!f) return v;
    fseek(f, 0, SEEK_END);
    v.resize(ftell(f));
    fseek(f, 0, SEEK_SET);
    if (fread(v.data(), 1, v.size(), f) != v.size()) v.clear();
    fclose(f);
    return v;
}

static bool dk_type(games::Game g) {
    return g == games::Game::PoolsOfDarkness || g == games::Game::TreasuresOfTheSavageFrontier ||
           g == games::Game::DarkQueenOfKrynn || g == games::Game::DeathKnightsOfKrynn;
}

uint32_t g_now = 0;          // (the PC platform layer's clock)
static int total_music = 0;

// TEXT=1: the first words the script prints after the music starts (where
// in the story it is)
static void text_after(const uint8_t* code, uint32_t len, uint32_t from, const ecl::OpSet& set) {
    if (!getenv("TEXT")) return;
    uint32_t at = from;
    for (int k = 0; k < 40 && at < len; ++k) {
        ecl::Insn in;
        if (!ecl::decode(code, len, at, set, in) || in.next <= at) { ++at; continue; }
        for (uint32_t i = in.at + 1; i + 2 < in.next; ++i)
            if (code[i] == 0x80 && code[i + 1] > 3 && i + 2 + code[i + 1] <= in.next) {
                char t[300];
                ecl::unpack_string(code + i + 2, code[i + 1], t, sizeof t);
                printf("      \"%.120s\"\n", t);
                return;
            }
        at = in.next;
    }
}
static bool constant(const ecl::Operand& o) { return o.code == 0 || o.code == 2; }  // (2: a word)

static void scan(const char* file, int id, const uint8_t* code, uint32_t len,
                 const ecl::OpSet& set, games::Game g) {
    int last3de = -1;
    uint32_t at = 0;
    while (at < len) {
        ecl::Insn in;
        if (!ecl::decode(code, len, at, set, in) || in.next <= at) { ++at; continue; }
        if (in.op == 0x09 && in.count >= 2 && in.ops[1].word() == 0x03DE &&
            in.ops[1].code != 0) {
            last3de = constant(in.ops[0]) ? (in.ops[0].code ? in.ops[0].word() : in.ops[0].low) : -2;
        }
        if (dk_type(g) && in.op == 0x2C && in.count >= 1) {
            const auto& o = in.ops[0];
            if (!constant(o))
                printf("%s block %d @%04X: SOUND [%04X] (a variable)\n", file, id, in.at, o.word());
            else if (g == games::Game::DarkQueenOfKrynn && o.word() >= 0x20 && o.word() <= 0x27) {
                // (the Dark Queen: sound ids 0x20-0x27 are its songs - DQK.EXE's
                // sound routine, file 0x12501: DS 0x1F21 file, 0x1F31 sequence)
                printf("%s block %d @%04X: MUSIC song %d\n", file, id, in.at, o.word() - 0x1F);
                text_after(code, len, in.next, set);
                ++total_music;
            } else if ((o.word() & 0xFF) >= 0x80) {
                printf("%s block %d @%04X: MUSIC song %d\n", file, id, in.at, o.word() & 0x7F);
                text_after(code, len, in.next, set);
                ++total_music;
            }
        }
        if (!dk_type(g) && in.op == 0x2D && in.count >= 1 && in.ops[0].word() == set.sound_call) {
            if (last3de >= 0x80) {
                printf("%s block %d @%04X: MUSIC song %d (CALL %04X, 03DE=%02X)\n", file, id,
                       in.at, last3de & 0x7F, set.sound_call, last3de);
                ++total_music;
            } else if (last3de == -2) {
                printf("%s block %d @%04X: sound CALL with 03DE a variable\n", file, id, in.at);
            }
        }
        at = in.next;
    }
}

int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "music_scan <game 1-9> <folder>\n"); return 1; }
    const auto g = static_cast<games::Game>(atoi(argv[1]));
    const profile::Profile* p = profile::first(g);
    if (!p || !p->ecl_ops) { fprintf(stderr, "no profile / opcodes\n"); return 1; }
    const ecl::OpSet& set = *p->ecl_ops;
    std::string dir = argv[2];
    if (g == games::Game::DarkQueenOfKrynn) {
        auto bytes = slurp(dir + "/DISK3/ECL.GLB");
        dax::MemorySource src(bytes.data(), bytes.size());
        hlib::Library lib;
        if (hlib::open(src, 0, bytes.size(), lib) != hlib::Status::Ok) { fprintf(stderr, "ECL.GLB?\n"); return 1; }
        const int n = hlib::dir_count(src, lib);
        for (int k = 0; k < n; ++k) {
            uint16_t id, idx;
            uint32_t pos, len;
            if (!hlib::dir_item(src, lib, k, id, idx) || !hlib::entry(src, lib, idx, pos, len)) continue;
            scan("ECL.GLB", id, bytes.data() + pos, len, set, g);
        }
    } else {
        std::vector<std::string> files;
        if (DIR* d = opendir(dir.c_str())) {
            while (dirent* e = readdir(d)) {
                std::string n = e->d_name, u = n;
                for (auto& c : u) c = toupper(c);
                if (u.rfind("ECL", 0) == 0 && u.size() > 4 && u.substr(u.size() - 4) == ".DAX") files.push_back(n);
            }
            closedir(d);
        }
        std::sort(files.begin(), files.end());
        static dax::Index index;
        static uint8_t buf[70000];
        for (auto& f : files) {
            auto bytes = slurp(dir + "/" + f);
            dax::MemorySource src(bytes.data(), bytes.size());
            if (dax::read_index(src, index) != dax::Status::Ok) continue;
            for (int i = 0; i < index.count; ++i) {
                const auto& e = index.entries[i];
                const uint32_t got = dax::load_block(src, index, e, buf);
                if (got < 2) continue;
                scan(f.c_str(), e.id, buf + 2, got - 2, set, g);
            }
        }
    }
    printf("== %d music starts\n", total_music);
    return 0;
}
