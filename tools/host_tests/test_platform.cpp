// The platform layer's PC side (tools/host): the files the game code reads
// and writes, as the board's card gives them. CI builds it with
//   g++ -std=c++17 -Wall -Werror -fsanitize=address,undefined -Itools/host -Isrc
//       -o test_platform tools/host_tests/test_platform.cpp tools/host/*.cpp src/engine/dax.cpp
#include "game/game_find.h"
#include "game/tidy.h"
#include "host.h"
#include "platform/file.h"
#include "platform/memlog.h"
#include "platform/platform.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <vector>

uint32_t g_now = 1000;
static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)

int main()
{
    char tmpl[] = "/tmp/test_platform_XXXXXX";
    const char* root = mkdtemp(tmpl);
    if (!root) return 1;
    pc::card_root() = root;
    CHECK(plat::card_ready());
    CHECK(pc::host_path("/GOLDBOX/A/B.DAX") == std::string(root) + "/A/B.DAX");
    CHECK(pc::host_path("/OTHER") == "/OTHER");
    // The clock is the driver's
    g_now = 1234;
    CHECK(plat::now_ms() == 1234);
    // A folder, a file written, read back, its size, seeking
    CHECK(!plat::exists("/GOLDBOX/GAME"));
    CHECK(plat::mkdir("/GOLDBOX/GAME") && plat::exists("/GOLDBOX/GAME"));
    std::vector<uint8_t> data(1500);
    for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<uint8_t>(i * 7 + 3);
    {
        plat::File w = plat::open("/GOLDBOX/GAME/F.BIN", "w");
        CHECK(static_cast<bool>(w) && !w.isDirectory());
        CHECK(w.write(data.data(), 1000) == 1000);
        w.close();
        CHECK(!w);
        plat::File a = plat::open("/GOLDBOX/GAME/F.BIN", "a");
        CHECK(a.write(data.data() + 1000, 500) == 500);
    }
    CHECK(pc::open_files() == 0);                    // (the last copy gone: closed)
    plat::File r = plat::open("/GOLDBOX/GAME/F.BIN");
    CHECK(static_cast<bool>(r) && r.size() == 1500);
    uint8_t b[16];
    CHECK(r.seek(1495) && r.read(b, 16) == 5 && b[0] == data[1495] && b[4] == data[1499]);
    // A DAX source over it: reads across its 512-byte buffer, past the end
    {
        plat::FileSource src(r);
        CHECK(src.size() == 1500);
        std::vector<uint8_t> got(1500);
        CHECK(src.read_at(0, got.data(), 1500) == 1500 && got == data);
        CHECK(src.read_at(1400, got.data(), 300) == 100 && got[0] == data[1400] && got[99] == data[1499]);
        CHECK(src.read_at(500, got.data(), 30) == 30 && memcmp(got.data(), data.data() + 500, 30) == 0);
        CHECK(src.read_at(1500, got.data(), 4) == 0);
    }
    // Copies share the file; close() closes it for all of them (as Arduino's)
    {
        plat::File copy = r;
        CHECK(pc::open_files() == 1);
        r.close();
        CHECK(!r && !copy);
        CHECK(pc::open_files() == 0);
    }
    // The card's 3 file slots: a fourth open fails
    {
        plat::File f1 = plat::open("/GOLDBOX/GAME/F.BIN");
        plat::File f2 = plat::open("/GOLDBOX/GAME/F.BIN");
        plat::File f3 = plat::open("/GOLDBOX/GAME/F.BIN");
        CHECK(f1 && f2 && f3 && pc::open_files() == 3);
        plat::File f4 = plat::open("/GOLDBOX/GAME/F.BIN");
        CHECK(!f4);
        f2.close();
        plat::File f5 = plat::open("/GOLDBOX/GAME/F.BIN");
        CHECK(static_cast<bool>(f5));
    }
    CHECK(pc::open_files() == 0);
    // A folder's entries, each with its own name; missing files
    {
        plat::File w = plat::open("/GOLDBOX/GAME/G.TXT", "w");
        CHECK(w.write(reinterpret_cast<const uint8_t*>("hi"), 2) == 2);
    }
    CHECK(plat::mkdir("/GOLDBOX/GAME/SUB"));
    plat::File dir = plat::open("/GOLDBOX/GAME");
    CHECK(dir.isDirectory());
    int files = 0, dirs = 0;
    bool seen_f = false, seen_g = false, seen_sub = false;
    for (plat::File e = dir.openNextFile(); e; e = dir.openNextFile()) {
        if (e.isDirectory()) ++dirs;
        else ++files;
        seen_f |= strcmp(e.name(), "F.BIN") == 0;
        seen_g |= strcmp(e.name(), "G.TXT") == 0 && e.size() == 2;
        seen_sub |= strcmp(e.name(), "SUB") == 0 && e.isDirectory();
    }
    CHECK(files == 2 && dirs == 1 && seen_f && seen_g && seen_sub);
    dir.close();
    CHECK(!plat::open("/GOLDBOX/GAME/NONE.BIN"));
    CHECK(plat::remove("/GOLDBOX/GAME/G.TXT") && !plat::exists("/GOLDBOX/GAME/G.TXT"));
    // rename: onto a new name, then over an existing file (replaced whole);
    // a missing source fails and leaves the target as it was
    {
        plat::File t = plat::open("/GOLDBOX/GAME/T1~", "w");
        CHECK(t && t.write(reinterpret_cast<const uint8_t*>("new"), 3) == 3);
        t.close();
        CHECK(plat::rename("/GOLDBOX/GAME/T1~", "/GOLDBOX/GAME/T1") && plat::exists("/GOLDBOX/GAME/T1") &&
              !plat::exists("/GOLDBOX/GAME/T1~"));
        t = plat::open("/GOLDBOX/GAME/T2~", "w");
        CHECK(t && t.write(reinterpret_cast<const uint8_t*>("newer!"), 6) == 6);
        t.close();
        CHECK(plat::rename("/GOLDBOX/GAME/T2~", "/GOLDBOX/GAME/T1"));
        t = plat::open("/GOLDBOX/GAME/T1", "r");
        uint8_t rb[8] = {};
        CHECK(t && t.size() == 6 && t.read(rb, 8) == 6 && memcmp(rb, "newer!", 6) == 0);
        t.close();
        CHECK(!plat::rename("/GOLDBOX/GAME/NONE~", "/GOLDBOX/GAME/T1") && plat::exists("/GOLDBOX/GAME/T1"));
        plat::remove("/GOLDBOX/GAME/T1");
    }
    // A default File does nothing
    plat::File none;
    CHECK(!none && none.size() == 0 && none.read(b, 4) == 0 && !none.isDirectory() && !none.openNextFile());
    // The memory figures and the memory log: unknown / plenty on a PC
    CHECK(plat::free_heap() == 0 && plat::largest_block() == 0);
    CHECK(memlog::largest_kb() >= 999 || getenv("LOWMEM"));
    // Clean Up Game Files (game/tidy.*): a GOG and a Steam install made of
    // empty files with the releases' own names - what goes and what stays
    {
        auto put = [&](const std::string& rel) {
            std::string path = std::string(games::kRootDir) + "/" + rel;
            for (size_t k = strlen(games::kRootDir) + 1; (k = path.find('/', k)) != std::string::npos; ++k)
                plat::mkdir(path.substr(0, k).c_str());
            plat::File f = plat::open(path.c_str(), "w");
            f.write(reinterpret_cast<const uint8_t*>("x"), 1);
            f.close();
        };
        auto there = [&](const std::string& rel) { return plat::exists((std::string(games::kRootDir) + "/" + rel).c_str()); };
        const char* gog[] = {"Pools of Darkness/GAME.EXE", "Pools of Darkness/GAME.OVR", "Pools of Darkness/ECL1.DAX",
                             "Pools of Darkness/POOL4.CFG", "Pools of Darkness/ITEMS", "Pools of Darkness/FINAL.ACF",
                             "Pools of Darkness/INTRO.BMA", "Pools of Darkness/Adventurers Journal.pdf",
                             "Pools of Darkness/goggame-1432643408.ico", "Pools of Darkness/SAVE/CHRDATA1.SAV",
                             "Pools of Darkness/SAVE/VAULTA.DAT", "Pools of Darkness/Manual.pdf", nullptr};
        const char* gog_out[] = {"Pools of Darkness/VACE.EXE", "Pools of Darkness/CONTROL.EXE", "Pools of Darkness/EGA.DRV",
                                 "Pools of Darkness/IBMSND.COM", "Pools of Darkness/P1.SMS", "Pools of Darkness/START.BAT",
                                 "Pools of Darkness/unins000.exe", "Pools of Darkness/unins000.dat", "Pools of Darkness/gog.ico",
                                 "Pools of Darkness/goggame-1432643408.hashdb", "Pools of Darkness/dosbox_pod.conf",
                                 "Pools of Darkness/Launch Pools of Darkness.lnk", "Pools of Darkness/DOSBOX/DOSBox.exe",
                                 "Pools of Darkness/DOSBOX/Documentation/README.txt", nullptr};
        const char* steam[] = {"The Dark Queen of Krynn/GAME/DQK/DQK.EXE", "The Dark Queen of Krynn/GAME/DQK/DISK1/ADLIB.ADV",
                               "The Dark Queen of Krynn/GAME/DQK/DISK1/ADDQ1.XMI", "The Dark Queen of Krynn/GAME/DQK/DISK1/INSTR.AD",
                               "The Dark Queen of Krynn/GAME/DQK/DISK2/MONCHA.GLB", "The Dark Queen of Krynn/GAME/DQK/SAVGAMA.QSV",
                               "The Dark Queen of Krynn/GAME/DQK/SAVE/BALTHIRN.QCH", "The Dark Queen of Krynn/GAME/DKK/SAVE/x.GAM",
                               "The Dark Queen of Krynn/game.ico", "The Dark Queen of Krynn/Documentation/Journal-ENG.pdf",
                               "The Dark Queen of Krynn/Default files/Saves/SAVGAMA.QSV",
                               "The Dark Queen of Krynn/GBC/Games/07. The Dark Queen of Krynn/Game.dat",
                               "The Dark Queen of Krynn/GBC/Games/07. The Dark Queen of Krynn/JE_012.jpg",
                               "The Dark Queen of Krynn/Documentation/mine.zip", nullptr};
        const char* steam_out[] = {"The Dark Queen of Krynn/GAME/DQK/DISK1/PCDQ1.XMI", "The Dark Queen of Krynn/GAME/DQK/DISK1/TANDY.ADV",
                                   "The Dark Queen of Krynn/GAME/DQK/DISK1/SFXDQ.VOC", "The Dark Queen of Krynn/GAME/DQK/INSTALL.EXE",
                                   "The Dark Queen of Krynn/GAME/DQK/START.BAT", "The Dark Queen of Krynn/base.conf",
                                   "The Dark Queen of Krynn/run-game.bat", "The Dark Queen of Krynn/metadata",
                                   "The Dark Queen of Krynn/stdout.txt", "The Dark Queen of Krynn/Default files/graphics.conf",
                                   "The Dark Queen of Krynn/GBC/GBC.exe", "The Dark Queen of Krynn/GBC/GBC.dat",
                                   "The Dark Queen of Krynn/GBC/Games/07. The Dark Queen of Krynn/Baldric_1.bmp",
                                   "The Dark Queen of Krynn/GBC/Games/07. The Dark Queen of Krynn/XP.txt",
                                   "The Dark Queen of Krynn/DOSBOX/Source/dosbox-0.74-3.tar.gz",
                                   "The Dark Queen of Krynn/Documentation/Controls.exe", nullptr};
        const char* launcher[] = {"Launcher/Sprites/Backgrounds/Back-007.jpg", "Launcher/Sprites/Backgrounds/Back-016+.jpg", nullptr};
        const char* launcher_out[] = {"Launcher/Launcher.exe", "Launcher/Sprites/Backgrounds/Back-001+.jpg",
                                      "Launcher/Sprites/Title-007-PLO4.jpg", "Launcher/fonts/SouvenirStd-Light.otf", nullptr};
        for (const char* const* l : {gog, gog_out, steam, steam_out, launcher, launcher_out})
            for (; *l; ++l) put(*l);
        tidy::Report dry;
        tidy::clean_game("Pools of Darkness", "Pools of Darkness", false, dry);
        CHECK(dry.files == 14 && there("Pools of Darkness/VACE.EXE"));     // (a dry run removes nothing)
        tidy::Report r;
        tidy::clean_game("Pools of Darkness", "Pools of Darkness", true, r);
        tidy::clean_game("The Dark Queen of Krynn", "The Dark Queen of Krynn/GAME/DQK", true, r);
        tidy::clean_launcher(true, r);
        for (const char* const* l : {gog, steam, launcher})
            for (; *l; ++l) CHECK(there(*l));
        for (const char* const* l : {gog_out, steam_out, launcher_out})
            for (; *l; ++l) CHECK(!there(*l));
        CHECK(r.files == 14 + 16 + 4 && r.left == 1 && r.failed == 0);
        CHECK(!there("Pools of Darkness/DOSBOX") && !there("The Dark Queen of Krynn/DOSBOX") && !there("Launcher/fonts"));
        CHECK(there("The Dark Queen of Krynn/GAME/DKK/SAVE") && there("The Dark Queen of Krynn/GAME/DQK/DISK1"));
        // the verdicts on their own
        CHECK(tidy::verdict("GAME/CURSE/COPYCURS.EXE", "GAME/CURSE") == tidy::Verdict::Remove);
        CHECK(tidy::verdict("GAME/CURSE/START.EXE", "GAME/CURSE") == tidy::Verdict::Keep);
        CHECK(tidy::verdict("GAME/PAUSE6K.COM", "GAME/CURSE") == tidy::Verdict::Remove);
        CHECK(tidy::verdict("GAME/CURSE/SAVE/Notes.txt", "GAME/CURSE") == tidy::Verdict::Keep);
        CHECK(tidy::verdict("POOLRAD/CHRDATA1.ITM", "POOLRAD") == tidy::Verdict::Keep);
        CHECK(tidy::verdict("codewheel.exe", "POOLRAD") == tidy::Verdict::Remove);
        CHECK(tidy::verdict("Cluebook.PDF", "POOLRAD") == tidy::Verdict::Keep);
        // (the trees gone again)
        for (const char* const* l : {gog, steam, launcher})
            for (; *l; ++l) plat::remove((std::string(games::kRootDir) + "/" + *l).c_str());
        const std::string rm = "rm -rf '" + std::string(root) + "/Pools of Darkness' '" + std::string(root) +
                               "/The Dark Queen of Krynn' '" + std::string(root) + "/Launcher'";
        CHECK(system(rm.c_str()) == 0);
    }
    // Where a folder's game is (game/game_find.h, the boards' and the
    // port's scan): GOG, Steam's GAME/<SHORT>, the Dark Queen's DISK1-3,
    // Unlimited Adventures passed over, a folder with none
    {
        auto put = [&](const std::string& rel) {
            std::string path = std::string(games::kRootDir) + "/" + rel;
            for (size_t k = strlen(games::kRootDir) + 1; (k = path.find('/', k)) != std::string::npos; ++k)
                plat::mkdir(path.substr(0, k).c_str());
            plat::File f = plat::open(path.c_str(), "w");
            f.close();
        };
        for (const char* f : {"Curse of the Azure Bonds/ECL1.DAX", "Curse of the Azure Bonds/GEO1.DAX",
                              "Curse of the Azure Bonds/DOSBOX/x.txt", "CURSE/GAME/CURSE/ECL1.DAX",
                              "CURSE/GAME/CURSE/SAVE/a.sav", "CURSE/Documentation/j.pdf", "CURSE/GBC/Games/02. Curse/Game.dat",
                              "Disk Set/GAME/DQK/DISK1/CPIC.TLB", "Disk Set/GAME/DQK/DISK2/MONCHA.GLB",
                              "Disk Set/GAME/DQK/DISK3/x.TLB", "Disk Set/GAME/DQK/DQK.EXE", "Unlimited Adventures/DISK1/A.TLB",
                              "Odd/a/ZZ.DAX", "Odd/b/ZZ.DAX", "Odd/b/YY.DAX", "Nothing/readme.txt"})
            put(f);
        game_find::Found f;
        CHECK(game_find::look("Curse of the Azure Bonds", f) == game_find::Look::Game && !f.hlib && f.files == 2 &&
              strcmp(f.data_dir, "Curse of the Azure Bonds") == 0 && f.game == games::Game::CurseOfTheAzureBonds);
        CHECK(game_find::look("CURSE", f) == game_find::Look::Game && f.files == 1 &&
              strcmp(f.data_dir, "CURSE/GAME/CURSE") == 0 && f.game == games::Game::CurseOfTheAzureBonds);
        CHECK(game_find::look("Disk Set", f) == game_find::Look::Game && f.hlib && f.files == 3 &&
              strcmp(f.data_dir, "Disk Set/GAME/DQK") == 0 && f.game == games::Game::DarkQueenOfKrynn);
        CHECK(game_find::look("Unlimited Adventures", f) == game_find::Look::NotOurs);
        CHECK(game_find::look("Odd", f) == game_find::Look::Game && f.files == 1 && strcmp(f.data_dir, "Odd/a") == 0 &&
              f.game == games::Game::Unknown);          // (the first by name)
        CHECK(game_find::look("Nothing", f) == game_find::Look::NoGame);
        CHECK(game_find::look("Not there", f) == game_find::Look::NoGame);
        CHECK(pc::open_files() == 0);
        CHECK(game_find::passed_over("_cyd") && game_find::passed_over("Launcher") && game_find::passed_over("GBC") &&
              !game_find::passed_over("CURSE"));
        const std::string rm = "rm -rf '" + std::string(root) + "/Curse of the Azure Bonds' '" + std::string(root) +
                               "/CURSE' '" + std::string(root) + "/Disk Set' '" + std::string(root) +
                               "/Unlimited Adventures' '" + std::string(root) + "/Odd' '" + std::string(root) + "/Nothing'";
        CHECK(system(rm.c_str()) == 0);
    }
    // (tidy up)
    plat::remove("/GOLDBOX/GAME/F.BIN");
    rmdir((std::string(root) + "/GAME/SUB").c_str());
    rmdir((std::string(root) + "/GAME").c_str());
    rmdir(root);
    if (failures) {
        printf("test_platform: %d failure(s)\n", failures);
        return 1;
    }
    printf("test_platform: all checks passed\n");
    return 0;
}
