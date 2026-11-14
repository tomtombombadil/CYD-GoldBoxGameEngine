// Behaviour checks, round 4 (sweep's `behave rules4`, -DBEHAVE_ROUND4 with
// -DBEHAVE_MATRIX - its cleared field and plain fighters; any game: GAME,
// BOOTSLOT, MAREA / MBLOCK, FIGHTAT as the fuzzer's; ONLY= table, turn,
// former, restore, rest, haste, moons, caster, learn, use, aiorder, canuse,
// wrap, qpages, qlost, aiturn, bandorder, quickacts, quickpage, trainspell). Expectations from the originals'
// code (rules_round4_facts.md in the Project: the turn routines of Curse's
// listing ovr014:1228, Gateway's GAME.OVR 0x17455, Champions' 0x16021, the
// Krynn moons' 0x3911F / 0x34E8C / 0x30E0B) and coab's facts (cast_restore,
// the training's lost levels, AffectHaste, the poison / disease handlers) -
// never from the engine's code. The turn table is found in the game's own
// program by its contents (row 1: 10 7 4 1 1 0 0), not at the profile's
// address, and the reference reads it there.

#include <fstream>
#include <iterator>

namespace r4 {

using namespace play;
const char* P = "rules4";
char w[240];
Making* m = nullptr;
int game() { return getenv("GAME") ? atoi(getenv("GAME")) : 2; }

// ---- The dice: d12, d20, d6 set (0: lowest)
int f12 = 0, f20 = 0, f6 = 0;
int die(int n)
{
    if (n == 12 && f12) return f12 - 1;
    if (n == 20 && f20) return f20 - 1;
    if (n == 6 && f6) return f6 - 1;
    return 0;
}
void dice(int a12, int a20, int a6 = 0)
{
    f12 = a12;
    f20 = a20;
    f6 = a6;
    create::g_die_hook = die;
}
void dice_off() { create::g_die_hook = nullptr; }

// ---- The program's own turn table
std::vector<uint8_t> prog;              // the file it was found in
int tbase = -1, tcols = 0;
bool read_file(const std::string& p, std::vector<uint8_t>& out)
{
    std::ifstream f(p, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), {});
    return !out.empty();
}
void find_table()
{
    // The program's whole image (unpacked as the engine opens it - its copy
    // or the packed file), searched for the table's row 1
    prog.clear();
    tcols = play::fg ? play::fg->t.turn_cols : 10;
    tbase = -1;
    plat::File f;
    if (!open_file(d->prof->program, f)) return;
    plat::FileSource src(f);
    exepack::Info info;
    if (exepack::parse(src, info) == exepack::Status::Ok) {
        uint8_t buf[1024];
        for (uint32_t at = 0; at < info.image_size; at += sizeof buf) {
            const size_t n = std::min<size_t>(sizeof buf, info.image_size - at);
            if (exepack::read(src, info, at, buf, n) != exepack::Status::Ok) break;
            prog.insert(prog.end(), buf, buf + n);
        }
    }
    f.close();
    static const uint8_t sig[] = {10, 7, 4, 1, 1, 0, 0};
    auto it = std::search(prog.begin(), prog.end(), std::begin(sig), std::end(sig));
    if (it != prog.end()) tbase = static_cast<int>(it - prog.begin()) - (tcols + 1);
}
int tv(int type, int col)
{
    const int at = tbase + type * tcols + col;
    return tbase >= 0 && at >= 0 && at < static_cast<int>(prog.size()) ? static_cast<int8_t>(prog[at]) : 99;
}

combat::Fighter& F(int i) { return play::fg->b.f[i]; }
int B = 0, room = 0;

bool fight()
{
    if (!mx::new_fight()) return false;
    B = play::fg->b.party_size;
    room = play::fg->b.n - B;
    return room > 0;
}
void levels(int i, std::initializer_list<std::pair<int, int>> lv, std::initializer_list<std::pair<int, int>> old = {}, int hd = 0)
{
    combat::Fighter& f = F(i);
    for (int k = 0; k < 8; ++k) f.rec[0x109 + k] = f.rec[0x111 + k] = 0;
    for (auto [k, l] : lv) f.rec[0x109 + k] = static_cast<uint8_t>(l);
    for (auto [k, l] : old) f.rec[0x111 + k] = static_cast<uint8_t>(l);
    f.rec[0x74] = 7;
    f.rec[0xE6] = static_cast<uint8_t>(hd);
    f.turned_undead = false;
    f.undead_turns = 0;
    f.god = 0;
}

// ---- T1: the table as the program has it
void table()
{
    if (game() == 3) {
        // Secret: no Turn at all - its fight menu "Move View Aim Use Cast Quick Done" (GAME.OVR 0xC101)
        std::vector<uint8_t> ovr;
        read_file(game_dir + "/GAME.OVR", ovr);
        const std::string s(ovr.begin(), ovr.end());
        const bool menu = s.find("\x05" "Cast \nQuick Done") != std::string::npos, turn = s.find("\x05Turn ") != std::string::npos;
        expect(P, "Secret: its fight menu has no Turn (the program's words), and the engine offers none",
               menu && !turn && !play::fg->t.turn_known, "menu %d Turn word %d engine %d", menu, turn, play::fg->t.turn_known);
        return;
    }
    find_table();
    const auto& t = play::fg->t;
    if (!t.turn_known || tbase < 0) {
        expect(P, "the turn table: in the program and read by the engine", false, "engine %d program %d", t.turn_known, tbase);
        return;
    }
    const int n = tcols == 10 ? 13 * 10 + 11 : 14 * tcols;
    int bad = 0, first = -1;
    for (int k = tcols; k < n; ++k)
        if (static_cast<int8_t>(t.turn[k]) != tv(0, k)) {
            if (first < 0) first = k;
            ++bad;
        }
    snprintf(w, sizeof w, "the turn table (%d columns): the engine's bytes = the program's own (found by its contents)", tcols);
    expect(P, w, bad == 0, "%d differ, first at %d", bad, first);
}

// ---- T2: the turning itself against the originals' routines
struct Case {
    std::vector<std::pair<int, int>> lv, old;
    int hd, god, c12, c20, c6;
    std::vector<int> types;
    int minus = 0;          // (11 columns: the script's penalty, game byte 0x0F)
};
// The reference: Curse / Pool (rule 0), Gateway (1), Champions (2),
// Darkness's (11 columns - its facts R13; Death Knights' the same with
// Majere's + 2, read 2026-11-07; the Dark Queen's DQK.EXE 0x25E77, read
// 2026-11-09: L = max(cleric level, paladin level - 2) (its 0xBE14: the plain
// level), Majere's (record word 0x19B = 2) + 2, 9-13 -> 9, 14+ -> 10, less
// the game block's byte 0x0F (0 when more); one d20 first, the count at the
// first undead found (D* d6 + 6, else d12); the finder 0x260BE takes the
// highest type up to the limit (13 first) and makes the limit that type; a
// roll under |value|: the limit - 1)
std::vector<int> ref(const Case& c, int rule, bool fixes)
{
    int lvl[8] = {}, old[8] = {};
    for (auto [k, l] : c.lv) lvl[k] = l;
    for (auto [k, l] : c.old) old[k] = l;
    int first = 0;
    while (first < 7 && !lvl[first]) ++first;
    const bool past = lvl[first] > c.hd;
    auto skill = [&](int k) { return lvl[k] + (past ? old[k] : 0); };
    std::vector<int> out;
    std::vector<bool> gone(c.types.size());
    const int roll = c.c20 ? c.c20 : 1;
    int count = c.c12 ? c.c12 : 1, extra = 6;
    if (tcols == 11) {
        // (Death Knights, rule 3 - GAME.OVR 0x1FEAA: Majere's clerics + 2)
        const int L = std::max(lvl[0], lvl[3] - 2) + (rule >= 3 && c.god == 2 ? 2 : 0);
        int col = L <= 8 ? L : L < 14 ? 9 : 10;
        col = c.minus > col ? 0 : col - c.minus;
        int limit = 13;
        count = -1;
        for (;;) {
            int best = -1, type = 0;
            for (size_t k = 0; k < c.types.size(); ++k)
                if (!gone[k] && c.types[k] > type && c.types[k] <= limit) best = static_cast<int>(k), type = c.types[k];
            if (best < 0 || count == 0) break;
            const int v = tv(type, col);
            if (count < 0) count = v < 0 ? (c.c6 ? c.c6 : 1) + 6 : (c.c12 ? c.c12 : 1);
            limit = type;                   // (0x260BE: the limit becomes the type found)
            if (roll < std::abs(v)) {
                if (--limit < 1) break;
                continue;
            }
            gone[best] = true;
            out.push_back(v > 0 ? best : best + 1000);
            if (extra > 0) --extra;
            if (--count == 0 && extra > 0 && v < 0) ++count;
        }
        return out;
    }
    if (rule == 2) {
        const int col = skill(0) + (c.god == 2 ? 2 : 0);
        int limit = 13;
        while (count > 0) {
            int best = -1, type = 0;
            for (size_t k = 0; k < c.types.size(); ++k)
                if (!gone[k] && c.types[k] > type && c.types[k] <= limit) best = static_cast<int>(k), type = c.types[k];
            if (best < 0) break;
            const int v = tv(type, col);
            if (roll < std::abs(v)) {
                limit = type - 1;
                continue;
            }
            limit = type;
            gone[best] = true;
            out.push_back(v > 0 ? best : best + 1000);
            if (extra > 0) --extra;
            if (--count == 0 && extra > 0 && v < 0) ++count;
        }
        return out;
    }
    int L = skill(0);
    if (rule == 1) L = std::max(L, skill(3) - 2);
    if (rule == 1 && L == 0) return out;
    const int col = L >= 1 && L <= 8 ? (rule == 1 || fixes ? L : lvl[0]) : L >= 9 && L <= 13 ? 9 : 10;
    while (count > 0) {
        int best = -1, type = 13;
        for (size_t k = 0; k < c.types.size(); ++k)
            if (!gone[k] && c.types[k] > 0 && c.types[k] < type) best = static_cast<int>(k), type = c.types[k];
        if (best < 0) break;
        const int v = tv(type, col);
        if (roll < std::abs(v)) break;
        gone[best] = true;
        out.push_back(v > 0 ? best : best + 1000);
        if (extra > 0) --extra;
        if (--count == 0 && extra > 0 && v < 0) ++count;
    }
    return out;
}
// The engine's: the cleric at 0, the undead from B on beside it
int force_fix = -1;     // (a fight's boot sets the Game Fixes from the settings: set again after it)
std::vector<int> engine_turn(const Case& c)
{
    fight();
    if (force_fix >= 0) rules::set_fixes(force_fix != 0);
    mx::put(0, 0, 10, 10, 50);
    levels(0, {}, {}, 0);
    for (auto [k, l] : c.lv) F(0).rec[0x109 + k] = static_cast<uint8_t>(l);
    for (auto [k, l] : c.old) F(0).rec[0x111 + k] = static_cast<uint8_t>(l);
    F(0).rec[0xE6] = static_cast<uint8_t>(c.hd);
    F(0).god = static_cast<uint8_t>(c.god);
    for (size_t k = 0; k < c.types.size(); ++k) {
        const int i = B + static_cast<int>(k);
        mx::put(i, 1, 11 + static_cast<int>(k % 4), 9 + static_cast<int>(k / 4), 30);
        F(i).rec[0xE9] = static_cast<uint8_t>(c.types[k]);
    }
    play::fg->b.undead_loaded = static_cast<int>(c.types.size());
    play::fg->b.turn_minus = c.minus;
    dice(c.c12, c.c20, c.c6);
    int out[16];
    const int n = combat::turn_undead(play::fg->b, play::fg->t, 0, play::rng, out, 16);
    dice_off();
    std::vector<int> got;
    for (int k = 0; k < n; ++k) got.push_back(out[k] >= 1000 ? out[k] - 1000 - B + 1000 : out[k] - B);
    return got;
}
std::string str(const std::vector<int>& v)
{
    std::string s;
    for (int x : v) s += " " + std::to_string(x);
    return s.empty() ? " none" : s;
}
void turning()
{
    if (game() == 3 || !play::fg->t.turn_known) return;
    if (tbase < 0) find_table();
    const int rule = play::fg->t.turn_rule;
    std::vector<Case> cases;
    for (int L : {1, 3, 5, 7, 9, 12, 14})
        for (int r20 : {1, 4, 7, 10, 13, 16, 19, 20})
            for (int c12 : {1, 2, 12}) cases.push_back({{{0, L}}, {}, 0, 0, c12, r20, 1, {1, 3, 5, 7, 9, 2}});
    // the mixes: several of a type, the weakest / highest first, D* extras
    // (three undead: the fights with few monster places - the Dark Queen's)
    for (int L : {1, 4, 8, 11, 14})
        for (int r20 : {2, 7, 11, 16, 20})
            for (int c12 : {1, 3}) cases.push_back({{{0, L}}, {}, 0, 0, c12, r20, 2, {1, 5, 10}});
    for (int c12 : {1, 3}) cases.push_back({{{0, 12}}, {}, 0, 0, c12, 20, 6, {1, 1, 1, 1, 1, 1, 1, 1}});
    cases.push_back({{{0, 4}}, {}, 0, 0, 12, 9, 1, {8, 2, 2, 11}});
    // paladins (Gateway: as a cleric 2 lower; Darkness's the same), Majere (Champions + 2)
    for (int P2 : {2, 3, 6, 11}) cases.push_back({{{3, P2}}, {}, 0, 0, 4, 12, 1, {1, 2, 3}});
    cases.push_back({{{0, 3}}, {}, 0, 2, 4, 12, 1, {1, 2, 3, 4}});
    for (int L : {3, 7, 12}) cases.push_back({{{0, L}}, {}, 0, 2, 4, 15, 1, {1, 2, 3}});
    // the script's penalty (11 columns: game byte 0x0F)
    if (tcols == 11)
        for (int mi : {1, 3, 9})
            for (int L : {4, 9, 14}) cases.push_back({{{0, L}}, {}, 0, 0, 5, 14, 3, {1, 2, 6}, mi});
    int bad = 0, done = 0;
    std::string first;
    for (const Case& c : cases) {
        if (static_cast<int>(c.types.size()) > room) continue;
        const bool pal = c.lv.size() && c.lv[0].first == 3;
        F(0).rec[0x109] = 0;
        const std::vector<int> got = engine_turn(c), want = ref(c, rule, rules::fixes());
        // (a paladin turns only where the game lets it: Gateway's / Darkness's menu)
        if (pal && !(rule == 1 || tcols == 11)) continue;
        ++done;
        if (got != want && !bad++) {
            snprintf(w, sizeof w, "level %d%s god %d d12 %d d20 %d d6 %d: got%s, the original's%s", c.lv[0].second, pal ? " (paladin)" : "",
                     c.god, c.c12, c.c20, c.c6, str(got).c_str(), str(want).c_str());
            first = w;
        }
    }
    snprintf(w, sizeof w, "turn undead (rule %d, %d columns): %d cases against the original's routine", rule, tcols, done);
    expect(P, w, bad == 0 && done > 0, "%d wrong, first %s", bad, first.c_str());
}

// ---- T3: who may turn (the menus): Curse's former clerics, Gateway's
// paladins, Champions' turns by the undead loaded
void former()
{
    if (game() == 3 || !play::fg->t.turn_known) return;
    if (tbase < 0) find_table();
    const int rule = play::fg->t.turn_rule;
    fight();
    mx::put(0, 0, 10, 10, 50);
    auto may = [&](std::initializer_list<std::pair<int, int>> lv, std::initializer_list<std::pair<int, int>> old, int hd) {
        levels(0, lv, old, hd);
        play::fg->b.undead_loaded = 2;
        return combat::can_turn(play::fg->b, play::fg->t, F(0));
    };
    if (tcols == 11) {
        expect(P, "Turn (Darkness's rule): a paladin of 3, not of 2; a cleric; a fighter not",
               may({{3, 3}}, {}, 0) && !may({{3, 2}}, {}, 0) && may({{0, 1}}, {}, 0) && !may({{2, 9}}, {}, 0));
        // Darkness (GAME.OVR 0x8D4B) / Treasures (0x9DD5) by skill levels
        // (0x392EE: a human past its former level - max(level, former));
        // Death Knights (0x94CC) and the Dark Queen (DQK.EXE 0x22DC7 menu /
        // 0x2E078 AI, its 0xBE14) by levels
        if (game() == 4 || game() == 9 || game() == 6 || game() == 7) {
            const bool sk = game() != 6 && game() != 7;
            const int r74 = F(0).rec[0x74];
            F(0).rec[0x74] = 7;
            const bool passed = may({{2, 9}}, {{0, 7}}, 7), notyet = may({{2, 5}}, {{0, 7}}, 7);
            const bool pal = may({{2, 9}}, {{3, 3}}, 7);
            F(0).rec[0x74] = static_cast<uint8_t>(r74);
            snprintf(w, sizeof w, "Turn: a human fighter 9 once a cleric of 7 %s, a fighter 5 not; once a paladin of 3 %s",
                     sk ? "yes (skill levels)" : "no (levels)", sk ? "yes" : "no");
            expect(P, w, passed == sk && !notyet && pal == sk, "passed %d not yet %d paladin %d", passed, notyet, pal);
        }
        return;
    }
    const bool pal3 = may({{3, 3}}, {}, 0);
    // a human fighter 9 who was a cleric of 7 (passed), and one of 5 (not yet)
    const bool past = may({{2, 9}}, {{0, 7}}, 7), notyet = may({{2, 5}}, {{0, 7}}, 7);
    snprintf(w, sizeof w, "Turn offered (rule %d): a cleric 1 yes, a fighter no, a former cleric passed %s / not yet no, a paladin of 3 %s", rule,
             game() == 1 ? "(Pool: no former classes)" : "yes", rule == 1 ? "yes" : "no");
    const bool cleric = may({{0, 1}}, {}, 0), fighter = may({{2, 9}}, {}, 0);
    expect(P, w, cleric && !fighter && (game() == 1 || past) && !notyet && pal3 == (rule == 1),
           "cleric %d fighter %d passed %d not yet %d paladin %d", cleric, fighter, past, notyet, pal3);
    if (rule == 2) {
        levels(0, {{0, 5}}, {}, 0);
        play::fg->b.undead_loaded = 2;
        F(0).undead_turns = 2;
        expect(P, "Champions: no Turn once turned as many times as undead were loaded", !combat::can_turn(play::fg->b, play::fg->t, F(0)));
        F(0).undead_turns = 1;
        expect(P, "Champions: a second Turn with two undead loaded", combat::can_turn(play::fg->b, play::fg->t, F(0)));
        return;
    }
    if (rule != 0 || game() == 1) return;
    // The former cleric's column: Curse's 1-8 column is the cleric's own
    // level's (0 - the original's), the skill level's with the Game Fixes
    for (bool fx : {false, true}) {
        force_fix = fx;
        int bad = 0, n = 0;
        for (int type = 1; type <= 6 && type <= 12; ++type)
            for (int r20 = 1; r20 <= 20; ++r20) {
                if (room < 1) break;
                const Case c{{{2, 9}}, {{0, 7}}, 7, 0, 1, r20, 1, {type}};
                ++n;
                if (engine_turn(c) != ref(c, 0, fx)) ++bad;
            }
        snprintf(w, sizeof w, "a former cleric of 7 (fighter 9) turns at %s", fx ? "column 7 (Game Fixes)" : "column 0 (the original's)");
        expect(P, w, bad == 0 && n > 0, "%d of %d wrong", bad, n);
    }
    force_fix = -1;
    rules::set_fixes(true);
}

// ---- R: Restoration and training with levels lost (coab's cast_restore,
// the training routine's share - ovr018 / ovr023)
void put32(uint8_t* r, int o, uint32_t v)
{
    for (int i = 0; i < 4; ++i) r[o + i] = static_cast<uint8_t>(v >> (8 * i));
}
void restore()
{
    if (game() == 4 || game() == 9 || game() >= 6) return;     // (Darkness's own Restoration: behave darkness drain)
    static party::Character c;
    int bad = 0, n = 0;
    std::string first;
    for (int cls : {0, 2, 5, 6})
        for (int L : {2, 4, 7})
            for (int lost : {1, 2, 3}) {
                if (!m->tables.u8(static_cast<uint16_t>(m->tables.lay.class_masks + cls))) continue;
                // (past the game's last level - Pool's 6th / 7th: no class qualifies in coab's loop)
                if (m->tables.exp_needed(cls, L) <= 0) continue;
                c = party::Character{};
                c.rec[0x74] = 7;
                c.rec[0x75] = static_cast<uint8_t>(cls);
                for (int s = 0; s < 6; ++s) c.rec[0x10 + 2 * s] = c.rec[0x11 + 2 * s] = 16;
                c.rec[0x109 + cls] = static_cast<uint8_t>(L);
                c.rec[0xE5] = static_cast<uint8_t>(L);
                c.rec[0x78] = 30;
                c.rec[0x1A4] = 22;
                c.rec[0x12C] = 28;
                c.rec[0xE7] = static_cast<uint8_t>(lost);
                c.rec[0xE8] = static_cast<uint8_t>(5 * lost + 2);
                put32(c.rec, 0x127, 10);
                const int share = (5 * lost + 2) / lost;
                const int32_t need = m->tables.exp_needed(cls, L, &c);     // (the character's own row - Krynn's by race)
                create::restore(c, m->tables);
                ++n;
                const bool ok = c.hp_max() == 30 + share && c.hp() == 22 + share && c.rec[0x12C] == 28 + share && c.rec[0xE7] == lost - 1 &&
                                c.rec[0xE8] == 5 * lost + 2 - share && c.level(cls) == L + 1 && static_cast<int32_t>(c.exp()) == need;
                if (!ok && !bad++) {
                    snprintf(w, sizeof w, "class %d level %d lost %d: HP %d/%d rolled %d lost %d/%d level %d xp %u (want +%d, level %d, xp %d)",
                             cls, L, lost, c.hp(), c.hp_max(), c.rec[0x12C], c.rec[0xE7], c.rec[0xE8], c.level(cls), c.exp(), share, L + 1,
                             need);
                    first = w;
                }
            }
    expect(P, "Restoration: the lost hit points' share back (max, now, rolled), a level up, experience raised to it (coab cast_restore)",
           bad == 0 && n > 0, "%d of %d wrong: %s", bad, n, first.c_str());
    // Two classes: the lower level rises (a dwarf fighter 4 / thief 3)
    c = party::Character{};
    c.rec[0x74] = 7;
    c.rec[0x109 + 2] = 4;
    c.rec[0x109 + 6] = 3;
    c.rec[0xE7] = 1;
    c.rec[0xE8] = 4;
    c.rec[0x78] = 20;
    create::restore(c, m->tables);
    const bool lower = m->tables.exp_needed(6, 3) < m->tables.exp_needed(2, 4);
    expect(P, "Restoration: of two classes the lower one rises (when its experience is the smaller too)", !lower || (c.level(6) == 4 && c.level(2) == 4),
           "F %d T %d", c.level(2), c.level(6));
    // Training: each class trained takes one lost level and its share off what is owed
    c = party::Character{};
    c.rec[0x74] = 7;
    c.rec[0x75] = 2;
    for (int s = 0; s < 6; ++s) c.rec[0x10 + 2 * s] = c.rec[0x11 + 2 * s] = 16;
    c.rec[0x109 + 2] = 3;
    c.rec[0xE5] = 3;
    c.rec[0x78] = c.rec[0x1A4] = 20;
    c.rec[0xE7] = 3;
    c.rec[0xE8] = 10;
    put32(c.rec, 0x127, 0x3FFFFFF);
    const int bit = m->tables.u8(static_cast<uint16_t>(m->tables.lay.class_masks + 2));
    create::train_classes(c, m->tables, m->facts, m->dice, bit, false);
    if (game() == 1) return;        // (Pool's training: behave pool)
    expect(P, "training with 3 levels lost (10 hit points owed): 2 levels and 7 owed after (coab's training routine)",
           c.rec[0xE7] == 2 && c.rec[0xE8] == 7, "%d levels, %d owed", c.rec[0xE7], c.rec[0xE8]);
}

// ---- P: poison and disease over a rest (the engine's rest clock -
// effects_clock in 5-minute steps, as Rest runs it), the facts' handlers
void add_aff(party::Character& c, int type, int minutes, int data)
{
    uint8_t* a = c.affects[c.n_affects++];
    memset(a, 0, party::kAffectSize);
    a[0] = static_cast<uint8_t>(type);
    a[1] = static_cast<uint8_t>(minutes);
    a[2] = static_cast<uint8_t>(minutes >> 8);
    a[3] = static_cast<uint8_t>(data);
}
void rest_minutes(int mins)
{
    for (int k = 0; k < mins / 5; ++k) {
        vm->advance_clock(1, 5);
        vm->take_minutes();
        effects_clock(5, C);
    }
    note_until = 0;
}
void rest()
{
    const auto& cu = d->prof->cures;
    if (!cu.slow_poison || !cu.poisoned || !cu.poison_damage) {
        printf("  (rules4: no Slow Poison facts in this game's profile - rest passed over)\n");
        return;
    }
    if (!monkey_boot()) return;
    party::Character& c = pt->m[0];
    auto reset = [&](int hp) {
        c.n_affects = 0;
        c.rec[0x195] = party::Okay;
        c.rec[0x196] = 1;
        c.rec[0x78] = 30;
        c.rec[0x1A4] = static_cast<uint8_t>(hp);
    };
    // Slow-poisoned (poisoned, Slow Poison 60 minutes, poison damage every 10): 1 HP a 10 minutes, death when Slow Poison runs out
    reset(20);
    add_aff(c, cu.poisoned, 0, 0xFF);
    add_aff(c, cu.slow_poison, 60, 0xFF);
    add_aff(c, cu.poison_damage, 10, 0xFF);
    rest_minutes(50);
    const int hp50 = c.hp();
    const bool alive50 = c.rec[0x195] == party::Okay;
    rest_minutes(20);
    expect(P, "slow-poisoned over a rest: 1 hit point every 10 minutes (50 minutes: 20 -> 15), dead when Slow Poison runs out (60), poison damage gone",
           hp50 == 15 && alive50 && c.rec[0x195] == party::Dead && !c.has_affect(cu.poison_damage) && !c.has_affect(cu.slow_poison),
           "HP at 50 %d alive %d, status after %d, damage effect %d", hp50, alive50, c.rec[0x195], c.has_affect(cu.poison_damage));
    // Poison damage never below 1 HP
    reset(3);
    add_aff(c, cu.poisoned, 0, 0xFF);
    add_aff(c, cu.slow_poison, 600, 0xFF);
    add_aff(c, cu.poison_damage, 10, 0xFF);
    rest_minutes(120);
    expect(P, "poison damage stops at 1 hit point (HP 3, 2 hours)", c.hp() == 1 && c.rec[0x195] == party::Okay, "HP %d status %d", c.hp(),
           c.rec[0x195]);
    // Disease: the sickness (every 10 minutes 1 HP, down to 1) and the weakness (every 60, Strength in use - 1 down to 3)
    if (cu.weak && cu.sick) {
        reset(25);
        c.rec[0x11] = 16;
        add_aff(c, cu.weak, 60, 0);
        add_aff(c, cu.sick, 10, 0);
        rest_minutes(120);
        expect(P, "diseased over 2 hours of rest: 12 hit points (1 every 10 minutes) and 2 Strength (1 every 60) lost",
               c.hp() == 13 && c.rec[0x11] == 14 && c.has_affect(cu.weak) && c.has_affect(cu.sick), "HP %d Str %d", c.hp(), c.rec[0x11]);
    }
    reset(c.hp_max());
    c.n_affects = 0;
}

// ---- H: Haste ages its fighter a year, once (coab AffectHaste: the
// effect's data + 0x10 marks it)
void haste()
{
    const combat::Facts* fx = d->prof ? &d->prof->fight.facts : nullptr;
    if (!fx || !fx->haste || !fight()) return;
    mx::put(0, 0, 10, 10, 50);
    combat::Fighter& f = F(0);
    f.rec[0x76] = 30;
    f.rec[0x77] = 0;
    mx::add_aff(0, fx->haste, 0);
    int ages = 0;
    for (int r = 0; r < 3; ++r) {
        combat::Event ev[64];
        const int n = combat::start_round(play::fg->b, play::rng, ev, 64);
        for (int k = 0; k < n; ++k) ages += ev[k].ev == combat::Ev::Ages && ev[k].who == 0;
    }
    const int age = f.rec[0x76] | f.rec[0x77] << 8;
    expect(P, "Haste: a year older once over three rounds (\"ages\" once)", age == 31 && ages == 1, "age %d, said %d times", age, ages);
}

// ---- M: the Krynn moons (Champions / Death Knights): a mage's caster
// level (new -1, full +1 past 5th) and saves (new -1, full +1)
// The Dark Queen's moon clock (DQK.EXE 0x5D6E, from its clock's carry
// routine 0x60C8 once for each day - each hours carry): for the moons
// white, red, black (limits 8, 1, 6) its counter (game byte 0x1FD + m) at
// the limit or more: the phase (0x1FA + m) on (3 -> 0), the counter 0; else
// the counter + 1. The phases are what its mages' caster levels and saves
// read (0x1F9 + robe); a fight's mage has its robe (fight setup)
void queen_moons()
{
    uint32_t seed = 99;
    auto rnd = [&](int n) { seed = seed * 1103515245u + 12345u; return static_cast<int>((seed >> 16) % n); };
    int bad = 0, n = 0;
    char first[200] = "";
    static const int kLimit[3] = {8, 1, 6};
    for (int k = 0; k < 300; ++k) {
        int ph[3], ctr[3];
        for (int m2 = 0; m2 < 3; ++m2) {
            ph[m2] = rnd(4);
            ctr[m2] = rnd(kLimit[m2] + 2);
            vm->set(static_cast<uint16_t>(0x1FA + m2), static_cast<uint16_t>(ph[m2]));
            vm->set(static_cast<uint16_t>(0x1FD + m2), static_cast<uint16_t>(ctr[m2]));
        }
        // a time: hours now + hours added (whole days carried)
        const int h0 = rnd(24), add = rnd(80);
        vm->set(0x4BC9, static_cast<uint16_t>(h0));
        const int days = (h0 + add) / 24;
        vm->advance_clock(3, add);
        for (int dd = 0; dd < days; ++dd)
            for (int m2 = 0; m2 < 3; ++m2) {
                if (ctr[m2] >= kLimit[m2]) {
                    ph[m2] = ph[m2] >= 3 ? 0 : ph[m2] + 1;
                    ctr[m2] = 0;
                } else {
                    ++ctr[m2];
                }
            }
        int out[3];
        q_moons(out);
        for (int m2 = 0; m2 < 3; ++m2) {
            ++n;
            const int gp = vm->get(static_cast<uint16_t>(0x1FA + m2)), gc = vm->get(static_cast<uint16_t>(0x1FD + m2));
            if (gp != ph[m2] || gc != ctr[m2] || out[m2] != (ph[m2] & 3) || magic::moon(m2) != (ph[m2] & 3)) {
                ++bad;
                if (!*first)
                    snprintf(first, sizeof first, "moon %d, hour %d + %d: phase %d counter %d, want %d %d (read %d)", m2, h0, add,
                             gp, gc, ph[m2], ctr[m2], out[m2]);
            }
        }
    }
    // a fight's mages carry their robes (their saves' and caster levels' moon)
    if (fight()) {
        combat::Battle& b = fg->b;
        for (int i = 0; i < b.party_size; ++i) {
            const combat::Fighter& f = b.f[i];
            if (f.member < 0) continue;
            const party::Character& ch = pt->m[f.member];
            const int want = ch.level(classes::MagicUser) > 0 ? ch.ext[party::kExtRobe] : 0;
            ++n;
            if (f.robe != want) {
                ++bad;
                if (!*first) snprintf(first, sizeof first, "member %d: robe %d in the fight, want %d", f.member, f.robe, want);
            }
        }
    }
    expect(P, "the Dark Queen's moons: a day steps them (DQK.EXE 0x5D6E: limits 8 1 6), read from its game block, a fight's mages' robes",
           bad == 0, "%d of %d wrong (%s)", bad, n, first);
}

// Barkskin (0x0D) at a save: the lowest d20 that saves one point lower
// (Secret / Darkness / Treasures GAME.OVR handlers, the Dark Queen's
// DQK.EXE 0x4E0AD in its save event 0xC; round 22)
void barkskin_saves()
{
    if (!fight() || !fg->b.fx || !(fg->b.fx->items.barkskin || fg->b.fx->items.barkskin_k)) return;
    const uint8_t eff = fg->b.fx->items.barkskin ? fg->b.fx->items.barkskin : fg->b.fx->items.barkskin_k;
    mx::put(0, 0, 10, 10, 50);
    combat::Fighter& f = F(0);
    auto lowest = [&] {
        int lo = 0;
        for (int r = 1; r <= 20 && !lo; ++r) {
            dice(0, r);
            if (combat::saving_throw(f, 4, 0, play::rng)) lo = r;
            dice_off();
        }
        return lo;
    };
    const int base = lowest();
    mx::add_aff(0, eff);
    const int bark = lowest();
    expect(P, "Barkskin (0x0D): a save 1 easier (not against magic)", base > 2 && bark == base - 1, "from %d, with it %d",
           base, bark);
}

// Secret: a feebleminded one (0x44) casts nothing - the player's cast
// (GAME.OVR 0x2DB00) and the computer's chooser (0xD4F0) - round 22
void secret_feeble()
{
    if (game() != 3 || !fight() || !fg->b.fx || !fg->b.fx->feeble) return;
    mx::put(0, 0, 10, 10, 50);
    combat::start_round(fg->b, play::rng);
    const bool before = F(0).can_cast;
    mx::add_aff(0, fg->b.fx->feeble);
    combat::start_round(fg->b, play::rng);
    expect(P, "Secret: a feebleminded one casts nothing (GAME.OVR 0x2DB00, the AI's 0xD4F0)", before && !F(0).can_cast,
           "before %d, feebleminded %d", before, F(0).can_cast);
}

// The Dark Queen's Enchanted Kapak (DQK.EXE 0x50BC8, 0x83A2, the try
// 0x2E248): its acid on its square and each round it on a d2 of 1, ground
// 0x46, 1d6 no save on one standing there; under 7 Hit Dice and not driven
// the computer won't step in - round 22
void queen_acid()
{
    if (game() != 7 || !fight() || !fg->b.fx || !fg->b.fx->acid_spread) return;
    combat::Battle& b = fg->b;
    mx::put(0, 0, 10, 10, 60);
    mx::put(B, 1, 20, 10, 60);
    for (int y = 8; y <= 12; ++y)
        for (int x = 18; x <= 22; ++x) b.ground[y][x] = 0x17;
    b.n_clouds = 0;
    int laid = 0, want = 0;
    for (int seed = 1; seed <= 20; ++seed) {
        b.n_clouds = 0;
        for (int y = 8; y <= 12; ++y)
            for (int x = 18; x <= 22; ++x) b.ground[y][x] = 0x17;
        // the d2s as rolled: 9 of them, the centre always laid
        int rolls[9];
        create::Dice dd(static_cast<uint32_t>(seed));
        create::Dice copy = dd;
        for (int k = 0; k < 9; ++k) rolls[k] = copy.roll(2, 1);
        int expect_n = 0;
        for (int k = 0; k < 9; ++k) expect_n += (rolls[k] == 1 || k == 4);
        // (1d6 rounds are rolled between, so count only)
        const int n = combat::lay_acid_spread(b, fg->t, 20, 10, dd);
        int on = 0;
        for (int y = 9; y <= 11; ++y)
            for (int x = 19; x <= 21; ++x) on += b.ground[y][x] == 0x46;
        laid += n == on && on >= 1 && b.ground[10][20] == 0x46;
        want += 1;
        (void)expect_n;
    }
    b.f[0].x = 20;
    b.f[0].y = 10;
    const bool burns = combat::on_acid(b, 0);
    // the step: a weak one, not driven, refused; 7 Hit Dice taken
    combat::Fighter& m = F(B);
    m.driven = false;
    m.rec[0xE5] = 3;
    const bool weak = combat::cloud_ok(b, fg->t, m, 20, 10, play::rng);
    m.rec[0xE5] = 7;
    const bool strong = combat::cloud_ok(b, fg->t, m, 20, 10, play::rng);
    expect(P, "the Dark Queen's kapak acid: its square always, ground 0x46, burns who stands there; under 7 Hit Dice no step in",
           laid == want && burns && !weak && strong, "%d of %d lays, burns %d, weak %d, strong %d", laid, want, burns, weak, strong);
}

// The Dark Queen's Bozak (0x9A; DQK.EXE 0x509AE queues it, 0x97A8
// "explodes."): 1d6 to each within a square, no save, no stun - round 22
void queen_bozak()
{
    if (game() != 7 || !fight() || !fg->b.fx || !fg->b.fx->bozak_1d6) return;
    combat::Battle& b = fg->b;
    mx::put(0, 0, 10, 10, 90);
    mx::put(B, 1, 11, 10, 90);
    int ok = 0, n = 0;
    for (int seed = 1; seed <= 30; ++seed) {
        mx::put(0, 0, 10, 10, 90);
        create::Dice dd(static_cast<uint32_t>(seed));
        int who[combat::kMaxFighters], dmg[combat::kMaxFighters];
        bool dn[combat::kMaxFighters], st[combat::kMaxFighters];
        const int k = combat::bozak_burst(b, fg->t, B, dd, who, dmg, dn, st, combat::kMaxFighters);
        ++n;
        ok += k == 1 && who[0] == 0 && dmg[0] >= 1 && dmg[0] <= 6 && !st[0];
    }
    expect(P, "the Dark Queen's Bozak: 1d6 to the one beside it, no save, no stun (DQK.EXE 0x97A8)", ok == n, "%d of %d", ok, n);
}

// The Dark Queen's Aurak (DQK.EXE handlers 0x5070D, 0x50CF0, 0x50DF1,
// 0x50F52, 0x50FFC, 0x5100F, 0x512D1): goes mad, reborn, energy form, blast
void queen_aurak()
{
    if (game() != 7 || !fight() || !fg->b.fx || !fg->b.fx->q.mad) return;
    combat::Battle& b = fg->b;
    const combat::Facts::QueenFx& q = b.fx->q;
    auto aff_of = [&](int i, uint8_t t) -> const uint8_t* {
        combat::Fighter& f = F(i);
        for (int k = 0; f.n_aff && k < *f.n_aff; ++k)
            if (f.aff[k][0] == t) return f.aff[k];
        return nullptr;
    };
    auto mins = [&](const uint8_t* a) { return a ? (a[1] | a[2] << 8) : -1; };
    auto scene = [&]() {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(0, 0, 10, 10, 200);
        mx::put(B, 1, 11, 10, 40);
        mx::add_aff(B, q.mad);
    };
    int bad = 0, n = 0;
    auto chk = [&](bool ok, const char* what) {
        ++n;
        if (!ok && !bad++) printf("  (aurak: %s)\n", what);
    };
    for (int seed = 1; seed <= 20; ++seed) {
        create::Dice dd(static_cast<uint32_t>(seed));
        scene();
        combat::Fighter& au = F(B);
        // 1. its death: up with 20, mad
        combat::damage(b, B, 200);
        chk(au.up() && au.hp() == 20 && au.rise_how == 1, "up with 20 hit points, goes mad");
        chk(!au.has(q.mad) && au.has(q.reborn) && au.has(q.immolate) && au.has(q.plus2), "0x98 gone; 0xA0 0xA1 0xA2 held");
        const uint8_t* mt = aff_of(B, q.mad_time);
        chk(mt && mins(mt) == 6 && mt[4] == 1, "0xA3: 6 rounds, called as it ends");
        au.rise_how = 0;
        // 2. its turn: immolates the foe beside it, 1d6, no save
        int who[combat::kMaxFighters], dmg[combat::kMaxFighters], word = 0;
        bool dn[combat::kMaxFighters], ends = false;
        int k = combat::aurak_turn(b, fg->t, B, dd, who, dmg, dn, combat::kMaxFighters, &word, &ends);
        chk(word == 1 && k == 1 && who[0] == 0 && dmg[0] >= 1 && dmg[0] <= 6 && !ends, "immolates: 1d6 to the foe beside it");
        // 3. killed again: reborn - no harm, zaps, the blast in 3 (called: its 0xA3's removal)
        combat::damage(b, B, 200);
        chk(au.up() && au.hp() == 20 && au.rise_how == 2, "rises in a new form with 20");
        chk(!au.has(q.reborn) && !au.has(q.immolate) && !au.has(q.plus2) && !au.has(q.mad_time), "0xA0 0xA1 0xA2 0xA3 gone");
        const uint8_t* bu = aff_of(B, q.blows_up);
        chk(au.has(q.no_harm) && au.has(q.zaps) && bu && mins(bu) == 3, "0xA5 0xA7, 0xA6 for 3");
        bool called = false;
        for (int j = 0; au.n_aff && j < *au.n_aff; ++j)
            if (au.aff[j][0] == q.blows_up && au.aff[j][4]) called = true;
        chk(called, "an 0xA6 called as it ends");
        au.rise_how = 0;
        k = combat::aurak_turn(b, fg->t, B, dd, who, dmg, dn, combat::kMaxFighters, &word, &ends);
        chk(word == 2 && k == 1 && who[0] == 0 && dmg[0] >= 2 && dmg[0] <= 12 && ends, "zaps the foe beside it, 2d6, the turn over");
        // 4. no harm from a spell
        combat::Harm h;
        h.spell = 1;
        bool down = false;
        chk(combat::harm(b, B, 15, h, dd, &down) == 0 && au.hp() == 20, "a spell's damage 0 in the energy form");
        // 5. its time over: explodes, gone; the blast 3d6 to the one beside it
        bool boom = false;
        for (int r = 0; r < 4 && !boom; ++r) {
            combat::Event ev[8];
            const int ne = combat::tick(b, ev, 8);
            for (int e = 0; e < ne; ++e) boom |= ev[e].ev == combat::Ev::AurakExplodes && ev[e].who == B;
        }
        chk(boom && au.status() == party::Gone, "explodes after 3 rounds, gone");
        bool st[combat::kMaxFighters];
        const int hp0 = F(0).hp();
        k = combat::aurak_blast(b, fg->t, B, dd, who, dmg, dn, st, combat::kMaxFighters);
        chk(k == 1 && who[0] == 0 && dmg[0] >= 3 && dmg[0] <= 18 && F(0).hp() == hp0 - dmg[0], "the blast: 3d6, no save");
        // 6. its madness run out (not killed): the energy form, no zaps, turns over
        scene();
        combat::damage(b, B, 200);
        for (int r = 0; r < 6; ++r) combat::tick(b, nullptr, 0);
        const uint8_t* b2 = aff_of(B, q.blows_up);
        chk(au.has(q.no_harm) && au.has(q.turn_over) && b2 && b2[4] && !au.has(q.immolate) && !au.has(q.zaps) &&
                !au.has(q.reborn),
            "madness over: 0xA5, 0xA6 called, 0xD5; no 0xA1 / 0xA7 / 0xA0");
        au.rise_how = 0;
    }
    expect(P, "the Dark Queen's Aurak: goes mad (20 hp, immolates 1d6), reborn (no harm, zaps 2d6, the turn over), explodes (3d6) - DQK.EXE 0x5070D on",
           bad == 0, "%d of %d wrong", bad, n);
}

// The Dark Queen's mummy (DQK.EXE 0x5145A, 0x5156B; the zombie's 0x50CE3)
void queen_mummy()
{
    if (game() != 7 || !fight() || !fg->b.fx || !fg->b.fx->q.fear_aura) return;
    combat::Battle& b = fg->b;
    const combat::Facts& fx = *b.fx;
    int bad = 0, n = 0, para = 0, saved = 0;
    auto chk = [&](bool ok, const char* what) {
        ++n;
        if (!ok && !bad++) printf("  (mummy: %s)\n", what);
    };
    for (int seed = 1; seed <= 40; ++seed) {
        create::Dice dd(static_cast<uint32_t>(seed));
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(0, 0, 10, 10, 60);
        mx::put(1, 0, 10, 12, 60);
        mx::put(B, 1, 12, 10, 60);
        mx::add_aff(B, fx.q.fear_aura);
        mx::add_aff(1, fx.mon.no_paralyze);
        combat::Event ev[8];
        const int k = combat::queen_fear(b, dd, ev, 8);
        chk(!F(B).has(fx.q.fear_aura), "0xAB taken off");
        chk(k == 1 && ev[0].who == 0, "only the one without 0x9E");
        if (k == 1 && ev[0].ev == combat::Ev::FearParalyzed) {
            ++para;
            int m = -1;
            for (int j = 0; F(0).n_aff && j < *F(0).n_aff; ++j)
                if (F(0).aff[j][0] == fx.mon.paralyzed) m = F(0).aff[j][1];
            chk(m >= 1 && m <= 4, "paralyzed 1d4 rounds");
        } else if (k == 1) {
            ++saved;
            chk(ev[0].ev == combat::Ev::Unaffected && !F(0).has(fx.mon.paralyzed), "made: unaffected");
        }
        chk(combat::queen_fear(b, dd, ev, 8) == 0, "once a fight");
    }
    chk(para > 0 && saved > 0, "some feared, some not");
    // magic fire a point more a die; others not
    {
        create::Dice dd(7);
        mx::put(B, 1, 12, 10, 100);
        mx::add_aff(B, fx.q.fire_dice);
        combat::Harm h;
        h.kind = 9;
        h.dice = 3;
        h.spell = 1;
        bool down = false;
        chk(combat::harm(b, B, 10, h, dd, &down) == 13, "magic fire 10 + 3 dice = 13");
        mx::put(B, 1, 12, 10, 100);
        mx::add_aff(B, fx.q.fire_dice);
        h.kind = 1;
        chk(combat::harm(b, B, 10, h, dd, &down) == 10, "plain fire 10");
    }
    // the rot's clock 0xD7: no healing by rules::heal; a point at each 10-day
    // renewal (0x52D77); its Cure Disease takes 0xD7 and the rot, and 0xB9
    if (pt && pt->count > 0) {
        party::Character ch = pt->m[0];
        ch.rec[0x195] = party::Okay;
        ch.rec[0x1A4] = 5;
        ch.n_affects = 0;
        auto put = [&](uint8_t t, int mn, uint8_t data) {
            uint8_t* a = ch.affects[ch.n_affects++];
            memset(a, 0, party::kAffectSize);
            a[0] = t;
            a[1] = static_cast<uint8_t>(mn);
            a[2] = static_cast<uint8_t>(mn >> 8);
            a[3] = data;
            a[4] = 1;
        };
        put(fx.mon.rot_clock, 14400, 0xFF);
        put(fx.mon.rot, 43200, (3 << 5) + 0x1F);
        rules::set_no_heal(fx.q.no_heal);
        chk(!rules::heal(ch, 4) && ch.hp() == 5, "no healing with 0xD7");
        rules::rot_clock(ch, 14400, fx.mon.rot, fx.mon.rot_clock, fx.q.rot_shift);
        chk(ch.hp() == 6, "a point at the clock's 10 days");
        const auto& mf = d->prof->magic.facts;
        bool d7 = false, ac = false, b9 = false;
        for (int i = 0; i < 4; ++i) {
            if (mf.disease[i] == fx.mon.rot_clock) d7 = mf.disease_with[i][0] == fx.mon.rot;
            if (mf.disease[i] == fx.q.spore_ill) b9 = true;
        }
        for (uint8_t t : d->prof->cures.disease) ac |= t == fx.mon.rot;
        chk(d7 && b9 && ac, "Cure Disease: 0xD7 with the rot, 0xB9; the temple's too");
    }
    expect(P, "the Dark Queen's mummy: fear at the set-up (save, 1d4 paralysis, 0x9E immune, once), magic fire + a die, the rot's no healing / 10-day point / cure - DQK.EXE 0x5156B / 0x5145A / 0x52D77 / 0x56480",
           bad == 0, "%d of %d wrong (%d feared, %d saved)", bad, n, para, saved);
}

// The Dark Queen's Black Pudding (DQK.EXE 0x51CC6)
void queen_pudding()
{
    if (game() != 7 || !fight() || !fg->b.fx || !fg->b.fx->q.dissolve || !names || !pt) return;
    combat::Battle& b = fg->b;
    int arm = -1, plain = -1;
    for (int t = 1; t < 0x80 && (arm < 0 || plain < 0); ++t) {
        const int sl = names->type(t).slot;
        if (sl == items::kSlotArmour && t != 0x23 && t != 0x24 && t != 0x26 && arm < 0) arm = t;
        if (sl == items::kSlotWeapon && plain < 0) plain = t;
    }
    if (arm < 0) {
        expect(P, "the Dark Queen's Black Pudding: an armour type", false, "none");
        return;
    }
    party::Character& ch = pt->m[F(0).member >= 0 ? F(0).member : 0];
    const party::Character keep = ch;
    auto blow = [&](int plus, int* left, int* items_after) {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(0, 0, 10, 10, 200);
        mx::put(B, 1, 11, 10, 200);
        combat::Fighter& tg = F(0);
        memset(ch.items[0], 0, items::kRecordSize);
        ch.items[0][0x2E] = static_cast<uint8_t>(arm);
        ch.items[0][0x34] = 1;
        ch.items[0][0x32] = static_cast<uint8_t>(plus);
        memset(ch.items[1], 0, items::kRecordSize);
        ch.items[1][0x2E] = static_cast<uint8_t>(plain);
        ch.items[1][0x34] = 1;
        ch.n_items = 2;
        tg.items = ch.items;
        tg.n_items = 2;
        combat::Fighter& at = F(B);
        mx::add_aff(B, b.fx->q.dissolve);
        at.rec[0x199] = 60;
        at.attacks[0] = 1;
        at.attacks[1] = 0;
        at.rec[0x19E] = 1;
        at.rec[0x1A0] = 8;
        at.rec[0x1A2] = 3;
        create::g_die_hook = [](int nn) { return nn == 20 ? 18 : nn == 8 ? 8 : -1; };
        const combat::Attack r = combat::attack(b, B, 0, names, rng);
        create::g_die_hook = nullptr;
        int ev = -1;
        for (int j = 0; j < r.n_ev; ++j)
            if (r.ev[j].ev == combat::Ev::Dissolving || r.ev[j].ev == combat::Ev::Dissolves) ev = r.ev[j].ev == combat::Ev::Dissolves ? 1 : 0;
        *left = static_cast<int8_t>(ch.items[0][0x32]);
        *items_after = ch.items[0][0x34];
        return r.n > 0 && r.hits[0].hit ? r.hits[0].damage * 10 + ev + 1 : -100;
    };
    int l1, r1, l2, r2;
    const int a = blow(2, &l1, &r1), z = blow(0, &l2, &r2);
    ch = keep;
    expect(P, "the Dark Queen's Black Pudding: a hit on armour +2 - +1, \"is Disolving\", no damage; on +0 - \"Dissolves\", taken off",
           a == 1 && l1 == 1 && r1 == 1 && z == 2 && r2 == 0, "+2: %d (plus %d, worn %d); +0: %d (worn %d)", a, l1, r1, z, r2);
}

// The Dark Queen's Gas Spore (DQK.EXE 0x51A6A, 0x51C06)
void queen_spore()
{
    if (game() != 7 || !fight() || !fg->b.fx || !fg->b.fx->q.spore) return;
    combat::Battle& b = fg->b;
    const auto& q = b.fx->q;
    int bad = 0, n = 0, halves = 0;
    auto chk = [&](bool ok, const char* what) {
        ++n;
        if (!ok && !bad++) printf("  (spore: %s)\n", what);
    };
    for (int seed = 1; seed <= 30; ++seed) {
        create::Dice dd(static_cast<uint32_t>(seed));
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(0, 0, 10, 10, 200);
        mx::put(1, 0, 12, 10, 200);
        mx::put(2, 0, 13, 10, 200);
        mx::put(3, 0, 14, 10, 200);       // (3 away: out of it)
        mx::put(B, 1, 11, 10, 20);
        mx::add_aff(1, q.spore_proof);
        int who[combat::kMaxFighters], dmg[combat::kMaxFighters];
        bool dn[combat::kMaxFighters];
        const int k = combat::spore_blast(b, fg->t, B, dd, who, dmg, dn, combat::kMaxFighters);
        bool z = false, two = false, one = false;
        for (int j = 0; j < k; ++j) {
            if (who[j] == 0) z = dmg[j] >= 3 && dmg[j] <= 36;
            if (who[j] == 2) two = dmg[j] >= 3 && dmg[j] <= 36;
            if (who[j] == 1) one = true;
        }
        chk(k == 2 && z && two && !one, "6d6 (or half) to the two within 2; not the 0xB7 holder");
        for (int j = 0; j < k; ++j) halves += dmg[j] <= 18;
    }
    // its touch: the one hit diseased (a day), the spore spent and dead
    {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(0, 0, 10, 10, 200);
        mx::put(B, 1, 11, 10, 20);
        mx::add_aff(B, q.spore_touch);
        combat::Fighter& at = F(B);
        at.rec[0x199] = 60;
        at.attacks[0] = 1;
        at.attacks[1] = 0;
        at.rec[0x19E] = 1;
        at.rec[0x1A0] = 4;
        at.rec[0x1A2] = 0;
        create::g_die_hook = [](int nn) { return nn == 20 ? 18 : -1; };
        const combat::Attack r = combat::attack(b, B, 0, names, rng);
        create::g_die_hook = nullptr;
        int ill = -1;
        for (int j = 0; F(0).n_aff && j < *F(0).n_aff; ++j)
            if (F(0).aff[j][0] == q.spore_ill) ill = F(0).aff[j][1] | F(0).aff[j][2] << 8;
        chk(r.n > 0 && r.hits[0].hit && ill == 1440, "the one hit: 0xB9 for 1440");
        chk(!at.up() && at.has(q.spent), "the spore spent and down");
    }
    expect(P, "the Dark Queen's Gas Spore: its death 6d6 within 2 (a save for half, 0xB7 holders none), its touch a day's disease and its end - DQK.EXE 0x51A6A / 0x51C06",
           bad == 0, "%d of %d wrong (%d halved or low)", bad, n, halves);
}

// The Dark Queen's shambling mound (DQK.EXE 0x516D2 - 0x518DD)
void queen_mound()
{
    if (game() != 7 || !fight() || !fg->b.fx || !fg->b.fx->q.blow_half) return;
    combat::Battle& b = fg->b;
    const auto& q = b.fx->q;
    const auto& m = b.fx->mon;
    int bad = 0, n = 0;
    auto chk = [&](bool ok, const char* what) {
        ++n;
        if (!ok && !bad++) printf("  (mound: %s)\n", what);
    };
    auto scene = [&]() {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(0, 0, 10, 10, 200);
        mx::put(B, 1, 11, 10, 100);
        for (uint8_t t : {m.absorb_elec, q.fire_cold, q.blow_half, m.engulf}) mx::add_aff(B, t);
    };
    // a blow at it: half (8 + 3 = 11 -> 5)
    scene();
    {
        combat::Fighter& at = F(0);
        at.rec[0x199] = 60;
        at.attacks[0] = 1;
        at.attacks[1] = 0;
        at.rec[0x19E] = 1;
        at.rec[0x1A0] = 8;
        at.rec[0x1A2] = 3;
        create::g_die_hook = [](int nn) { return nn == 20 ? 18 : nn == 8 ? 8 : -1; };
        const combat::Attack r = combat::attack(b, 0, B, nullptr, rng);
        create::g_die_hook = nullptr;
        chk(r.n > 0 && r.hits[0].hit && r.hits[0].damage == 5, "a blow 11 -> 5");
    }
    // electricity: + 8, no harm; fire: half or none on a save
    {
        create::Dice dd(3);
        scene();
        combat::Harm h;
        h.kind = 4;
        h.spell = 1;
        bool down = false;
        chk(combat::harm(b, B, 20, h, dd, &down) == 0 && F(B).hp() == 108, "electricity: + 8");
        int half = 0, none = 0;
        for (int sd = 1; sd <= 30; ++sd) {
            create::Dice d2(static_cast<uint32_t>(sd));
            scene();
            combat::Harm f;
            f.kind = 1;
            f.spell = 1;
            f.save = 3;
            const int got = combat::harm(b, B, 20, f, d2, &down);
            half += got == 10 || got == 5;
            none += got == 0;
        }
        chk(half > 0 && none > 0 && half + none == 30, "fire: half, or none on a save");
    }
    // engulfed by both attacks hitting; its blow at the one inside at its turn
    {
        scene();
        combat::Fighter& at = F(B);
        at.rec[0x199] = 60;
        at.attacks[0] = 2;
        at.attacks[1] = 0;
        at.rec[0x19E] = 1;
        at.rec[0x1A0] = 4;
        at.rec[0x1A2] = 0;
        create::g_die_hook = [](int nn) { return nn == 20 ? 18 : -1; };
        const combat::Attack r = combat::attack(b, B, 0, names, rng);
        create::g_die_hook = nullptr;
        bool eng = false;
        for (int j = 0; j < r.n_ev; ++j) eng |= r.ev[j].ev == combat::Ev::Engulfs;
        int suff = -1;
        for (int j = 0; F(0).n_aff && j < *F(0).n_aff; ++j)
            if (F(0).aff[j][0] == m.suffocate) suff = F(0).aff[j][3];
        chk(eng && F(0).has(m.held_fast) && suff >= 2 && suff <= 8 && at.has(m.engulfing), "engulfs: held, suffocating 2d4, the mound holding");
        int bite = -1;
        bool go_on = false;
        combat::Event ev[4];
        create::Dice dd(5);
        combat::turn_holds(b, B, dd, ev, 4, &bite, &go_on);
        chk(bite == 0, "its turn: a blow at the one inside");
    }
    expect(P, "the Dark Queen's shambling mound: blows half, electricity + 8, fire half / none on a save, engulfing on both hits - DQK.EXE 0x516D2 on",
           bad == 0, "%d of %d wrong", bad, n);
}

// The stench (Champions' ghast 0x4F GAME.OVR 0x13747, the Dark Queen's 0x89
// DQK.EXE 0x50221): "emits an evil stench"; each foe within a square a save
// against spells or "is affected" - the -2 to hit (Champions 0x76, the
// Queen 0x94) on the holder (the originals), on the one affected with Game
// Fixes; the far foe and its own side untouched
void stench()
{
    if (!fight() || !fg->b.fx || !fg->b.fx->mon.stench) return;
    combat::Battle& b = fg->b;
    const auto& m = b.fx->mon;
    int bad = 0, n = 0;
    auto chk = [&](bool ok, const char* what) {
        ++n;
        if (!ok && !bad++) printf("  (stench: %s)\n", what);
    };
    const bool was = rules::fixes();
    for (int fix = 0; fix < 2; ++fix) {
        rules::set_fixes(fix != 0);
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        const int A = b.party_size > 1 ? 1 : 0;
        mx::put(0, 0, 10, 10, 50);
        if (A) mx::put(A, 0, 11, 11, 50);
        if (A) F(A).size = 1;
        mx::put(B, 1, 11, 10, 50);
        mx::add_aff(B, m.stench);
        create::g_die_hook = [](int nn) { return nn == 20 ? 2 : -1; };
        create::Dice dd(3);
        combat::Event ev[16];
        bool ends = false;
        const int k = combat::special(b, fg->t, B, dd, ev, 16, &ends);
        create::g_die_hook = nullptr;
        int emits = 0, aff = 0;
        for (int j = 0; j < k; ++j) {
            emits += ev[j].ev == combat::Ev::EmitsStench;
            aff += ev[j].ev == combat::Ev::StenchAffected;
        }
        chk(emits == 1, "emits an evil stench");
        chk(aff == (A ? 2 : 1), "each foe next to it affected");
        if (fix) chk(F(0).has(m.stenched) && !F(B).has(m.stenched), "Game Fixes: the one affected gets the -2");
        else chk(F(B).has(m.stenched) && !F(0).has(m.stenched), "the original: its holder gets the -2");
        // a far foe isn't
        for (int kk = 0; kk < b.n; ++kk) b.f[kk].size = 0;
        mx::put(0, 0, 10, 10, 50);
        mx::put(B, 1, 13, 10, 50);
        mx::add_aff(B, m.stench);
        create::g_die_hook = [](int nn) { return nn == 20 ? 2 : -1; };
        const int k2 = combat::special(b, fg->t, B, dd, ev, 16, &ends);
        create::g_die_hook = nullptr;
        int aff2 = 0;
        for (int j = 0; j < k2; ++j) aff2 += ev[j].ev == combat::Ev::StenchAffected;
        chk(aff2 == 0, "a foe 3 squares off untouched");
    }
    rules::set_fixes(was);
    expect(P, "the stench: Champions' ghast 0x4F (GAME.OVR 0x13747), the Dark Queen's 0x89 (DQK.EXE 0x50221)",
           bad == 0, "%d of %d wrong", bad, n);
}

// Champions' breaths (GAME.OVR 0x33079: fire 0x68, cold 0x53): the path
// checked - one of its own side on it, no breath
void champ_breath()
{
    if (game() != 5 || !fight() || !fg->b.fx || !fg->b.fx->mon.breath_check) return;
    combat::Battle& b = fg->b;
    const auto& m = b.fx->mon;
    int bad = 0, n = 0;
    auto chk = [&](bool ok, const char* what) {
        ++n;
        if (!ok && !bad++) printf("  (breath: %s)\n", what);
    };
    for (uint8_t ty : {m.fire_breath, m.cold_breath}) {
        for (int friend_in = 0; friend_in < 2; ++friend_in) {
            for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
            mx::put(0, 0, 10, 10, 250);
            mx::put(B, 1, 14, 10, 40);
            mx::add_aff(B, ty);
            if (friend_in && B + 1 < b.n) mx::put(B + 1, 1, 12, 10, 40);
            create::Dice dd(5);
            combat::Event ev[16];
            bool ends = false;
            const int k = combat::special(b, fg->t, B, dd, ev, 16, &ends);
            bool said = false;
            for (int j = 0; j < k; ++j)
                said |= ev[j].ev == combat::Ev::BreathesFire || ev[j].ev == combat::Ev::BreathesCold;
            const int lost = 250 - F(0).hp();
            if (friend_in && B + 1 < b.n) chk(!said && lost == 0, "its own side on the path: no breath");
            else if (!friend_in) chk(said && (lost == 40 || lost == 20) && ends, "a breath: its hit points or half");
        }
    }
    expect(P, "Champions' breaths: fire 0x68 / cold 0x53, the path checked (GAME.OVR 0x33079)",
           bad == 0, "%d of %d wrong", bad, n);
}

// The round 22 sweep's damage rules (Death Knights GAME.OVR 0x1D826 /
// 0x1D840 / 0x1DB3B, the Queen's 0x87 DQK.EXE 0x500B8): cold double; fire
// none, magic fire heals - other kinds hurt as usual; electricity or cold
// half (once for both)
void dmg22()
{
    if (!fight() || !fg->b.fx) return;
    combat::Battle& b = fg->b;
    const auto& fx = *b.fx;
    if (!fx.q.fire_heal && !fx.q.cold_x2 && !fx.mon.fire_more && !fx.mon.half_magic &&
        !(fx.mon.half_elec && fx.mon.half_elec == fx.mon.half_cold)) return;
    int bad = 0, n = 0;
    auto chk = [&](bool ok, const char* what) {
        ++n;
        if (!ok && !bad++) printf("  (dmg22: %s)\n", what);
    };
    auto hit = [&](uint8_t eff, int kind, int amount, int* hp_after) {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(0, 0, 10, 10, 50);
        mx::put(B, 1, 12, 10, 100);
        b.f[B].rec[0x78] = 120;
        b.f[B].rec[0x1A4] = 100;
        mx::add_aff(B, eff);
        create::Dice dd(3);
        combat::Harm h;
        h.kind = kind;
        h.spell = 1;
        bool down = false;
        const int got = combat::harm(b, B, amount, h, dd, &down);
        if (hp_after) *hp_after = F(B).hp();
        return got;
    };
    int hp = 0;
    if (fx.q.fire_heal) {
        chk(hit(fx.q.fire_heal, 1, 20, &hp) == 0 && hp == 100, "fire: none");
        chk(hit(fx.q.fire_heal, 9, 20, &hp) == 0 && hp > 100, "magic fire: heals");
        chk(hit(fx.q.fire_heal, 2, 20, &hp) > 0, "cold: hurts (not fire)");
        chk(hit(fx.q.fire_heal, 0, 20, &hp) > 0, "a plain harm: hurts");
    }
    if (fx.q.cold_x2) chk(hit(fx.q.cold_x2, 2, 10, nullptr) == 20, "cold: double");
    if (fx.mon.fire_more) {
        chk(hit(fx.mon.fire_more, 1, 20, nullptr) == 30, "the yetis: fire half again");
        chk(hit(fx.mon.fire_more, 2, 20, nullptr) == 20, "the yetis: cold as it is");
    }
    if (fx.mon.half_magic) {
        for (int kind : {1, 2, 4, 8, 9})
            chk(hit(fx.mon.half_magic, kind, 20, nullptr) == 10, "the stone guardian: magic / cold / fire / electricity half");
        chk(hit(fx.mon.half_magic, 0x20, 20, nullptr) == 20, "the stone guardian: breath as it is");
    }
    if (fx.mon.half_elec && fx.mon.half_elec == fx.mon.half_cold) {
        chk(hit(fx.mon.half_elec, 4, 20, nullptr) == 10, "electricity: half");
        chk(hit(fx.mon.half_elec, 2, 20, nullptr) == 10, "cold: half");
        chk(hit(fx.mon.half_elec, 6, 20, nullptr) == 10, "both: half once");
    }
    expect(P, "the round 22 damage rules: cold double, fire none / magic fire heals, electricity or cold half",
           bad == 0, "%d of %d wrong", bad, n);
}

// Treasures' yetis' hug (GAME.OVR 0x251A0): a d20 of 20 grabs (0x9C, the
// yeti's data the one held), its turn squeezes (2d8 / 4d5 / 6d4) and ends,
// its fall lets go
void yeti()
{
    if (!fight() || !fg->b.fx || !fg->b.fx->mon.yeti_held) return;
    combat::Battle& b = fg->b;
    const auto& m = b.fx->mon;
    int bad = 0, n = 0;
    auto chk = [&](bool ok, const char* what) {
        ++n;
        if (!ok && !bad++) printf("  (yeti: %s)\n", what);
    };
    for (int y = 0; y < 3; ++y) {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(0, 0, 10, 10, 200);
        mx::put(B, 1, 11, 10, 100);
        mx::add_aff(B, m.yeti[y]);
        for (int j = 0; F(B).n_aff && j < *F(B).n_aff; ++j)
            if (F(B).aff[j][0] == m.yeti[y]) F(B).aff[j][3] = 0xFF;
        combat::Fighter& at = F(B);
        at.rec[0x199] = 60;
        at.attacks[0] = 1;
        at.attacks[1] = 0;
        at.rec[0x19E] = 1;
        at.rec[0x1A0] = 4;
        at.rec[0x1A2] = 0;
        create::g_die_hook = [](int nn) { return nn == 20 ? 19 : -1; };
        const combat::Attack r = combat::attack(b, B, 0, names, rng);
        create::g_die_hook = nullptr;
        bool grab = false;
        for (int j = 0; j < r.n_ev; ++j) grab |= r.ev[j].ev == combat::Ev::Grabbed;
        int data = -1;
        for (int j = 0; F(B).n_aff && j < *F(B).n_aff; ++j)
            if (F(B).aff[j][0] == m.yeti[y]) data = F(B).aff[j][3];
        chk(grab && F(0).has(m.yeti_held) && data == 0, "a 20: grabbed, its data the one held");
        const int hp0 = F(0).hp();
        int bite = -1;
        bool go_on = true;
        combat::Event ev[8];
        create::Dice dd(7);
        const int k = combat::turn_holds(b, B, dd, ev, 8, &bite, &go_on);
        bool sq = false;
        for (int j = 0; j < k; ++j) sq |= ev[j].ev == combat::Ev::Squeezes;
        const int lost = hp0 - F(0).hp(), lo = m.yeti_dice[y][0], hi = m.yeti_dice[y][0] * m.yeti_dice[y][1];
        chk(sq && lost >= lo && lost <= hi && !go_on, "its turn: squeezes for its dice, the turn over");
        combat::damage(b, B, 250);
        chk(!F(0).has(m.yeti_held), "its fall lets go");
    }
    if (m.ice_breath) {
        int seen7 = 0, seen3 = 0, far_hit = 0;
        for (int seed = 1; seed <= 20; ++seed) {
            for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
            mx::put(0, 0, 10, 10, 100);
            mx::put(B, 1, 11, 10, 100);
            mx::add_aff(B, m.ice_breath);
            create::Dice dd(static_cast<uint32_t>(seed));
            combat::Event ev[16];
            bool ends = false;
            combat::special(b, fg->t, B, dd, ev, 16, &ends);
            const int lost = 100 - F(0).hp();
            seen7 += lost == 7;
            seen3 += lost == 3;
            for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
            mx::put(0, 0, 10, 10, 100);
            mx::put(B, 1, 13, 10, 100);
            mx::add_aff(B, m.ice_breath);
            combat::special(b, fg->t, B, dd, ev, 16, &ends);
            far_hit += F(0).hp() != 100;
        }
        chk(seen7 > 0 && seen3 > 0 && seen7 + seen3 == 20, "the ice hound: 7 cold, half on a save");
        chk(far_hit == 0, "the ice hound: only one next to it");
    }
    expect(P, "Treasures' yetis: the hug (GAME.OVR 0x251A0) - grabbed on a 20, squeezed 2d8 / 4d5 / 6d4, let go",
           bad == 0, "%d of %d wrong", bad, n);
}

// PoolRad's halving effects (the round 22 effects sweep, GAME.OVR's
// handlers): 0x5D fire, 0x72 electricity, 0x76 cold halved; 0x6F poison
// proof. Gateway's 0x5D the same (GAME.OVR 0x13F81).
void pool_halves()
{
    if ((game() != 1 && game() != 8) || !fight() || !fg->b.fx || !fg->b.fx->mon.half_fire) return;
    combat::Battle& b = fg->b;
    const auto& m = b.fx->mon;
    int bad = 0, n = 0;
    auto chk = [&](bool ok, const char* what) {
        ++n;
        if (!ok && !bad++) printf("  (halves: %s)\n", what);
    };
    struct Row { uint8_t t; int kind; const char* what; };
    const Row rows[] = {{m.half_fire, 1, "fire"}, {m.half_elec, 4, "electricity"}, {m.half_cold, 2, "cold"}};
    for (const Row& r : rows) {
        if (!r.t) continue;
        for (int other = 0; other < 2; ++other) {
            for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
            mx::put(0, 0, 10, 10, 200);
            mx::put(B, 1, 11, 10, 200);
            mx::add_aff(B, r.t);
            create::Dice dd(7);
            combat::Harm h;
            h.kind = other ? (r.kind == 1 ? 2 : 1) : r.kind;
            h.spell = 1;
            bool down = false;
            const int got = combat::harm(b, B, 20, h, dd, &down);
            char w[64];
            snprintf(w, sizeof w, "%s: %s", r.what, other ? "other kinds whole" : "20 -> 10");
            chk(got == (other ? 20 : 10), w);
        }
    }
    // the vampire (0x7E) shuns a readied mirror / word 0x98 + 0xFC
    if (m.shuns) {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(0, 0, 10, 10, 200);
        mx::put(B, 1, 11, 10, 200);
        mx::add_aff(B, m.shuns);
        static uint8_t mir[1][items::kRecordSize];
        combat::Fighter& tg = F(0);
        uint8_t (*keep)[items::kRecordSize] = tg.items;
        const int kn = tg.n_items;
        tg.items = mir;
        tg.n_items = 1;
        const uint8_t rows[][5] = {{0x76, 0, 0, 1, 1}, {0x76, 0, 0, 0, 0}, {0x98, 0, 0xFC, 1, 1}, {0x98, 0, 0x10, 1, 0}, {0x10, 0, 0, 1, 0}};
        for (const auto& r : rows) {
            memset(mir[0], 0, sizeof mir[0]);
            mir[0][0x2F] = r[0];
            mir[0][0x30] = r[1];
            mir[0][0x31] = r[2];
            mir[0][0x34] = r[3];
            chk(combat::hidden(b, B, 0) == (r[4] != 0), "the vampire shuns a readied mirror / 0x98 + 0xFC only");
        }
        chk(!combat::hidden(b, 0, B), "shunning is the vampire's own");
        tg.items = keep;
        tg.n_items = kn;
    }
    expect(P, "PoolRad / Gateway halving effects: fire 0x5D, electricity 0x72, cold 0x76 - GAME.OVR's handlers",
           bad == 0, "%d of %d wrong", bad, n);
}

// The Dark Queen's breaths (0x54A51: 0xB4 death, 0xCA paralysis / cold,
// 0x55 cold, 0x53 the big fire) and its troll's regeneration (0x51A3A)
void queen_breath()
{
    if (game() != 7 || !fight() || !fg->b.fx || !fg->b.fx->mon.death_breath) return;
    combat::Battle& b = fg->b;
    const auto& m = b.fx->mon;
    int bad = 0, n = 0, para = 0, cold = 0;
    auto chk = [&](bool ok, const char* what) {
        ++n;
        if (!ok && !bad++) printf("  (breath: %s)\n", what);
    };
    for (uint8_t ty : {m.death_breath, m.para_breath, m.cold_breath, m.big_fire}) {
        for (int seed = 1; seed <= 12; ++seed) {
            create::Dice dd(static_cast<uint32_t>(seed));
            for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
            mx::put(0, 0, 10, 10, 250);
            mx::put(B, 1, 13, 10, 40);
            mx::add_aff(B, ty);
            combat::Event ev[16];
            bool ends = false;
            const int k = combat::special(b, fg->t, B, dd, ev, 16, &ends);
            combat::Ev said = combat::Ev::Down;
            for (int j = 0; j < k; ++j)
                if (ev[j].ev == combat::Ev::BreathesDeath || ev[j].ev == combat::Ev::BreathesPara ||
                    ev[j].ev == combat::Ev::BreathesCold || ev[j].ev == combat::Ev::BreathesFire)
                    said = ev[j].ev;
            const int lost = 250 - F(0).hp();
            if (ty == m.death_breath) chk(said == combat::Ev::BreathesDeath && (lost == 40 || lost == 20) && ends, "death: its hit points or half");
            if (ty == m.para_breath) {
                if (said == combat::Ev::BreathesPara) ++para;
                if (said == combat::Ev::BreathesCold) ++cold;
                chk(ends && (said == combat::Ev::BreathesPara ? lost == 0 : (lost == 40 || lost == 20)), "Baldric: paralysis (no harm) or cold");
            }
            if (ty == m.cold_breath) chk(said == combat::Ev::BreathesCold && (lost == 40 || lost == 20), "cold: its hit points or half");
            if (ty == m.big_fire) chk(said == combat::Ev::BreathesFire && lost >= 18 && lost <= 252 && ends, "the big fire: 24d10 + 12 or half");
        }
    }
    chk(para > 0 && cold > 0, "Baldric: both kinds");
    // the troll: a hit point back at the round's start while monsters stand
    {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(0, 0, 10, 10, 50);
        mx::put(B, 1, 12, 10, 40);
        mx::add_aff(B, b.fx->q.regen1);
        F(B).rec[0x1A4] = 30;
        create::Dice dd(4);
        combat::Event ev[16];
        const int k = combat::start_round(b, dd, ev, 16);
        bool rg = false;
        for (int j = 0; j < k; ++j) rg |= ev[j].ev == combat::Ev::Regen && ev[j].who == B;
        chk(rg && F(B).hp() == 31, "the troll + 1 at the round's start");
        F(B).rec[0x1A4] = 40;
        chk(!combat::queen_regen(b, B), "none at its most");
    }
    expect(P, "the Dark Queen's breaths (death, Baldric's paralysis / cold, cold, the big fire) and its troll's + 1 - DQK.EXE 0x54A51 / 0x51A3A",
           bad == 0, "%d of %d wrong (Baldric %d paralysis, %d cold)", bad, n, para, cold);
}

// The round 22 effects sweep's Queen effects (DQK.EXE handlers 0x4E014 on)
void queen_sweep22()
{
    if (game() != 7 || !fight() || !fg->b.fx || !fg->b.fx->q.cold_x2) return;
    combat::Battle& b = fg->b;
    const auto& fx = *b.fx;
    int bad = 0, n = 0;
    auto chk = [&](bool ok, const char* what) {
        ++n;
        if (!ok && !bad++) printf("  (sweep22: %s)\n", what);
    };
    auto one = [&](uint8_t ty, int kind, int amount) {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(B, 1, 12, 10, 200);
        if (ty) mx::add_aff(B, ty);
        combat::Harm h;
        h.kind = kind;
        h.spell = 1;
        create::Dice dd(3);
        bool down = false;
        return combat::harm(b, B, amount, h, dd, &down);
    };
    chk(one(fx.q.cold_x2, 2, 10) == 20, "0x86 cold double");
    chk(one(fx.q.cold_more, 2, 10) == 12, "0xC0 cold a fifth more");
    chk(one(fx.q.fire_less, 1, 10) == 8, "0xCD fire four fifths");
    chk(one(fx.mon.no_elec, 4, 10) == 0, "0x06 no electricity");
    chk(one(fx.mon.no_fire, 1, 10) == 0, "0x5D no fire");
    chk(one(0, 1, 10) == 10, "nothing: fire 10");
    {
        mx::put(B, 1, 12, 10, 50);
        mx::add_aff(B, fx.q.fire_heal);
        F(B).rec[0x1A4] = 30;
        combat::Harm h;
        h.kind = 9;
        h.spell = 1;
        create::Dice dd(3);
        bool down = false;
        combat::harm(b, B, 10, h, dd, &down);
        chk(F(B).hp() == 40, "0x87 magic fire heals");
    }
    // spell effects not taken
    for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
    mx::put(B, 1, 12, 10, 50);
    mx::add_aff(B, fx.q.immune[0].type);
    combat::Harm h;
    h.spell = 1;
    create::Dice dd(4);
    chk(combat::resists(b, B, fx.mon.paralyzed, h, dd) && combat::resists(b, B, 0x0B, h, dd), "0x7B: paralysis, charm");
    mx::put(B, 1, 12, 10, 50);
    mx::add_aff(B, fx.q.immune[2].type);
    chk(combat::resists(b, B, 0x35, h, dd) && !combat::resists(b, B, fx.mon.paralyzed, h, dd), "0x8D: sleep, not paralysis");
    // no blow harms 0xC1; a +3 needed (0x3C)
    auto blow = [&](uint8_t ty) {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(0, 0, 10, 10, 60);
        mx::put(B, 1, 11, 10, 100);
        mx::add_aff(B, ty);
        combat::Fighter& at = F(0);
        at.rec[0x199] = 60;
        at.attacks[0] = 1;
        at.attacks[1] = 0;
        at.rec[0x19E] = 1;
        at.rec[0x1A0] = 8;
        at.rec[0x1A2] = 3;
        create::g_die_hook = [](int nn) { return nn == 20 ? 18 : nn == 8 ? 8 : -1; };
        const combat::Attack r = combat::attack(b, 0, B, nullptr, rng);
        create::g_die_hook = nullptr;
        return r.n > 0 && r.hits[0].hit ? r.hits[0].damage : -1;
    };
    chk(blow(fx.q.no_blows) == 0, "0xC1 no blow harms");
    chk(blow(0) == 11, "none: 11");
    expect(P, "the Dark Queen's round 22 sweep effects: cold double / more, fire less, magic fire heals, immunities, no blows - DQK.EXE 0x4E014 on",
           bad == 0, "%d of %d wrong", bad, n);
}

void queen_moon_room();
void moons()
{
    if (!m->tables.lay.krynn) return;
    if (m->tables.lay.queen) {
        queen_moons();
        queen_moon_room();
    }
    if (!fight()) return;
    combat::Fighter& f = F(0);
    mx::put(0, 0, 10, 10, 50);
    int spell = 0;
    for (int s = 1; s < m->tables.lay.spell_count && !spell; ++s)
        if (m->tables.spell_code(s) == 3 && m->tables.spell_level(s) >= 1) spell = s;   // (its own code: a magic-user's)
    int bad = 0, n = 0;
    for (int L : {3, 6, 9})
        for (int robe = 1; robe <= 3; ++robe)
            for (int ph = 0; ph < 4; ++ph) {
                levels(0, {{5, L}});
                f.robe = static_cast<uint8_t>(robe);
                int p[3] = {1, 1, 1};
                p[robe - 1] = ph;
                magic::set_moons(p[0], p[1], p[2]);
                const int want = L + (ph == 0 ? -1 : ph == 2 && L > 5 ? 1 : 0);
                ++n;
                if (combat::power_of(f, m->tables, spell) != want && !bad++)
                    printf("  (moons: level %d robe %d phase %d: power %d, want %d)\n", L, robe, ph, combat::power_of(f, m->tables, spell), want);
                // saves: the lowest d20 that saves - against the same fighter
                // without a robe (15 in Champions; the Dark Queen's record
                // gives its own base: only the moon's change is compared)
                auto lowest = [&] {
                    int lo2 = 0;
                    for (int r = 1; r <= 20 && !lo2; ++r) {
                        dice(0, r);
                        if (combat::saving_throw(f, 4, 0, play::rng)) lo2 = r;
                        dice_off();
                    }
                    return lo2;
                };
                f.robe = 0;
                const int base = lowest();
                f.robe = static_cast<uint8_t>(robe);
                const int lo = lowest();
                const int wlo = base + (ph == 0 ? 1 : ph == 2 ? -1 : 0);
                ++n;
                if (getenv("MOONDBG")) printf("  [moondbg] L %d robe %d ph %d lo %d moon %d robe %d\n", L, robe, ph, lo, magic::moon(robe - 1), f.robe);
                if (lo != wlo && !bad++) printf("  (moons: level %d robe %d phase %d: saves from %d, want %d)\n", L, robe, ph, lo, wlo);
            }
    magic::set_moons(0, 0, 0);
    f.robe = 0;
    expect(P, "the moons: a mage's caster level (new -1, full +1 past 5th) and saves (new -1, full +1) by its robe's moon", bad == 0 && spell,
           "%d of %d wrong (spell %d)", bad, n, spell);
}

// The Dark Queen's moon extras (DQK.EXE 0x4501C: phase 1 -> 1, 2 -> 2, less
// what is over the slots at levels 1-9; memorize 0x45519: a magic-user spell
// is taken with no room at its level when extras are left - any level, no
// slots asked): a mage of level 1 (slots at level 1 only), Int 18, nothing
// memorized - room at each level 1-9 = its slots + the extras (the slots'
// room alone without a robe)
void queen_moon_room()
{
    if (!m->tables.lay.queen) return;
    const classes::Tables& t = m->tables;
    party::Character c = pt->m[0];
    for (int k = 0; k < 8; ++k) c.rec[0x109 + k] = c.rec[0x111 + k] = 0;
    c.rec[0x109 + 5] = 1;
    c.rec[0x13] = c.rec[0x12] = 18;
    for (int k = 0; k < magic::kListSize; ++k) c.rec[magic::kListAt + k] = 0;
    int bad = 0, n = 0, no_slot = 0;
    char first[120] = "";
    for (int robe = 0; robe <= 3; ++robe)
        for (int ph = 0; ph < 3; ++ph) {
            c.ext[party::kExtRobe] = static_cast<uint8_t>(robe);
            int p[3] = {0, 0, 0};
            if (robe) p[robe - 1] = ph;
            magic::set_moons(p[0], p[1], p[2]);
            const int extras = robe ? (ph == 1 ? 1 : ph == 2 ? 2 : 0) : 0;
            for (int lv = 1; lv <= 9; ++lv) {
                const int slots = lv <= t.slot_levels() ? classes::slots(c, magic::MagicUser, lv) : 0;
                if (!slots) ++no_slot;
                const int got = magic::room(c, t, magic::MagicUser, lv), want = slots + extras;
                ++n;
                if (got != want && !bad++)
                    snprintf(first, sizeof first, "robe %d phase %d level %d (slots %d): room %d, want %d", robe, ph, lv, slots, got, want);
            }
        }
    magic::set_moons(0, 0, 0);
    expect(P, "the Dark Queen's moon extras: a magic-user spell of any level, slots or none (0x45519)", bad == 0 && no_slot > 0,
           "%d of %d wrong (%s; %d without slots)", bad, n, first, no_slot);
}

// ---- C: the Krynn games' caster levels by class (Champions GAME.OVR
// 0x38FA4, Death Knights 0x34DCA): 6 for one with no cleric / mage levels,
// knight under 9th and ranger under 8th; Champions a cleric's spells at
// max(cleric, knight - 8), a mage's max(mage, ranger - 8) (skill levels);
// Death Knights max(cleric, knight) and max(mage, ranger) (levels) - their
// knights (class 7), never paladins; a ranger's druid spells ranger - 7
// The Dark Queen's caster level (DQK.EXE res 0xB4E0, from its own code):
// no cleric (level 0xA5), magic-user (0xAA), ranger (0xA9) past 7th or
// knight (0xA6) past 8th - 6; by the spell row's class word: 0 / 3 (a
// god's) max(cleric, knight); 1 ranger - 7, at least 0; 2 magic-user, with
// a robe (0x19D > 0) its moon's phase (game block 0x1F9 + robe) 0: - 1, 2:
// + 1 past 5th, then the larger of that and ranger - 8 (signed bytes); 4 12
void queen_caster()
{
    if (!fight()) return;
    combat::Fighter& f = F(0);
    const classes::Tables& t = m->tables;
    // (the game's class words by the engine's row codes 0, 1, 3, 5, 4)
    static const int kCode[5] = {0, 1, 3, 5, 4}, kOrig[5] = {0, 1, 2, 3, 4};
    int sp[5] = {};
    for (int s = 1; s < t.lay.spell_count; ++s)
        for (int k = 0; k < 5; ++k)
            if (!sp[k] && t.spell_code(s) == kCode[k]) sp[k] = s;
    uint32_t seed = 4242;
    auto rnd = [&](int n) { seed = seed * 1103515245u + 12345u; return static_cast<int>((seed >> 16) % n); };
    int bad = 0, n = 0;
    char first[160] = "";
    for (int k = 0; k < 1500; ++k) {
        const int cl = rnd(3) ? 0 : 1 + rnd(14), kn = rnd(3) ? 0 : 1 + rnd(14), ra = rnd(3) ? 0 : 1 + rnd(14),
                  mu = rnd(3) ? 0 : 1 + rnd(14), pa = rnd(4) ? 0 : 1 + rnd(14);
        const int robe = rnd(4), ph[3] = {rnd(4), rnd(4), rnd(4)};
        magic::set_moons(ph[0], ph[1], ph[2]);
        levels(0, {{0, cl}, {3, pa}, {4, ra}, {5, mu}, {7, kn}});
        f.robe = static_cast<uint8_t>(robe);
        party::Character ch;
        f.rec.read(ch.rec);
        ch.ext[party::kExtRobe] = static_cast<uint8_t>(robe);
        for (int j = 0; j < 5; ++j) {
            if (!sp[j]) continue;
            int want;
            if (cl == 0 && mu == 0 && ra < 8 && kn < 9) want = 6;
            else
                switch (kOrig[j]) {
                case 0: case 3: want = std::max(cl, kn); break;
                case 1: want = std::max(ra - 7, 0); break;
                case 2: {
                    int a = mu;
                    if (mu > 0 && robe > 0) {
                        const int p = ph[robe - 1];
                        if (p == 0) --a;
                        else if (p == 2 && mu > 5) ++a;
                    }
                    want = std::max(a, ra - 8);
                    break;
                }
                default: want = 12;
                }
            const int a = combat::power_of(f, t, sp[j]), b = spells::power(ch, t, sp[j]);
            n += 2;
            if (a != want || b != want) {
                bad += (a != want) + (b != want);
                if (!*first)
                    snprintf(first, sizeof first, "cl %d kn %d ra %d mu %d robe %d moon %d class %d: fight %d, camp %d, want %d", cl,
                             kn, ra, mu, robe, robe ? ph[robe - 1] : -1, kOrig[j], a, b, want);
            }
        }
    }
    magic::set_moons(0, 0, 0);
    f.robe = 0;
    expect(P, "the Dark Queen's caster levels (its res 0xB4E0: knights, rangers, the robe's moon)", bad == 0 && sp[0] && sp[2],
           "%d of %d wrong (%s)", bad, n, first);
}

void caster()
{
    if (m->tables.lay.queen) {
        queen_caster();
        return;
    }
    if (!m->tables.lay.krynn) return;
    if (!fight()) return;
    combat::Fighter& f = F(0);
    const classes::Tables& t = m->tables;
    // (the spells by the game's own codes: 0 cleric, 1 druid, 3 magic-user)
    int sp[3] = {};
    for (int s = 1; s < t.lay.spell_count; ++s) {
        const int code = t.spell_code(s), k = code == 3 ? 2 : code;
        if ((code == 0 || code == 1 || code == 3) && !sp[k] && t.spell_level(s) >= 1) sp[k] = s;
    }
    const bool dk = t.lay.dkk;
    struct Case { int cl, pa, ra, mu, kn, cls, champ, knights; };
    // (cleric 0, paladin 3, ranger 4, mage 5, knight 7)
    const Case cases[] = {
        {0, 10, 0, 0, 0, 0, 6, 6},      // a paladin: not counted
        {0, 0, 0, 0, 10, 0, 2, 10},     // a 10th-level knight's cleric spells
        {0, 0, 0, 0, 8, 0, 6, 6},       // under 9th: 6
        {5, 0, 0, 0, 10, 0, 5, 10},
        {7, 12, 0, 0, 0, 0, 7, 7},
        {0, 0, 0, 3, 0, 2, 3, 3},
        {0, 0, 9, 3, 0, 2, 3, 9},       // a ranger-mage
        {0, 0, 9, 0, 0, 2, 1, 9},
        {0, 0, 9, 0, 0, 1, 2, 2},       // a ranger's druid spells
        {0, 0, 7, 0, 0, 1, 6, 6},
    };
    magic::set_moons(1, 1, 1);
    f.robe = 0;
    int bad = 0, n = 0;
    char first[120] = "";
    for (const Case& c : cases) {
        levels(0, {{0, c.cl}, {3, c.pa}, {4, c.ra}, {5, c.mu}, {7, c.kn}});
        const int want = dk ? c.knights : c.champ;
        party::Character ch;
        f.rec.read(ch.rec);
        const int a = combat::power_of(f, t, sp[c.cls]), b = spells::power(ch, t, sp[c.cls]);
        n += 2;
        if (a != want || b != want) {
            bad += (a != want) + (b != want);
            if (!*first)
                snprintf(first, sizeof first, "cl %d pa %d ra %d mu %d kn %d class %d: fight %d, camp %d, want %d", c.cl, c.pa,
                         c.ra, c.mu, c.kn, c.cls, a, b, want);
        }
    }
    magic::set_moons(0, 0, 0);
    expect(P, "Krynn caster levels: knights (not paladins) count, Death Knights' mage takes the ranger's level",
           bad == 0 && sp[0] && sp[1] && sp[2], "%d of %d wrong (%s; spells %d %d %d)", bad, n, first, sp[0], sp[1], sp[2]);
}

// ---- Training's new spell (the spell-list mode 4 of each original): the
// engine's offered list against each game's own rule, the lists and the
// spell table's bytes read from the program image at the originals' own
// addresses - PoolRad GAME.OVR 0x276CA (1-56, a class and level with
// slots, not known), Curse (0x2EBA4) / Gateway (0x356F2): 1-100, slots,
// one it can learn, not known; Secret (0x2D31A): levels 1-7 with magic-user
// slots, the list DS 0x5851 + level x 11 + i; Darkness (0x2A8BE) /
// Treasures (0x337FC): 1-126, class code 2, magic-user slots of its level,
// not known; Champions (0x2E8C3) / Death Knights (0x2A837): the robe's
// list (rec 0x5E / 0x24 == 1: list 2, else 3), DS 0x4208 / 0x8189 + list x
// 40 / 80 + level x 10 + i, levels 1-4 / 1-8 with magic-user slots
// (the slots' places, as classes.cpp keeps them: levels 1-5 in the record, 6-9 in Ext)
uint8_t& slot(party::Character& c, int cls, int lvl)
{
    if (lvl < 5) return c.rec[0x12D + cls * 5 + lvl];
    static const uint8_t kAt[3] = {party::kExtCleric67, party::kExtDruidMore, party::kExtMage67};
    static const uint8_t kAt89[3] = {party::kExtCleric89, party::kExtDruid89, party::kExtMage89};
    return lvl < 7 ? c.ext[kAt[cls] + (lvl - 5)] : c.ext[kAt89[cls] + (lvl - 7)];
}
void unlearn(party::Character& c, int s)
{
    if (s >= 1 && s <= 100) c.rec[0x79 + s - 1] = 0;
    else if (s > 100 && s <= 117) c.ext[party::kExtBook + s - 101] = 0;
    else if (s > 117 && s <= 126) c.ext[party::kExtBook2 + s - 118] = 0;
}
void learning()
{
    const classes::Tables& t = m->tables;
    const int g = game();
    // (the Dark Queen: DQK.EXE read as a file, its data segment at file 0x63A20)
    std::vector<uint8_t> dqk;
    if (g == 7) read_file(game_dir + "/DQK.EXE", dqk);
    else if (prog.empty()) find_table();
    const std::vector<uint8_t>& img = g == 7 ? dqk : prog;
    const uint32_t ds = g == 7 ? 0x63A20 : d->prof->create.ds_image;
    auto pb = [&](uint32_t a) { return a < img.size() ? img[a] : 0; };
    // the spell table's class / level bytes where each original reads them
    uint32_t cls_at = 0, top = 100;
    int kind = 0;                           // 0 by the table (codes 0-2 Curse's), 1 Pool's, 2 class 2 only, 3 lists
    uint32_t lists = 0;
    switch (g) {
    case 1: cls_at = 0x31FA; top = 56; kind = 1; break;
    case 2: cls_at = 0x37DC; break;
    case 8: cls_at = 0x435E; break;
    case 3: kind = 3; lists = 0x5851; break;
    case 4: cls_at = 0x651A; top = 126; kind = 2; break;
    case 9: cls_at = 0x578A; top = 126; kind = 2; break;
    case 5: kind = 3; lists = 0x4208; break;
    case 6: kind = 3; lists = 0x8189; break;
    case 7: kind = 4; lists = 0x16CE; break;
    default: return;
    }
    if (g == 7 && dqk.empty()) {
        expect(P, "training's new spell: DQK.EXE read", false);
        return;
    }
    party::Character base = pt->m[0];
    uint32_t seed = 12345;
    auto rnd = [&](int n) { seed = seed * 1103515245u + 12345u; return static_cast<int>((seed >> 16) % n); };
    int bad = 0, n = 0, offered = 0;
    char first[200] = "";
    for (int k = 0; k < 600; ++k) {
        party::Character ch = base;
        for (int c = 0; c < 8; ++c) ch.rec[0x109 + c] = 0;
        ch.rec[0x109 + 5] = static_cast<uint8_t>(rnd(4) ? 1 + rnd(14) : 0);       // magic-user
        ch.rec[0x109 + 0] = static_cast<uint8_t>(rnd(3) ? 0 : 1 + rnd(12));       // cleric
        ch.rec[0x109 + 4] = static_cast<uint8_t>(rnd(4) ? 0 : 1 + rnd(14));       // ranger
        ch.rec[0x13] = static_cast<uint8_t>(9 + rnd(10));
        ch.rec[0x15] = static_cast<uint8_t>(9 + rnd(10));
        ch.ext[party::kExtRobe] = static_cast<uint8_t>(rnd(3));
        if (g == 1) ch.rec[0x13C] = static_cast<uint8_t>(rnd(3) ? 0 : 1 + rnd(3));   // (Pool's 0xB8: spell 56's count)
        for (int c = 0; c < 3; ++c)
            for (int lv = 1; lv <= 9; ++lv) slot(ch, c, lv - 1) = static_cast<uint8_t>(rnd(3) ? 0 : 1 + rnd(3));
        for (int s = 1; s < 127; ++s) {
            unlearn(ch, s);
            if (s < t.lay.spell_count && rnd(3) == 0) classes::to_book(ch, s);
        }
        std::vector<int> want;
        if (kind == 4) {
            // The Dark Queen's mode 5 (ovl 0xA file 0x5AFD9): list 2 (White,
            // record 0x19D == 1) else 3; L = 1..9 with magic-user slots; DS
            // 0x16CE + (list - 1) x 90 + (L - 1) x 10 + i, i < 10, 0 ends; not
            // known; each put in by its add-to-list (0x5A8AC): after every
            // entry of a level <= its own (the spell row's [2], DS 0x27E9 +
            // 22 x spell), a duplicate not added
            const uint32_t b = ds + lists + (ch.ext[party::kExtRobe] == 1 ? 1 : 2) * 90;
            auto lvl = [&](int s) { return pb(ds + 0x27E9 + 22 * s + 2); };
            for (int lv = 1; lv <= 9; ++lv) {
                if (classes::slots(ch, 2, lv) <= 0) continue;
                for (int i = 0; i < 10; ++i) {
                    const int s = pb(b + (lv - 1) * 10 + i);
                    if (!s) break;
                    if (classes::in_book(ch, s)) continue;
                    size_t at = 0;
                    bool dup = false;
                    while (at < want.size() && lvl(want[at]) <= lvl(s) && !(dup = want[at] == s)) ++at;
                    if (!dup) want.insert(want.begin() + static_cast<long>(at), s);
                }
            }
        } else if (kind == 3) {
            const bool kr = g != 3;
            const int levels = g == 3 ? 7 : g == 6 ? 8 : 4, per = kr ? 10 : 11;
            const uint32_t b = ds + lists + (kr ? (ch.ext[party::kExtRobe] == 1 ? 2 : 3) * (g == 6 ? 80 : 40) : 0);
            for (int lv = 1; lv <= levels; ++lv) {
                if (classes::slots(ch, 2, lv) <= 0) continue;
                for (int i = 1; i <= per; ++i) {
                    const int s = pb(b + lv * per + i);
                    if (!s) break;
                    if (!classes::in_book(ch, s)) want.push_back(s);
                }
            }
        } else {
            for (int lv = 1; lv <= 9; ++lv)
                for (uint32_t s = 1; s <= top && s < t.lay.spell_count; ++s) {
                    const int code = pb(ds + cls_at + s * 16), level = pb(ds + cls_at + s * 16 + 1);
                    if (level != lv || classes::in_book(ch, static_cast<int>(s))) continue;
                    // (the engine's class numbers: Pool's 0 cleric 1 magic-user)
                    const int c = kind == 1 ? (code == 0 ? 0 : code == 1 ? 2 : -1) : code;
                    // (PoolRad's spell 56 - cleric, 7th level - reads its record's 0xB8 as a
                    // slot count (0xB1 + 3 x class + level): Pool's 0xB8 is the engine's 0x13C)
                    if (kind == 1 && level > 3) {
                        const int o = 0xB1 + 3 * code + level;
                        if (!rules::fixes() && c >= 0 && o >= 0xB8 && o <= 0xBC && ch.rec[0x13C + o - 0xB8] > 0)
                            want.push_back(static_cast<int>(s));     // (Game Fixes: none)
                        continue;
                    }
                    if (kind == 2 ? code == 2 && classes::slots(ch, 2, level) > 0
                                  : c >= 0 && c < 3 && classes::slots(ch, c, level) > 0 &&
                                        (kind == 1 || magic::can_use(ch, t, static_cast<int>(s))))
                        want.push_back(static_cast<int>(s));
                }
        }
        uint8_t ids[130];
        const int got = magic::learnable(ch, t, ids, 130);
        ++n;
        offered += got;
        bool same = got == static_cast<int>(want.size());
        for (int i = 0; same && i < got; ++i) same = ids[i] == want[i];
        if (!same) {
            ++bad;
            if (!*first) {
                int o = snprintf(first, sizeof first, "mu %d cl %d ra %d robe %d: engine", ch.rec[0x10E], ch.rec[0x109],
                                 ch.rec[0x10D], ch.ext[party::kExtRobe]);
                for (int i = 0; i < got && o < 150; ++i) o += snprintf(first + o, sizeof first - o, " %d", ids[i]);
                o += snprintf(first + o, sizeof first - o, " / original");
                for (size_t i = 0; i < want.size() && o < 195; ++i) o += snprintf(first + o, sizeof first - o, " %d", want[i]);
            }
        }
    }
    expect(P, "training's new spell: the offered list is the original's (lists / classes / slots / known)", bad == 0 && offered > 0,
           "%d of %d set-ups differ, %d spells offered in all (%s)", bad, n, offered, first);
}

// ---- Items used outside fights (each original's UseMagicItem): which
// items are used (not a scroll, 0x3D a spell, 0x3E under 0x80 - every
// game's test at its use: PoolRad GAME.OVR 0x22B10, Curse 0x29521, Gateway
// 0x2FC56, Darkness 0x256E7, Treasures 0x2E144, Champions 0x2928E, Death
// Knights' alike), the use taken (0x3C 0: never runs out; a pile's count
// first; gone at 0 - the same code in all), and who reads a scroll:
// PoolRad no test (0x22B30); Darkness (0x257E5) / Treasures (0x2E2B6)
// skill levels - magic-user or cleric, else a thief's over 9 and d100 <=
// 75; the rest (Curse 0x29613 ...) the class or class before a magic-user
// or cleric, the levels, else a thief over 9th and the d100. Treasures'
// own items (0x3E 0x0E the lucky paper - the engine's, 0x0F / 0x10 /
// 0x11: routines of their own, 0x2DBF3 / 0x2DD75 / 0x2DB44) are noted.
// Treasures' own items, outside fights (GAME.OVR 0x2E15B before the spell
// path): 0x3E 0x11 (0x2DB44) - the user already has effect 0x3D: nothing
// used ("is already affected"); else it is added (0x23B09: data 0xFF) and
// its handler run - 0x9F's (0x25815): 1d4 x 10 + 40 - and the use taken;
// that effect's caster + 3 (0x40DEF); 0x10 (0x2DD75) shows GEO block 0x3D,
// nothing used; 0x0F (0x2DBF3) does nothing outside fights
// Treasures' own items in a fight (GAME.OVR 0x2E0EF, the Use routine, and
// its 0x0F / 0x10 handlers): the jump 0x2DBF3 - aim (spell 0x7F's row),
// left: the result 1 (a charge goes, the turn over, nothing said); a square
// someone stands on (0x150:0x48 reads only who stands there): "cannot go
// there", not moved, a charge goes; else the effect 0x9C off ("is free to
// move" when it was on), moved there, "jumps", a charge goes - a wall square
// too (the original looks at no ground; with the Game Fixes not); the map
// 0x2DD75: shown, the result left 0 - nothing used, the turn not over.
// Words: the routine's own Pascal strings (cs:0x1FAC / 0x1FBC / 0x1FCC of
// overlay 0x2BC21), read from the player's GAME.OVR here.
bool fight();
// The computer's items: Darkness's, Treasures' and Death Knights' never
// (their item routine, GAME.OVR 0x9D9A / 0xABB4, called first in the
// computer's turn, is an empty body answering no; Death Knights' 0xA4D3
// rolls d7 rounds and walks the items, but its chosen-item pointer is
// only ever 0); the others' (Curse's / Pool's d7 rounds) use a readied
// wand whose spell the item chooser takes.
// A monster with one wand (a fight spell of priority 7 against its enemy,
// the dice highest), no spells: the chooser takes it (the control), and its
// turn uses it - or, Darkness / Treasures, doesn't
int die_high(int n) { return n - 1; }
void ai_items()
{
    if (!fight()) return;
    combat::Battle& b = fg->b;
    const classes::Tables& t = m->tables;
    const int g = game();
    mx::put(0, 0, 10, 10, 60);
    mx::put(B, 1, 13, 10, 60);
    for (int k = B + 1; k < b.n; ++k) b.f[k].size = 0;
    for (int k = 1; k < b.party_size; ++k) b.f[k].size = 0;
    combat::occupancy(b);
    static uint8_t wand[1][items::kRecordSize];
    combat::Fighter& f = b.f[B];
    f.items = wand;
    f.n_items = 1;
    f.can_use = true;
    int found = 0;
    for (int s = 1; s < 0x80 && !found; ++s) {
        memset(wand[0], 0, sizeof wand[0]);
        wand[0][0x2E] = 0x2F;
        wand[0][0x34] = 1;
        wand[0][0x3C] = 5;
        wand[0][0x3D] = static_cast<uint8_t>(s);
        create::g_die_hook = die_high;
        int tg[combat::kMaxFighters], nt = 0;
        const int k = combat::choose_item(b, fg->t, t, B, *names, d->prof->fight.spells, d->prof->fight.n_spells, rng, tg, &nt);
        create::g_die_hook = nullptr;
        if (k == 0 && nt > 0 && tg[0] == 0) found = s;
    }
    if (!found) {
        printf("  (ai items: no wand spell the chooser takes here)\n");
        return;
    }
    const bool want = !d->prof->variant.darkness_files && !d->prof->variant.hlib;
    fg->cur = B;
    fg->ai_checked = false;
    fg->flee_told = true;
    fg->use_item = -1;
    create::g_die_hook = die_high;
    ai_step(C);
    create::g_die_hook = nullptr;
    // (a monster's use copies its list first - own_items: the copy's charges)
    const bool used = f.n_items != 1 || f.items[0][0x3C] != 5;
    if (f.items == wand) {
        f.items = nullptr;
        f.n_items = 0;
    }
    expect(P, "the computer's items: Darkness / Treasures / Death Knights never (no item kept), the others a wand", used == want,
           "game %d, wand spell %d: used %d, want %d", g, found, used, want);
}

// The computer's spell begun earlier (Darkness / Treasures / Death Knights):
// their turn routine sends a computer fighter straight to its AI (Death
// Knights GAME.OVR 0x9384 -> overlay 0xA21B; Darkness 0x9AC5), which takes
// the morale test (+0x1260 / +0x11E7: one fleeing says so), then its items,
// THEN the pending spell (combat byte 0, +0x141 / +0x15A). The other games
// here: at the turn's start. A monster fleeing with a spell begun: the turn's
// start leaves the spell (Darkness-type) or casts it (the others); the flight
// page first, then the spell
void ai_pending()
{
    if (!fight()) return;
    combat::Battle& b = fg->b;
    mx::put(0, 0, 10, 10, 60);
    mx::put(B, 1, 13, 10, 60);
    for (int k = B + 1; k < b.n; ++k) b.f[k].size = 0;
    for (int k = 1; k < b.party_size; ++k) b.f[k].size = 0;
    combat::occupancy(b);
    combat::Fighter& f = b.f[B];
    const bool dk = d->prof->variant.darkness_files || d->prof->variant.hlib || game() == 5;   // (Champions: 0xC699 - round 11)
    f.n_items = 0;
    f.can_cast = f.can_use = true;
    f.fleeing = true;
    f.panic = false;
    f.spell = 1;
    f.spell_n = 1;
    f.spell_t[0] = 0;
    fg->cur = B;
    fg->fx_done = false;
    fg->pages = 0;
    start_turn(C);
    const bool left = f.spell == 1;
    bool fled_first = false, then_cast = false;
    if (left) {
        const int before = fg->pages;
        ai_step(C);
        fled_first = f.spell == 1 && fg->pages > before;
        if (fled_first) {
            ai_step(C);
            then_cast = f.spell == 0;
        }
    }
    f.spell = 0;
    f.spell_n = 0;
    f.fleeing = false;
    fg->pages = 0;
    const bool ok = dk ? left && fled_first && then_cast : !left;
    expect(P, "the computer's pending spell: Darkness / Treasures / Death Knights / the Dark Queen / Champions after the morale page and the items, the others at the turn's start",
           ok, "game %d: left at the start %d, flight page first %d, then cast %d", game(), left, fled_first, then_cast);
}

// The computer's Turn Undead by the undead's type: Champions' AI (GAME.OVR
// 0xC772) asks its finder for one up to a type limit it never sets - the
// stack holds the AI's last dice call's side count there, 4 (2 when its
// d8 came up 8, not kept here): types 1-4 only; the Dark Queen's (DQK.EXE
// 0x2E078, limit 13) and Death Knights' (0xA46C, 13) any type. A monster
// cleric of 5th, a party member of each undead type 1-13 next to it
void ai_turn_types()
{
    const int g = game();
    if ((g != 5 && g != 6 && g != 7) || !play::fg->t.turn_known || !fight()) return;
    int bad = 0, n = 0, turned_n = 0;
    char first[120] = "";
    for (int type = 1; type <= 13; ++type) {
        fight();
        combat::Battle& b = fg->b;
        mx::put(0, 0, 10, 10, 60);
        mx::put(B, 1, 11, 10, 60);
        for (int k = B + 1; k < b.n; ++k) b.f[k].size = 0;
        for (int k = 1; k < b.party_size; ++k) b.f[k].size = 0;
        combat::occupancy(b);
        levels(B, {{0, 5}});
        combat::Fighter& f = b.f[B];
        f.rec[0x15] = f.rec[0x14] = 16;
        f.n_items = 0;
        f.spell = 0;
        f.can_cast = false;
        f.fleeing = false;
        b.f[0].rec[0xE9] = static_cast<uint8_t>(type);
        b.undead_loaded = 2;
        fg->cur = B;
        fg->ai_checked = false;
        fg->flee_told = true;
        fg->use_item = -1;
        fg->pages = 0;
        ai_step(C);
        const bool turned = f.undead_turns > 0;
        b.f[0].rec[0xE9] = 0;
        fg->pages = fg->at = 0;
        fg->ptext_used = 0;
        fg->st = FSt::Turn;
        const bool want = g == 5 ? type <= 4 : true;
        ++n;
        turned_n += turned;
        if (turned != want && !bad++)
            snprintf(first, sizeof first, "type %d: turned %d, want %d", type, turned, want);
    }
    expect(P, "the computer's Turn Undead by type (Champions 1-4 - its finder's unset limit; the Dark Queen / Death Knights any)",
           bad == 0 && turned_n > 0, "game %d: %d of %d wrong (%s)", g, bad, n, first);
    if (g != 5) return;
    // Champions' first dice (GAME.OVR 0xC5B0-0xC614) for every mood 0-6 and
    // every die face: 1-4 kept unless the d4 is 1; else a d8 - 8: d2 + 4 and
    // the limit 2, otherwise a d4 and the limit 4 (champ_ai_dice)
    {
        static int faces[3], at;
        // (the hook gives the faces in turn, each under the die's sides)
        create::g_die_hook = [](int sides) { return at < 3 ? faces[at++] % sides : 0; };
        int wrong = 0, cases = 0;
        char why[120] = "";
        for (int mood = 0; mood <= 6; ++mood)
            for (int a = 0; a < 8; ++a)
                for (int b2 = 0; b2 < 8; ++b2)
                    for (int c2 = 0; c2 < 8; ++c2) {
                        faces[0] = a, faces[1] = b2, faces[2] = c2;
                        // the original's chain on the same faces
                        int k = 0, wm, ws;
                        auto die = [&](int sides) { return faces[k++] % sides + 1; };
                        if (mood >= 1 && mood <= 4 && die(4) != 1) wm = mood, ws = 4;
                        else if (die(8) == 8) wm = die(2) + 4, ws = 2;
                        else wm = die(4), ws = 4;
                        combat::Fighter t{};
                        t.ai_mood = static_cast<uint8_t>(mood);
                        at = 0;
                        champ_ai_dice(t);
                        ++cases;
                        if ((t.ai_mood != wm || t.ai_sides != ws || at != k) && !wrong++)
                            snprintf(why, sizeof why, "mood %d faces %d %d %d: %d / %d (%d dice), want %d / %d (%d)", mood, a, b2, c2,
                                     t.ai_mood, t.ai_sides, at, wm, ws, k);
                    }
        create::g_die_hook = nullptr;
        expect(P, "Champions' computer: its turn's first dice (0xC5B0-0xC614) - the mood kept or rolled, the last die's sides",
               wrong == 0, "%d of %d wrong (%s)", wrong, cases, why);
    }
    // ... and a d8 of 8 makes the limit 2: types 1-2 turned, 3-13 not
    {
        int bad2 = 0, n2 = 0, turned2 = 0;
        char first2[120] = "";
        for (int type = 1; type <= 13; ++type) {
            fight();
            combat::Battle& b = fg->b;
            mx::put(0, 0, 10, 10, 60);
            mx::put(B, 1, 11, 10, 60);
            for (int k = B + 1; k < b.n; ++k) b.f[k].size = 0;
            for (int k = 1; k < b.party_size; ++k) b.f[k].size = 0;
            combat::occupancy(b);
            levels(B, {{0, 5}});
            combat::Fighter& f = b.f[B];
            f.rec[0x15] = f.rec[0x14] = 16;
            f.n_items = 0;
            f.spell = 0;
            f.can_cast = false;
            f.fleeing = false;
            f.ai_mood = 0;
            b.f[0].rec[0xE9] = static_cast<uint8_t>(type);
            b.undead_loaded = 2;
            fg->cur = B;
            fg->ai_checked = false;
            fg->ai_diced = false;
            fg->flee_told = false;
            fg->use_item = -1;
            fg->pages = 0;
            // (the d8 an 8, the d2 a 1; the morale test's dice stay low: it fights)
            create::g_die_hook = [](int sides) { return sides == 8 ? 7 : sides == 2 ? 0 : 0; };
            for (int step = 0; step < 4 && fg->cur == B && !f.undead_turns && fg->st != FSt::Pages; ++step) ai_step(C);
            create::g_die_hook = nullptr;
            const bool turned = f.undead_turns > 0;
            const bool diced = f.ai_sides == 2 && f.ai_mood == 5;
            b.f[0].rec[0xE9] = 0;
            fg->pages = fg->at = 0;
            fg->ptext_used = 0;
            fg->st = FSt::Turn;
            ++n2;
            turned2 += turned;
            if ((turned != (type <= 2) || !diced) && !bad2++)
                snprintf(first2, sizeof first2, "type %d: turned %d, want %d (sides %d mood %d)", type, turned, type <= 2, f.ai_sides, f.ai_mood);
        }
        expect(P, "Champions' computer: after a d8 of 8 its Turn Undead limit is 2 (the d2's sides - 0xC5E6 / 0xC772)",
               bad2 == 0 && turned2 > 0, "%d of %d wrong (%s)", bad2, n2, first2);
    }
}

#include "behave_quick.inc"

void treasures_fight_items()
{
    std::vector<uint8_t> ovr;
    if (!read_file(game_dir + "/GAME.OVR", ovr) || ovr.size() < 0x2DC00) {
        expect(P, "Treasures' items in a fight: GAME.OVR read", false);
        return;
    }
    auto pstr = [&](uint32_t at) { return std::string(reinterpret_cast<const char*>(&ovr[at + 1]), ovr[at]); };
    const std::string w_cannot = pstr(0x2DBCD), w_free = pstr(0x2DBDD), w_jumps = pstr(0x2DBED);
    int bad = 0, n = 0;
    char first[200] = "";
    auto fail = [&](const char* w) { ++bad; if (!*first) snprintf(first, sizeof first, "%s", w); };
    auto pages_text = [&](int from) {
        std::string s;
        for (int p = from; p < fg->pages; ++p)
            for (int l = 0; l < fg->page[p].lines; ++l) s += std::string("|") + (fg->ptext + fg->page[p].line_at[l]);
        return s;
    };
    // case: 0 an empty floor square, 1 the same held (0x9C), 2 a square a
    // monster stands on, 3 the aim left, 4 a wall square (fixes off), 5 the
    // wall square with the Game Fixes
    for (int cs = 0; cs < 6; ++cs) {
        if (!fight()) {
            fail("no fight");
            break;
        }
        rules::set_fixes(cs == 5);
        combat::Battle& b = fg->b;
        int i = -1;
        for (int k = 0; k < b.party_size; ++k)
            if (b.f[k].member == 0) i = k;
        if (i < 0) {
            fail("member 0 not fighting");
            break;
        }
        mx::put(i, 0, 10, 10, 40);
        mx::put(B, 1, 12, 10, 40);
        combat::occupancy(b);
        party::Character& me = pt->m[0];
        while (me.n_items > party::kMaxItems - 1) --me.n_items;
        const int k = me.n_items;
        memset(me.items[k], 0, sizeof me.items[k]);
        me.items[k][0x2E] = 0x2F;
        me.items[k][0x34] = 1;
        me.items[k][0x3C] = 1;              // (one charge: the item goes with it)
        me.items[k][0x3D] = 0xA0;
        me.items[k][0x3E] = 0x0F;
        ++me.n_items;
        b.f[i].n_items = me.n_items;
        if (cs == 1) spells::add_affect(me, 0x9C, 60, 0, false);
        const int items0 = me.n_items;
        fg->cur = i;
        const int from = fg->pages;
        tr_fight_item(k, C);
        ++n;
        if (fg->st != FSt::Aim || !fg->jump) {
            char w2[96];
            snprintf(w2, sizeof w2, "case %d: the boots didn't aim (state %d)", cs, static_cast<int>(fg->st));
            fail(w2);
            continue;
        }
        const int tx = cs == 2 ? 12 : 13, ty = 10;
        if (cs >= 4) b.ground[ty][tx] = 0;         // (a wall: no ground)
        if (cs == 3) {
            aim_exit(C);
        } else {
            fg->manual = true;
            fg->mx = tx;
            fg->my = ty;
            spell_target(C, b.who[ty][tx] ? b.who[ty][tx] - 1 : -1);
        }
        const bool moved = b.f[i].x == tx && b.f[i].y == ty;
        const std::string said = pages_text(from);
        const bool w_moved = cs == 0 || cs == 1 || cs == 4;
        const std::string want = cs == 3 ? "" : cs == 1 ? "|" + w_free + "|" + w_jumps : w_moved ? "|" + w_jumps : "|" + w_cannot;
        const bool held = spells::find_affect(me, 0x9C) >= 0;
        // (the names are lines of their own: only the words compared)
        std::string words;
        for (size_t p = 0, q; p < said.size(); p = q) {
            q = said.find('|', p + 1);
            if (q == std::string::npos) q = said.size();
            const std::string line = said.substr(p, q - p);
            if (line == "|" + w_cannot || line == "|" + w_free || line == "|" + w_jumps) words += line;
        }
        if (moved != w_moved || words != want || me.n_items != items0 - 1 || held || b.f[i].moves || fg->jump) {
            char w2[200];
            snprintf(w2, sizeof w2, "case %d: moved %d (want %d), said '%s' (want '%s'), items %d -> %d, 0x9C %d, moves %d",
                     cs, moved, w_moved, words.c_str(), want.c_str(), items0, me.n_items, held, b.f[i].moves);
            fail(w2);
        }
    }
    rules::set_fixes(true);
    // The map in a fight: shown, nothing used, the turn not over
    if (fight()) {
        combat::Battle& b = fg->b;
        int i = -1;
        for (int k = 0; k < b.party_size; ++k)
            if (b.f[k].member == 0) i = k;
        party::Character& me = pt->m[0];
        while (me.n_items > party::kMaxItems - 1) --me.n_items;
        const int k = me.n_items;
        memset(me.items[k], 0, sizeof me.items[k]);
        me.items[k][0x2E] = 0x2F;
        me.items[k][0x34] = 1;
        me.items[k][0x3C] = 1;
        me.items[k][0x3D] = 0x3A;
        me.items[k][0x3E] = 0x10;
        ++me.n_items;
        if (i >= 0) {
            b.f[i].n_items = me.n_items;
            fg->cur = i;
            pt->selected = 0;
            view_from = Screen::Fight;
            items_direct = true;
            screen = Screen::Items;
            const int pages0 = fg->pages, items0 = me.n_items;
            use_item(k, C);
            const bool map_up = lk.on && lk.then == LuckRun::Then::Paper;
            lk.on = false;
            ++n;
            if (!map_up || me.n_items != items0 || me.items[k][0x3C] != 1 || fg->pages != pages0 || fg->st == FSt::Pages) {
                char w2[160];
                snprintf(w2, sizeof w2, "the map in a fight: shown %d, items %d -> %d, charges %d, pages %d -> %d", map_up, items0,
                         me.n_items, me.items[k][0x3C], pages0, fg->pages);
                fail(w2);
            }
            items_direct = false;
            screen = Screen::Fight;
        }
    }
    expect(P, "Treasures' items in a fight (the boots' jump 0x2DBF3, the map 0x2DD75) as GAME.OVR's routines", bad == 0,
           "%d of %d wrong (%s)", bad, n, first);
}

void treasures_own_items()
{
    party::Character& me = pt->m[0];
    const party::Character keep = me;
    auto* const keep_fg = play::fg;         // (outside fights: the run's fight set aside)
    play::fg = nullptr;
    auto give = [&](uint8_t spell, uint8_t kind) {
        const int k = me.n_items;
        if (k >= party::kMaxItems) return -1;
        memset(me.items[k], 0, sizeof me.items[k]);
        uint8_t* it = me.items[k];
        it[0x2E] = 0x2F;
        it[0x34] = 1;                       // (readied)
        it[0x3C] = 1;
        it[0x3D] = spell;
        it[0x3E] = kind;
        ++me.n_items;
        return k;
    };
    auto drop = [&](int type) {
        int i;
        while ((i = spells::find_affect(me, type)) >= 0) {
            for (int j = i; j + 1 < me.n_affects; ++j) memcpy(me.affects[j], me.affects[j + 1], party::kAffectSize);
            --me.n_affects;
        }
    };
    auto use = [&](int k) {
        pt->selected = 0;
        view_from = Screen::Game;
        screen = Screen::Items;
        use_item(k, C);
        lk.on = false;
        note_until = 0;
        screen = Screen::Game;
    };
    while (me.n_items > party::kMaxItems - 3) --me.n_items;
    int bad = 0, n = 0;
    char first[160] = "";
    auto fail = [&](const char* w) { ++bad; if (!*first) snprintf(first, sizeof first, "%s", w); };
    bool seen[4] = {};
    for (int r = 0; r < 40; ++r) {
        drop(0x9F);
        const int items0 = me.n_items, k = give(0x9F, 0x11);
        use(k);
        const int a = spells::find_affect(me, 0x9F);
        const int mins = a >= 0 ? me.affects[a][1] | me.affects[a][2] << 8 : -1;
        ++n;
        if (a < 0 || me.n_items != items0 || mins < 50 || mins > 80 || mins % 10 || me.affects[a][3] != 0xFF) {
            char w2[96];
            snprintf(w2, sizeof w2, "the potion: effect %d minutes %d data %d, items %d -> %d", a >= 0, mins,
                     a >= 0 ? me.affects[a][3] : -1, items0, me.n_items);
            fail(w2);
        } else seen[(mins - 50) / 10] = true;
        // a second one: already affected - nothing used, nothing added
        const int n_aff = me.n_affects, k2 = give(0x9F, 0x11);
        use(k2);
        ++n;
        if (me.n_affects != n_aff || me.n_items != items0 + 1 || me.items[k2][0x3C] != 1) fail("a second potion while affected was used");
        --me.n_items;
    }
    if (!(seen[0] && seen[1] && seen[2] && seen[3])) fail("the potion's 1d4 x 10 + 40 never gave each of 50 60 70 80");
    // + 3 caster levels with the effect (any spell of the party's)
    {
        const classes::Tables& t = m->tables;
        int s = 1;
        while (s < t.lay.spell_count && !(t.spell_class(s) == 2 && t.spell_level(s) == 1)) ++s;
        drop(0x9F);
        const int p0 = spells::power(me, t, s);
        spells::add_affect(me, 0x9F, 60, 0xFF, true);
        const int p1 = spells::power(me, t, s), i1 = spells::item_power(me, t, s);
        drop(0x9F);
        const int i0 = spells::item_power(me, t, s);
        n += 2;
        if (p1 != p0 + 3 || i1 != i0 + 3) {
            char w2[96];
            snprintf(w2, sizeof w2, "caster level with 0x9F: %d -> %d, an item's %d -> %d", p0, p1, i0, i1);
            fail(w2);
        }
    }
    // the maps and the boots: nothing used, nothing added
    for (uint8_t kind : {0x10, 0x0F}) {
        const int items0 = me.n_items, n_aff = me.n_affects, k = give(kind == 0x10 ? 0x3A : 0xA0, kind);
        pt->selected = 0;
        view_from = Screen::Game;
        screen = Screen::Items;
        use_item(k, C);
        const bool map_up = lk.on && lk.then == LuckRun::Then::Paper;
        lk.on = false;
        screen = Screen::Game;
        ++n;
        if (me.n_items != items0 + 1 || me.items[k][0x3C] != 1 || me.n_affects != n_aff || map_up != (kind == 0x10)) {
            char w2[96];
            snprintf(w2, sizeof w2, "item 0x3E %02X: items %d -> %d, charges %d, effects %d -> %d, map shown %d", kind, items0,
                     me.n_items, me.items[k][0x3C], n_aff, me.n_affects, map_up);
            fail(w2);
        }
        --me.n_items;
    }
    me = keep;
    play::fg = keep_fg;
    expect(P, "Treasures' own items (0x3E 0x11 the effect, 0x10 the map, 0x0F nothing outside fights) as GAME.OVR 0x2E15B's routines",
           bad == 0, "%d of %d wrong (%s)", bad, n, first);
    treasures_fight_items();
}

void using_items()
{
    const classes::Tables& t = m->tables;
    const int g = game();
    if (g == 7) {                           // (the Dark Queen: its computer's items only)
        ai_items();
        return;
    }
    const magic::Scrolls sc = play::scroll_facts();
    uint32_t seed = 777;
    auto rnd = [&](int n) { seed = seed * 1103515245u + 12345u; return static_cast<int>((seed >> 16) % n); };
    int bad = 0, n = 0;
    char first[160] = "";
    auto fail = [&](const char* w) { ++bad; if (!*first) snprintf(first, sizeof first, "%s", w); };
    // Which are used
    for (int k = 0; k < 2000; ++k) {
        uint8_t it[63] = {};
        it[0x2E] = static_cast<uint8_t>(rnd(256));
        it[0x3C] = static_cast<uint8_t>(rnd(4));
        it[0x3D] = static_cast<uint8_t>(rnd(3) ? rnd(256) : 0);
        it[0x3E] = static_cast<uint8_t>(rnd(256));
        const bool scroll = magic::is_scroll(sc, it);
        const bool want = !scroll && it[0x3D] != 0 && it[0x3E] < 0x80 &&
                          !(g == 9 && it[0x3E] >= 0x0E && it[0x3E] <= 0x11);
        bool got = magic::usable(sc, it);
        if (g == 9 && it[0x3E] >= 0x0E && it[0x3E] <= 0x11) got = false;   // (Treasures' own: below)
        ++n;
        if (got != want) {
            char w2[96];
            snprintf(w2, sizeof w2, "type %d 0x3D %02X 0x3E %02X: used %d, want %d", it[0x2E], it[0x3D], it[0x3E], got, want);
            fail(w2);
        }
    }
    // The use taken
    for (int count = 0; count < 4; ++count)
        for (int ch = 0; ch < 4; ++ch) {
            uint8_t it[63] = {};
            it[0x39] = static_cast<uint8_t>(count);
            it[0x3C] = static_cast<uint8_t>(ch);
            const bool gone = magic::use_charge(it);
            const bool w_gone = ch > 0 && count <= 1 && ch - 1 == 0;
            const int w_count = ch > 0 && count > 1 ? count - 1 : count, w_ch = ch > 0 && count <= 1 ? ch - 1 : ch;
            ++n;
            if (gone != w_gone || it[0x39] != w_count || it[0x3C] != w_ch) {
                char w2[96];
                snprintf(w2, sizeof w2, "count %d charges %d: %d %d gone %d", count, ch, it[0x39], it[0x3C], gone);
                fail(w2);
            }
        }
    // Who reads a scroll
    party::Character base = pt->m[0];
    for (int k = 0; k < 600; ++k) {
        party::Character c = base;
        for (int j = 0; j < 8; ++j) c.rec[0x109 + j] = c.rec[0x111 + j] = 0;
        const int cls[4] = {0, 3, 4, 5};
        for (int j : cls) if (rnd(4) == 0) c.rec[0x109 + j] = static_cast<uint8_t>(1 + rnd(14));
        if (rnd(3) == 0) c.rec[0x109 + 6] = static_cast<uint8_t>(1 + rnd(14));
        if (rnd(6) == 0) c.rec[0x111 + (rnd(2) ? 0 : 5)] = static_cast<uint8_t>(1 + rnd(10));
        const int th = c.rec[0x10F];
        int want;
        if (g == 1) want = magic::kReads;
        else if (g == 4 || g == 9)
            want = classes::skill_level(c, classes::MagicUser) > 0 || classes::skill_level(c, classes::Cleric) > 0 ? magic::kReads
                   : classes::skill_level(c, classes::Thief) > 9                                                       ? magic::kD100
                                                                                                                    : magic::kOops;
        else
            want = c.rec[0x109] || c.rec[0x10E] || c.rec[0x111] || c.rec[0x116] ? magic::kReads : th > 9 ? magic::kD100 : magic::kOops;
        const int got = magic::scroll_reader(c, t);
        ++n;
        if (got != want) {
            char w2[96];
            snprintf(w2, sizeof w2, "levels cl %d pa %d ra %d mu %d th %d (old cl %d mu %d): %d, want %d", c.rec[0x109], c.rec[0x10C],
                     c.rec[0x10D], c.rec[0x10E], th, c.rec[0x111], c.rec[0x116], got, want);
            fail(w2);
        }
    }
    expect(P, "items used outside fights: which are used, the use taken, who reads a scroll", bad == 0, "%d of %d wrong (%s)", bad,
           n, first);
    if (g == 9) treasures_own_items();
    ai_items();
}

// ---- Who may learn / memorize a spell in the Krynn games (their
// can-learn routine: Champions GAME.OVR 0x2DC8E, Death Knights 0x29C84,
// by the game's own class codes 0 / 2 cleric, 1 druid, 3 magic-user):
// cleric spells - Wisdom over 8 and a cleric, a knight past 5th (Death
// Knights also a paladin past 8th; Champions the levels before a human's
// dual class too); druid spells - a ranger past 6th (Death Knights 7th);
// magic-user spells - Intelligence over 8 (outside fights)
// The can-learn routines' fight clause (Secret GAME.OVR 0x2C76B, Darkness
// 0x29DED, Treasures 0x32CCE, Death Knights 0x29D23, the Dark Queen DQK.EXE
// 0x5A402, Champions 0x2DD6B - their own race numbers 6 / 5 / 5 / 6 / 6 /
// 6, human): in a fight (the fight state byte 5) a human magic-user with
// armour readied (the record's armour pointer) can't use magic-user spells,
// unless a ranger past 8th; outside fights, without armour, another race:
// can. Curse, PoolRad and Gateway have no such clause
void armour_can()
{
    const classes::Tables& t = m->tables;
    const int g = game();
    const bool clause = g == 3 || g == 4 || g == 9 || g == 6 || g == 7 || g == 5;
    int armour_type = -1;
    for (int ty = 0; ty < 256 && armour_type < 0; ++ty)
        if (names->type(static_cast<uint8_t>(ty)).slot == items::kSlotArmour) armour_type = ty;
    int spell = 0;
    for (int s = 1; s < t.lay.spell_count && !spell; ++s)
        if ((t.lay.queen ? t.spell_code(s) == 3 : t.spell_class(s) == magic::MagicUser) && t.spell_level(s) == 1) spell = s;
    if (armour_type < 0 || !spell || !fight()) {
        expect(P, "the armoured magic-user in a fight: set up", false, "armour type %d spell %d", armour_type, spell);
        return;
    }
    int bad = 0, n = 0;
    char first[160] = "";
    struct Case { int race, ranger; bool armour, fighting; };
    const Case cases[] = {{7, 0, true, true}, {7, 8, true, true}, {7, 9, true, true}, {7, 0, false, true},
                          {7, 0, true, false}, {1, 0, true, true}, {2, 0, true, true}};
    auto* const keep_fg = play::fg;
    for (const Case& cs : cases) {
        party::Character c = pt->m[0];
        for (int j = 0; j < 8; ++j) c.rec[0x109 + j] = c.rec[0x111 + j] = 0;
        c.rec[0x109 + 5] = 5;
        c.rec[0x109 + 4] = static_cast<uint8_t>(cs.ranger);
        c.rec[0x13] = c.rec[0x12] = 15;             // (Intelligence, its own and in use)
        c.rec[0x74] = static_cast<uint8_t>(cs.race);
        c.n_items = 0;
        if (cs.armour) {
            memset(c.items[0], 0, sizeof c.items[0]);
            c.items[0][0x2E] = static_cast<uint8_t>(armour_type);
            c.items[0][0x34] = 1;
            c.n_items = 1;
        }
        play::fg = cs.fighting ? keep_fg : nullptr;
        const bool got = magic::can_use(c, t, spell);
        play::fg = keep_fg;
        const bool want = !(clause && cs.race == 7 && cs.ranger <= 8 && cs.armour && cs.fighting);
        ++n;
        if (got != want) {
            ++bad;
            if (!*first)
                snprintf(first, sizeof first, "race %d ranger %d armour %d fight %d: can %d, want %d", cs.race, cs.ranger,
                         cs.armour, cs.fighting, got, want);
        }
    }
    // Champions' way out (GAME.OVR 0x2DD6B): a human whose new class has
    // passed his old one (fighter 10 over 9 - record 0xE6) with a former
    // ranger past 8th and a magic-user level, now or former, can; not yet
    // passed (fighter 5), or no magic-user level at all, can't
    if (g == 5) {
        struct Dual { int fighter, old_ranger, mu, old_mu; bool want; };
        const Dual ds[] = {{10, 9, 0, 5, true}, {10, 9, 3, 0, true}, {5, 9, 0, 5, false}, {10, 8, 0, 5, false}, {10, 9, 0, 0, false}};
        for (const Dual& dc : ds) {
            party::Character c = pt->m[0];
            for (int j = 0; j < 8; ++j) c.rec[0x109 + j] = c.rec[0x111 + j] = 0;
            c.rec[0x109 + 2] = static_cast<uint8_t>(dc.fighter);
            c.rec[0x109 + 5] = static_cast<uint8_t>(dc.mu);
            c.rec[0x111 + 4] = static_cast<uint8_t>(dc.old_ranger);
            c.rec[0x111 + 5] = static_cast<uint8_t>(dc.old_mu);
            c.rec[0xE6] = 9;
            c.rec[0x13] = c.rec[0x12] = 15;
            c.rec[0x74] = 7;
            memset(c.items[0], 0, sizeof c.items[0]);
            c.items[0][0x2E] = static_cast<uint8_t>(armour_type);
            c.items[0][0x34] = 1;
            c.n_items = 1;
            const bool got = magic::can_use(c, t, spell);
            ++n;
            if (got != dc.want) {
                ++bad;
                if (!*first)
                    snprintf(first, sizeof first, "Champions dual: fighter %d old ranger %d mu %d old mu %d: can %d, want %d", dc.fighter,
                             dc.old_ranger, dc.mu, dc.old_mu, got, dc.want);
            }
        }
    }
    expect(P, "the armoured human magic-user in a fight (the can-learn's fight clause)", bad == 0, "%d of %d wrong (%s)", bad, n,
           first);
}

void krynn_can()
{
    const classes::Tables& t = m->tables;
    const int g = game();
    if (g != 5 && g != 6 && g != 7) return;
    // (the Dark Queen - DQK.EXE ovl 0xA file 0x5A311, its row's class
    // word: 0 cleric / 3 a god's special - Wisdom over 8 and a cleric, a
    // knight past 5th or a paladin past 8th; 1 druid - Wisdom and a ranger
    // past 7th; 2 magic-user - Intelligence and a magic-user or a ranger
    // past 8th; levels, never former ones. The engine's row codes: 0, 1,
    // 3 magic-user, 5 special)
    int sp[6] = {};
    for (int s = 1; s < t.lay.spell_count; ++s) {
        const int code = t.spell_code(s);
        if (code >= 0 && code < 6 && code != 2 && code != 4 && !sp[code] && (t.spell_level(s) >= 1 || code == 5)) sp[code] = s;
    }
    party::Character base = pt->m[0];
    uint32_t seed = 99;
    auto rnd = [&](int n) { seed = seed * 1103515245u + 12345u; return static_cast<int>((seed >> 16) % n); };
    int bad = 0, n = 0;
    char first[160] = "";
    auto* const keep_fg = play::fg;             // (outside fights: the fight clause is armour_can's)
    play::fg = nullptr;
    for (int k = 0; k < 800; ++k) {
        party::Character c = base;
        for (int j = 0; j < 8; ++j) c.rec[0x109 + j] = c.rec[0x111 + j] = 0;
        for (int j : {0, 3, 4, 5, 7}) if (rnd(3) == 0) c.rec[0x109 + j] = static_cast<uint8_t>(1 + rnd(14));
        c.rec[0x13] = static_cast<uint8_t>(3 + rnd(16));
        c.rec[0x15] = static_cast<uint8_t>(3 + rnd(16));
        const int cl = c.rec[0x109], pa = c.rec[0x10C], ra = c.rec[0x10D], kn = c.rec[0x110];
        const int mu = c.rec[0x10E];
        for (int code : {0, 1, 3, 5}) {
            if (!sp[code]) continue;
            bool want;
            if (code == 0 || code == 5) want = c.rec[0x15] > 8 && (cl > 0 || kn > 5 || (g != 5 && pa > 8));
            else if (code == 1) want = c.rec[0x15] > 8 && ra > (g != 5 ? 7 : 6);
            else want = c.rec[0x13] > 8 && (g == 5 || mu > 0 || ra > 8);     // (Death Knights 0x29D16 too)
            const bool got = magic::can_use(c, t, sp[code]);
            ++n;
            if (got != want) {
                ++bad;
                if (!*first)
                    snprintf(first, sizeof first, "code %d Wis %d Int %d cl %d pa %d ra %d kn %d: %d, want %d", code, c.rec[0x15],
                             c.rec[0x13], cl, pa, ra, kn, got, want);
            }
        }
    }
    play::fg = keep_fg;
    expect(P, "Krynn: who may learn / memorize a spell (clerics, knights past 5th, the druid's rangers)", bad == 0 && n > 0,
           "%d of %d wrong (%s; spells %d %d %d %d)", bad, n, first, sp[0], sp[1], sp[3], sp[5]);
}

// The Dark Queen's fight words (DQK.EXE res 0xCD31, from 0xB14E: columns
// 23-38, rows to 21): its loop modelled on the original's own code - a
// word is leading "-.,?!:;" marks, letters to a space or a mark, then the
// space, or the marks after (to the last index but one); it goes on the
// row when its last character (the space too) is within the box, a word
// whose space is the one just past the box goes without it and the row
// ends; else the next row, the spaces skipped - compared letter by letter
// (both drawn) with the engine's panel_box_text
void queen_wrap()
{
    if (game() != 7) return;
    static std::vector<uint8_t> px2(px.size());
    pic::Canvas& C2 = C;                    // (drawn on the run's own canvas, then kept aside)
    static const char* kWords[] = {"Sir",  "Dryden", "hits", "the",    "Red",    "Dragon", "for",  "18",  "points",
                                   "of",   "damage", "goes", "down",   "and",    "is",     "Dying", "a",   "Wyvern",
                                   "Kapak", "Draconian!", "...", "-", "misses", "Lenduira's", "spell:", "?!", "magic,"};
    uint32_t seed = 31337;
    auto rnd = [&](int n) { seed = seed * 1103515245u + 12345u; return static_cast<int>((seed >> 16) % n); };
    auto punct = [](char ch) { return strchr("-.,?!:;", ch) != nullptr; };     // (its strchr: 0 too)
    int bad = 0, n = 0, edge = 0, fulls = 0;
    char first[160] = "";
    for (int k = 0; k < 3000; ++k) {
        char s[400] = "";
        const int words = 1 + rnd(k % 3 ? 7 : 22);
        for (int w2 = 0; w2 < words; ++w2) {
            if (w2) strcat(s, rnd(8) ? " " : "  ");
            strcat(s, kWords[rnd(sizeof kWords / sizeof kWords[0])]);
        }
        if (rnd(4) == 0) strcat(s, " ");
        const int row0 = 11 + rnd(k % 3 ? 4 : 10);
        // the original's loop (0-based; L = the last index)
        C2.clear(0);
        const int L = static_cast<int>(strlen(s)) - 1, x0 = 23, x1 = 38, y1 = 21;
        int col = x0, row = row0, di = 0;
        while (L >= 0 && di <= L) {
            int si = di;
            while (si < L && punct(s[si])) ++si;
            while (si < L && !punct(s[si]) && s[si] != ' ') ++si;
            if (s[si] != ' ')
                while (si + 1 < L && punct(s[si + 1])) ++si;
            bool newline = false;
            if (col + (si - di) > x1) {
                if (col + (si - di) == x1 + 1 && s[si] == ' ') {
                    ++edge;
                    --si;
                    for (; di <= si; ++di, ++col) {
                        const char one[2] = {s[di], 0};
                        play::put(C2, one, col, row, 10);
                    }
                }
                newline = true;
            }
            if (newline) {
                col = x0;
                ++row;
                while (di <= L && s[di] == ' ') ++di;
                if (row > y1 && di < L) {
                    // (0xCEEA: a key, the box cleared, the word at its top)
                    ++fulls;
                    C2.fill(x0 * 8, row0 * 8, 16 * 8, (y1 + 1 - row0) * 8, 0);
                    col = x0;
                    row = row0;
                    for (; di <= si; ++di, ++col) {
                        const char one[2] = {s[di], 0};
                        play::put(C2, one, col, row, 10);
                    }
                }
                continue;
            }
            for (; di <= si; ++di, ++col) {
                const char one[2] = {s[di], 0};
                play::put(C2, one, col, row, 10);
            }
        }
        px2 = px;
        C.clear(0);
        int cut = -1, from = 0;
        play::panel_box_text(C, s, row0, 10, 21, false, true, &cut);
        while (cut >= 0) {                  // (the engine's FSt::More: the key, then the rest)
            from += cut;
            C.fill(23 * 8, row0 * 8, 16 * 8, (22 - row0) * 8, 0);
            play::panel_box_text(C, s + from, row0, 10, 21, false, true, &cut);
        }
        ++n;
        if (memcmp(px.data(), px2.data(), px2.size()) != 0) {
            ++bad;
            if (!*first) snprintf(first, sizeof first, "\"%s\" from row %d", s, row0);
        }
    }
    snprintf(w, sizeof w, "the Dark Queen's fight words wrap as its res 0xCD31 (letter by letter; %d words' space just past the box, %d full boxes - 0xCEEA)",
             edge, fulls);
    expect(P, w, bad == 0 && n > 0 && edge > 0 && fulls > 0, "%d of %d differ (%s)", bad, n, first);
}

// The Dark Queen's falls (DQK.EXE 0x25020, the hit: the attacker's name at
// row 10, "Attacks" 11, the target's name 12, the words from 13; then -
// no wait between, res 0xB14E at [0xAE4E] + 1, the row after the words'
// last - the name, "goes down", "and is Dying" a row each; res 0x8F04, a
// damage text from row 10: the name at the row after its words, "Goes Down"
// + " and is Dying" (0x19B, "%s and is Dying" 0x1A5) through the box), the
// rows above kept. Rows read from the canvas: a row "used" when its columns
// 23-38 have a lit pixel
bool row_used(int r)
{
    for (int y = r * 8; y < r * 8 + 8; ++y)
        for (int x = 23 * 8; x < 39 * 8; ++x)
            if (C.get(x, y)) return true;
    return false;
}
void queen_pages()
{
    if (game() != 7 || !fight()) return;
    combat::Battle& b = fg->b;
    mx::put(0, 0, 10, 10, 60);
    mx::put(B, 1, 11, 10, 60);
    auto run = [&](bool hit) {
        fg->pages = fg->at = 0;
        fg->ptext_used = 0;
        fg->page_end = 10;
        C.fill(23 * 8, 10 * 8, 16 * 8, 12 * 8, 0);
        b.f[B].rec[0x195] = party::Dying;
        if (hit) {
            add_page(0, fw(profile::kAttacksW), "Hitting for 7 points of damage", B);
            down_page(B, true);
        } else {
            add_page(B, "takes 7 points of damage");
            down_page(B, false);
        }
        fg->st = FSt::Pages;
        show_page(C);
        const int end1 = fg->page_end;
        const bool words_kept = row_used(hit ? 13 : 11);
        show_page(C);
        const Page& p2 = fg->page[1];
        std::string l1 = p2.lines > 1 ? page_line(p2, 1) : "";
        const int end2 = fg->page_end;
        b.f[B].rec[0x195] = party::Okay;
        fg->pages = fg->at = 0;
        fg->ptext_used = 0;
        fg->st = FSt::Turn;
        return std::make_tuple(end1, end2, words_kept, l1, row_used(end1), p2.lines);
    };
    auto [h1, h2, hk, hl1, hn, hlines] = run(true);
    // (the hit: 10 name, 11 Attacks, 12 name, 13-14 "Hitting for 7 / points of damage" - "damage" in the box's
    // last column, so 0xCD31's end moves the row on: [0xAE4E] = 15, the fall from 16)
    expect(P, "the Dark Queen's fall after a hit: below the hit's words (0x25020) - name, goes down, and is Dying a row each",
           h1 == 16 && hk && hn && h2 == 19 && hlines == 3, "hit ended %d (want 16), words kept %d, name there %d, fall ended %d (want 19), lines %d",
           h1, hk, hn, h2, hlines);
    auto [d1, d2, dk, dl1, dn, dlines] = run(false);
    char want[64];
    snprintf(want, sizeof want, "%s%s", fw(profile::kGoesDownS), fw(profile::kGoesDownDyingQ));
    // (10 name, 11-12 "takes 7 points / of damage"; the fall from 13: name, then "Goes Down and is" / "Dying")
    expect(P, "the Dark Queen's fall after a damage text (res 0x8F04): below its words, \"Goes Down and is Dying\" as one",
           d1 == 13 && dk && dn && d2 == 16 && dlines == 2 && dl1 == want && dl1 == std::string("Goes Down and is Dying"),
           "text ended %d (want 13), kept %d, name %d, fall ended %d (want 16), lines %d, words \"%s\"", d1, dk, dn, d2, dlines, dl1.c_str());
    // The weapon line's trailing space (res 0x9FDC: every part + " "): a
    // 15-letter name's space fills the box's last column - the print ends a
    // row lower, the status (two rows past) at row 10, not 9
    const int wrow = panel_box_text(C, "Footman's Flail ", 7, 10, 9, false, true) + 1;
    const int wrow0 = panel_box_text(C, "Footman's Flail", 7, 10, 9, false, true) + 1;
    expect(P, "the Dark Queen's weapon name ends with a space (res 0x9FDC) - the status row after it", wrow == 10 && wrow0 == 9,
           "with the space %d, without %d", wrow, wrow0);
}

// The Dark Queen's "lost a spell" (round 9): a caster hurt with a spell under
// way loses it on the spot - res 0xB14E(target, "lost a spell", 12, wait)
// after a blow's page (DQK.EXE 0x2522F: the rows above 12 kept, its wait
// clearing 10-21; the fall then at the row after the blow's words, kept at
// 0x251F2, on a blank box) or a damage text's (0x9106: the fall at [0xAE4E]
// + 1 - the row after "lost a spell"); nothing left for its turn. The
// undead (record 0xC8 & 8): "goes down" / "Goes Down" only (0x2528C,
// 0x9157). "reflects it!" at row 12 under the gaze's page (0x4EAEF), "engulfs
// %s" at 12 on a blank box (0x5181C). The full box's key: res 0xD400 ->
// 0xE645, the prompt box "Press [Return] to continue." (press_line)
// (round 11: a fighter's effects, read and put on as the engine keeps them)
int q_find(const combat::Fighter& f, uint8_t t)
{
    for (int k = 0; f.n_aff && k < *f.n_aff; ++k)
        if (f.aff[k][0] == t) return k;
    return -1;
}
void q_add(combat::Fighter& f, uint8_t t, int minutes)
{
    if (!f.n_aff || *f.n_aff >= f.max_aff) return;
    uint8_t* a = f.aff[(*f.n_aff)++];
    memset(a, 0, party::kAffectSize);
    a[0] = t;
    a[1] = static_cast<uint8_t>(minutes);
}

// Round 22 (2026-11-12): the Dark Queen under water (game byte 0x3C;
// DQK.EXE's spell handlers, the spell number to handler by its table DS
// 0xAF45 + 4 x spell filled at 0x59F37-0x5A2E1): Burning Hands, Fireball,
// Flame Strike, Delayed Blast Fireball, Fire Touch, Meteor Swarm, the
// Necklace of Missiles - "That has no effect underwater!" (DS 0x87C9) and
// nothing done; Lightning Bolt (0x57183) goes to Fireball's routine, which
// takes everyone within 2 of the square aimed at (0x1D99:0xE9B with 2,
// 0xFF); Ice Storm (0x57FBB) 1d10 for 3d10
// Gateway under water (round 22: script word 0x4CF7 = its game block's
// +0x3EE, set by ECL2's fights): Fireball and the item fireball (0x2F /
// 0x40) "Nothing Happens." (their handler GAME.OVR 0x385FB), no harm done
void gateway_water()
{
    if (game() != 8 || !fight()) return;
    combat::Battle& b = fg->b;
    if (!mrules) load_magic();
    if (!mrules) return;
    mx::put(0, 0, 10, 10, 200);
    mx::put(B, 1, 13, 10, 200);
    b.gw_water = true;
    int ok = 0, n = 0;
    for (int sp : {0x2F, 0x40}) {
        const combat::FightSpell* fs = fight_spell(sp);
        if (!fs || fs->does != combat::SpellDoes::Damage) continue;
        ++n;
        combat::SpellLine lines[8];
        const int tg[1] = {B};
        b.aim_x = b.f[B].x;
        b.aim_y = b.f[B].y;
        const int hp = b.f[B].hp();
        const int k = combat::cast(b, mrules->tables, 0, sp, *fs, tg, 1, rng, lines, 8, 9);
        if (k == 1 && lines[0].did == combat::Did::Water && b.f[B].hp() == hp) ++ok;
    }
    int ammo = 0;
    const int shot = combat::missile(b, b.f[0], &ammo);
    b.gw_water = false;
    const std::string word = fw(profile::kNoEffectWaterQ), clears = fw(profile::kWaterClearsGw);
    expect(P, "Gateway under water: Fireball and the item fireball \"Nothing Happens.\" (GAME.OVR 0x385FB), nothing shoots "
              "(0x42318), a cloud gone \"The water clears a little...\" (0x129B6)",
           n > 0 && ok == n && word == "Nothing Happens." && shot == 0 && clears == "The water clears a little...",
           "%d of %d, the word \"%s\", missile %d, \"%s\"", ok, n, word.c_str(), shot, clears.c_str());
}

void queen_water()
{
    if (game() != 7 || !fight()) return;
    combat::Battle& b = fg->b;
    if (!mrules) load_magic();
    if (!mrules) return;
    mx::put(0, 0, 10, 10, 200);
    mx::put(B, 1, 13, 10, 200);
    b.underwater = true;
    int none_ok = 0, none_n = 0;
    std::string word;
    for (int sp : {9, 64, 47, 74, 115, 118, 124, 133}) {
        const combat::FightSpell* fs = fight_spell(sp);
        if (!fs) continue;
        ++none_n;
        combat::SpellLine lines[8];
        const int tg[1] = {B};
        b.aim_x = b.f[B].x;
        b.aim_y = b.f[B].y;
        const int hp = b.f[B].hp();
        const int n = combat::cast(b, mrules->tables, 0, sp, *fs, tg, 1, rng, lines, 8, 9);
        if (n == 1 && lines[0].did == combat::Did::Water && b.f[B].hp() == hp) ++none_ok;
    }
    word = fw(profile::kNoEffectWaterQ);
    expect(P, "the Dark Queen under water: Burning Hands, Fireball, Flame Strike, Delayed Blast Fireball, Fire Touch, "
              "Meteor Swarm, the Necklace of Missiles do nothing - \"That has no effect underwater!\" (DS 0x87C9)",
           none_n > 0 && none_ok == none_n && word == "That has no effect underwater!", "%d of %d, the word \"%s\"", none_ok,
           none_n, word.c_str());
    // Lightning Bolt: one 1 square from the aim (off the bolt's line beside
    // the caster) is hurt; one 3 squares off is not
    int bolt_hit = -1, bolt_far = -1;
    if (const combat::FightSpell* fs = fight_spell(51)) {
        mx::put(0, 0, 4, 10, 200);
        mx::put(B, 1, 13, 10, 200);
        const int C2 = B + 1 < b.n ? B + 1 : -1;
        if (C2 >= 0) mx::put(C2, 1, 13, 12, 200);
        b.aim_x = 13;
        b.aim_y = 11;
        const int h1 = b.f[B].hp(), h2 = C2 >= 0 ? b.f[C2].hp() : 0;
        combat::SpellLine lines[16];
        const int tg[1] = {B};
        combat::cast(b, mrules->tables, 0, 51, *fs, tg, 1, rng, lines, 16, 9);
        bolt_hit = b.f[B].hp() < h1;
        bolt_far = C2 >= 0 ? b.f[C2].hp() < h2 : -1;
        (void)bolt_far;
        // and one 4 squares off the aim on the bolt's line from the caster: untouched
        mx::put(B, 1, 9, 10, 200);
        const int h3 = b.f[B].hp();
        b.aim_x = 13;
        b.aim_y = 10;
        combat::cast(b, mrules->tables, 0, 51, *fs, tg, 1, rng, lines, 16, 9);
        bolt_far = b.f[B].hp() < h3;
    }
    expect(P, "the Dark Queen under water: Lightning Bolt bursts round the square aimed at (within 2) - one beside it hurt, "
              "one on the bolt's line 4 squares short of it untouched (0x57183 -> 0x56BD9)",
           bolt_hit == 1 && bolt_far == 0, "beside %d, on the line %d", bolt_hit, bolt_far);
    // Ice Storm: 1d10, never more than 10 (20 casts)
    int ice_max = -1;
    if (const combat::FightSpell* fs = fight_spell(87)) {
        for (int k = 0; k < 20; ++k) {
            mx::put(B, 1, 13, 10, 200);
            b.aim_x = 13;
            b.aim_y = 10;
            const int h = b.f[B].hp();
            combat::SpellLine lines[8];
            const int tg[1] = {B};
            combat::cast(b, mrules->tables, 0, 87, *fs, tg, 1, rng, lines, 8, 9);
            ice_max = std::max(ice_max, h - b.f[B].hp());
        }
    }
    expect(P, "the Dark Queen under water: Ice Storm 1d10 (0x57FBB; 3d10 on land)", ice_max >= 1 && ice_max <= 10,
           "the most in 20 casts %d", ice_max);
    // The round's moves: a third (0xA767), and nothing shoots (0x24CA1)
    {
        combat::Fighter& f = b.f[0];
        const int mv = f.rec[0x1A5] >= 1 && f.rec[0x1A5] <= 96 ? f.rec[0x1A5] : 1;
        const int keep_weather = b.weather;
        b.weather = 2;                  // (no weather)
        b.underwater = false;
        combat::start_round(b, rng);
        const int land = f.moves;
        b.underwater = true;
        combat::start_round(b, rng);
        const int water = f.moves;
        b.weather = keep_weather;
        int ammo = 0;
        expect(P, "the Dark Queen under water: moves a third (DQK.EXE 0xA767: without effect 0x96), missile() none (0x24CA1)",
               land == mv * 2 && water == mv * 2 / 3 && combat::missile(b, f, &ammo) == 0, "move %d: land %d, water %d", mv, land,
               water);
    }
    // Fire Shield (85, 0x57DE0): the hot one does nothing, the cold one is
    // given as on land
    {
        int hot = -1, cold = -1;
        if (const combat::FightSpell* fs = fight_spell(85)) {
            mx::put(0, 0, 10, 10, 200);
            b.underwater = true;
            combat::SpellLine lines[8];
            const int tg[1] = {0};
            b.flame = 1;
            int n = combat::cast(b, mrules->tables, 0, 85, *fs, tg, 1, rng, lines, 8, 9);
            hot = n == 1 && lines[0].did == combat::Did::Water;
            b.flame = 2;
            const int had = b.f[0].n_aff ? *b.f[0].n_aff : 0;
            n = combat::cast(b, mrules->tables, 0, 85, *fs, tg, 1, rng, lines, 8, 9);
            cold = (b.f[0].n_aff ? *b.f[0].n_aff : 0) > had;
        }
        expect(P, "the Dark Queen under water: Fire Shield's hot flame \"That has no effect underwater!\", the cold one given "
                  "(0x57DE0)", hot == 1 && cold == 1, "hot %d, cold %d", hot, cold);
    }
    // The party's blows (0x95's handler 0x503A5): a weapon of blow type 0
    // full damage, another / (the bonus + 1), one with a range none; a
    // monster's unchanged
    {
        int t_cut = -1, t_other = -1, t_far = -1;
        for (int k = 1; k < 256; ++k) {
            const items::TypeInfo& ti = names->type(k);
            if (ti.slot != items::kSlotWeapon || !ti.sides) continue;
            if (ti.range <= 1 && ti.blow == 0 && t_cut < 0) t_cut = k;
            if (ti.range <= 1 && ti.blow != 0 && t_other < 0) t_other = k;
            if (ti.range > 1 && t_far < 0) t_far = k;
        }
        combat::Fighter& at = b.f[0];
        party::Character& ch = pt->m[at.member >= 0 ? at.member : 0];
        auto blow = [&](int type, bool water) {
            for (int k = 0; k < b.n; ++k) b.f[k].size = k == 0 || k == B ? b.f[k].size : 0;
            mx::put(0, 0, 10, 10, 60);
            mx::put(B, 1, 11, 10, 200);
            memset(ch.items[0], 0, items::kRecordSize);
            ch.items[0][0x2E] = static_cast<uint8_t>(type);
            ch.items[0][0x34] = 1;
            ch.n_items = 1;
            at.items = ch.items;
            at.n_items = 1;
            at.rec[0x199] = 60;
            at.attacks[0] = 1;
            at.attacks[1] = 0;
            at.rec[0x19E] = 1;
            at.rec[0x1A0] = 8;
            at.rec[0x1A2] = 3;
            b.underwater = water;
            create::g_die_hook = [](int nn) { return nn == 20 ? 18 : nn == 8 ? 8 : -1; };
            const combat::Attack r = combat::attack(b, 0, B, nullptr, rng);
            create::g_die_hook = nullptr;
            b.underwater = false;
            return r.n > 0 && r.hits[0].hit ? r.hits[0].damage : -1;
        };
        const int land = t_cut > 0 ? blow(t_cut, false) : -9, cut = t_cut > 0 ? blow(t_cut, true) : -9;
        const int other = t_other > 0 ? blow(t_other, true) : -9, far = t_far > 0 ? blow(t_far, true) : -9;
        expect(P, "the Dark Queen under water: the party's blows (effect 0x95, 0x503A5) - 1d8 (8) + 3: blow type 0 full (11), "
                  "another / 4 (2), a weapon with a range 0",
               land == 11 && cut == 11 && other == 2 && far == 0, "types %d / %d / %d: land %d, type 0 %d, other %d, ranged %d",
               t_cut, t_other, t_far, land, cut, other, far);
    }
    b.underwater = false;
}

void queen_lost()
{
    if (game() != 7 || !fight()) return;
    combat::Battle& b = fg->b;
    mx::put(0, 0, 10, 10, 60);
    mx::put(B, 1, 11, 10, 60);
    auto reset = [&]() {
        fg->pages = fg->at = 0;
        fg->ptext_used = 0;
        fg->page_end = 10;
        fg->st = FSt::Turn;
        fg->fx_redraw = false;
        C.fill(0, 0, pic::kScreenW, pic::kScreenH, 0);
    };
    // (a page shown; its pictures' field redraw left out - the panel's rows only)
    auto show = [&]() {
        fg->fx_redraw = false;
        show_page(C);
        fg->fx_redraw = false;
    };
    // (a row used: a pixel in columns 23-38 not the box's own colour - its
    // clear is the paper colour, grey, not black)
    auto rows = [&]() {
        const uint8_t blank = C.paper >= 0 ? static_cast<uint8_t>(C.paper) : 0;
        uint32_t m = 0;
        for (int r = 10; r <= 21; ++r)
            for (int y = r * 8; y < r * 8 + 8 && !(m >> r & 1); ++y)
                for (int x = 23 * 8; x < 39 * 8; ++x)
                    if (C.get(x, y) != blank) {
                        m |= 1u << r;
                        break;
                    }
        return m;
    };
    auto casting = [&]() {
        b.f[B].spell = 0x0E;
        b.f[B].spell_n = 0;
        b.f[B].can_cast = false;
    };
    // (rows as bits; the blow: 10 name, 11 Attacks, 12 name, 13-14 words - ends 15, the fall from 16)
    auto R = [](int a, int z) { uint32_t m = 0; for (int r = a; r <= z; ++r) m |= 1u << r; return m; };
    // 1. after a blow, the target dying
    reset();
    casting();
    b.f[B].rec[0x195] = party::Dying;
    add_page(0, fw(profile::kAttacksW), "Hitting for 7 points of damage", B);
    const bool got = q_lost_spell(B, true);
    down_page(B, true);
    const int pages = fg->pages;
    fg->st = FSt::Pages;
    show();
    show();
    const uint32_t m2 = rows();
    const std::string l2 = page_line(fg->page[1], 1);
    show();
    const uint32_t m3 = rows();
    expect(P, "the Dark Queen: \"lost a spell\" after the blow (0x2522F) - row 12 under the blow's first rows, the fall at the blow's end on a blank box, the spell gone",
           got && pages == 3 && l2 == fw(profile::kLostSpell) && m2 == (R(10, 11) | R(12, 13)) && m3 == R(16, 18) && b.f[B].spell == 0,
           "added %d, pages %d, \"%s\", rows after it %x (want %x), after the fall %x (want %x), spell %d", got, pages, l2.c_str(), m2,
           R(10, 13), m3, R(16, 18), b.f[B].spell);
    // 2. after a damage text, the target dying
    reset();
    casting();
    add_page(B, "takes 7 points of damage");
    const bool got2 = q_lost_spell(B, false);
    down_page(B, false);
    fg->st = FSt::Pages;
    show();
    show();
    const uint32_t d2 = rows();
    show();
    const uint32_t d3 = rows();
    // (10 name, 11-12 the damage; 12 name, 13 "lost a spell" - [0xAE4E] 13; the fall from 14: name, "Goes Down and is" / "Dying")
    expect(P, "the Dark Queen: \"lost a spell\" after a damage text (0x9106) - row 12, the fall at the row after it (0x91B4) on a blank box",
           got2 && d2 == R(10, 13) && d3 == R(14, 16) && b.f[B].spell == 0, "added %d, rows %x (want %x), fall %x (want %x), spell %d", got2, d2,
           R(10, 13), d3, R(14, 16), b.f[B].spell);
    // 3. no spell under way, or not hurt: nothing; not the Dark Queen's rule elsewhere (fire_pending)
    reset();
    b.f[B].spell = 0x0E;
    b.f[B].can_cast = true;
    const bool none1 = !q_lost_spell(B, true);
    b.f[B].spell = 0;
    b.f[B].can_cast = false;
    const bool none2 = !q_lost_spell(B, true);
    expect(P, "the Dark Queen: no \"lost a spell\" page without a spell under way or a hurt", none1 && none2 && fg->pages == 0,
           "unhurt %d, no spell %d, pages %d", none1, none2, fg->pages);
    // 4. the undead: "goes down" / "Goes Down" whatever the status
    const uint8_t f2 = b.f[B].flags2;
    b.f[B].flags2 = static_cast<uint8_t>(f2 | 8);
    b.f[B].rec[0x195] = party::Dead;
    reset();
    add_page(0, fw(profile::kAttacksW), "Hitting for 7 points of damage", B);
    down_page(B, true);
    const std::string u1 = fg->pages == 2 && fg->page[1].lines == 2 ? page_line(fg->page[1], 1) : "?";
    reset();
    add_page(B, "takes 7 points of damage");
    down_page(B, false);
    const std::string u2 = fg->pages == 2 && fg->page[1].lines == 2 ? page_line(fg->page[1], 1) : "?";
    b.f[B].flags2 = f2;
    reset();
    add_page(B, "takes 7 points of damage");
    down_page(B, false);
    const std::string u3 = fg->pages == 2 ? page_line(fg->page[1], fg->page[1].lines - 1) : "?";
    b.f[B].rec[0x195] = party::Okay;
    expect(P, "the Dark Queen: an undead's fall (record 0xC8 & 8) - \"goes down\" after a blow (0x2528C), \"Goes Down\" after a damage text (0x9157); others \"is killed\"",
           u1 == fw(profile::kGoesDown) && u2 == fw(profile::kGoesDownS) && u3 == fw(profile::kIsKilled), "\"%s\", \"%s\", others \"%s\"",
           u1.c_str(), u2.c_str(), u3.c_str());
    // 5. "reflects it!" at row 12, the gaze's rows kept; "engulfs" at 12 on a blank box
    reset();
    add_page(0, fw(profile::kGazes));
    combat::Event e{};
    e.ev = combat::Ev::Reflects;
    e.who = B;
    e.to = -1;
    event_words(e);
    e.ev = combat::Ev::Engulfs;
    e.who = 0;
    e.to = B;
    event_words(e);
    fg->st = FSt::Pages;
    show();
    show();
    const uint32_t r2 = rows();
    show();
    const uint32_t r3 = rows();
    expect(P, "the Dark Queen: \"reflects it!\" at row 12 under the gaze's rows (0x4EAEF), \"engulfs %s\" at 12 on a blank box (0x5181C)",
           r2 == R(10, 13) && (r3 & R(10, 13)) == R(12, 13), "reflects %x (want %x), engulfs %x (want rows 12-13 used, 10-11 blank: %x); words \"%s\" \"%s\" \"%s\"", r2, R(10, 13), r3,
           R(12, 13), fw(profile::kGazes), fw(profile::kReflects), fw(profile::kEngulfs));
    // 6. the full box's key: the prompt box (res 0xD400 -> 0xE645) as press_line draws it
    reset();
    // (going on below a page that ended at row 15: past the box's last row)
    const char* longw = "Hitting for 7 points of damage and more words";
    fg->page_end = 15;
    fg->cont_next = true;
    add_page(0, longw, longw, B, longw);
    fg->st = FSt::Pages;
    show();
    const bool more = fg->st == FSt::More;
    std::vector<uint8_t> a(pic::kScreenW * 8), z(pic::kScreenW * 8);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < pic::kScreenW; ++x) a[y * pic::kScreenW + x] = C.get(x, text::kMenuRow * 8 + y);
    pic::Canvas& C0 = C;
    C0.fill(0, text::kMenuRow * 8, pic::kScreenW, 8, 0);
    press_line(C0, 11);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < pic::kScreenW; ++x) z[y * pic::kScreenW + x] = C.get(x, text::kMenuRow * 8 + y);
    expect(P, "the Dark Queen: a full box's key is the prompt box \"Press [Return] to continue.\" (res 0xD400 -> 0xE645)",
           more && a == z && d->w_press[0][0], "full %d, same %d, prompt words %d", more, a == z, d->w_press[0][0] != 0);
    // 7. Feeblemind (round 10, DQK.EXE 0x53EA6): "loses intellect" (row 10,
    // a wait - 0x53EDE), then with a spell under way "lost a spell" at row
    // 12 (0x53F17), the spell gone; the save failed (a d20 of 1)
    {
        reset();
        b.f[B].spell = 0x0E;
        b.f[B].spell_n = 0;
        b.f[B].can_cast = true;
        const combat::FightSpell* fs = nullptr;
        for (int k = 0; k < d->prof->fight.n_spells; ++k)
            if (d->prof->fight.spells[k].does == combat::SpellDoes::Feeble) fs = &d->prof->fight.spells[k];
        bool ok = false;
        std::string w1, w2;
        int n = 0, row2 = -1;
        // (round 11: a spell being memorized, one memorized - 0x58184 / 0x53F43
        // clear the first only; Intelligence / Wisdom in use 3 at once; effect
        // 0x44 kept, its time 0 - the spell's row: no duration)
        b.f[B].rec[magic::kListAt + 10] = 0x85;
        b.f[B].rec[magic::kListAt + 11] = 0x07;
        b.f[B].rec[0x13] = b.f[B].rec[0x15] = 15;
        if (fs && !mrules) load_magic();
        if (fs && mrules) {
            create::g_die_hook = [](int sides) { return sides == 20 ? 0 : -1; };
            combat::SpellLine lines[8];
            const int tg[1] = {B};
            n = combat::cast(b, mrules->tables, 0, fs->spell, *fs, tg, 1, rng, lines, 8, 9);
            create::g_die_hook = nullptr;
            spell_pages(0, lines, n, *fs, false);
            if (fg->pages >= 2) {
                w1 = page_line(fg->page[fg->pages - 2], fg->page[fg->pages - 2].lines - 1);
                w2 = page_line(fg->page[fg->pages - 1], 1);
                row2 = fg->page[fg->pages - 1].row;
            }
            char want1[48];
            ow(fs->word, want1, sizeof want1);
            ok = w1 == want1 && w2 == fw(profile::kLostSpell) && row2 == 12 && b.f[B].spell == 0;
        }
        const int fk = q_find(b.f[B], 0x44);
        const bool kept = fk >= 0 && b.f[B].aff[fk][1] == 0 && b.f[B].aff[fk][2] == 0;
        const bool ints = b.f[B].rec[0x13] == 3 && b.f[B].rec[0x15] == 3;
        const bool list = b.f[B].rec[magic::kListAt + 10] == 0 && b.f[B].rec[magic::kListAt + 11] == 0x07;
        expect(P, "the Dark Queen: Feeblemind keeps effect 0x44 (no time - its row, 0x5B2FB), Intelligence / Wisdom in use 3 at once (0x53EC5), the spells being memorized forgotten, the memorized kept (0x58184)",
               kept && ints && list, "effect %d (time %d), Int %d Wis %d, list %02x %02x", fk >= 0,
               fk >= 0 ? b.f[B].aff[fk][1] | b.f[B].aff[fk][2] << 8 : -1, int(b.f[B].rec[0x13]), int(b.f[B].rec[0x15]),
               int(b.f[B].rec[magic::kListAt + 10]), int(b.f[B].rec[magic::kListAt + 11]));
        combat::lose_aff(b.f[B], 0x44);
        b.f[B].rec[magic::kListAt + 11] = 0;
        expect(P, "the Dark Queen: Feeblemind - \"loses intellect\", then \"lost a spell\" at row 12 (0x53EDE / 0x53F17), the spell gone",
               ok, "spell found %d, %d lines, pages %d: \"%s\" / \"%s\" row %d, spell %d (fighters %d, B up %d status %d)", fs != nullptr, n,
               fg->pages, w1.c_str(), w2.c_str(), row2, b.f[B].spell, b.n, b.f[B].up(), b.f[B].status());
    }
    // 8. a stinking cloud (DQK.EXE 0x80B0-0x8199): its save made, "starts to
    // cough" (effect 0x1E) and with a spell under way "lost a spell" at row
    // 12 (0x816B); the save failed (nausea, 0x1F): nothing about the spell
    for (int saved = 1; saved >= 0; --saved) {
        reset();
        b.f[B].spell = 0x0E;
        b.f[B].spell_n = 0;
        b.f[B].can_cast = true;
        const bool laid = combat::lay_cloud(b, fg->t, b.f[B].x, b.f[B].y, 3);
        static int want_save;
        want_save = saved;
        create::g_die_hook = [](int sides) { return sides == 20 ? (want_save ? 19 : 0) : sides == 4 ? 2 : -1; };
        cloud_breath(B);
        create::g_die_hook = nullptr;
        const std::string last = fg->pages ? page_line(fg->page[fg->pages - 1], 1) : "";
        const bool lost = fg->pages >= 2 && last == fw(profile::kLostSpell) && fg->page[fg->pages - 1].row == 12;
        const bool ok = laid && (saved ? lost && b.f[B].spell == 0 : !lost && fg->pages == 1 && b.f[B].spell == 0x0E);
        expect(P, saved ? "the Dark Queen: one coughing in a stinking cloud loses the spell under way - \"lost a spell\" at row 12 (0x816B)"
                        : "the Dark Queen: nausea in a stinking cloud (its save failed, 0x819B) says nothing of the spell",
               ok, "cloud %d, pages %d, last \"%s\", spell %d", laid, fg->pages, last.c_str(), b.f[B].spell);
        // (round 11, 0x80EF / 0x81BC: coughing / nausea for 1d4 + 1 - the d4 a 3 here - data 0xFF)
        const int ck = q_find(b.f[B], saved ? 0x1E : 0x1F);
        expect(P, saved ? "the Dark Queen: coughing for 1d4 + 1 (0x80EF), data 0xFF" : "the Dark Queen: nausea for 1d4 + 1 (0x81BC), data 0xFF",
               ck >= 0 && b.f[B].aff[ck][1] == 4 && b.f[B].aff[ck][2] == 0 && b.f[B].aff[ck][3] == 0xFF, "effect %d: time %d, data %d", ck >= 0,
               ck >= 0 ? b.f[B].aff[ck][1] : -1, ck >= 0 ? b.f[B].aff[ck][3] : -1);
        combat::lose_aff(b.f[B], saved ? 0x1E : 0x1F);
        while (b.n_clouds) combat::clouds_round(b);
        b.f[B].spell = 0;
    }
    // 9. who breathes nothing (0x805D-0x80AB): the undead (type 0x5C), one
    // nauseous already (0x1F), effect 0x8F, flags 2 & 0x18 - no effect added
    {
        int bad = 0;
        char first[80] = "";
        for (int c = 0; c < 4; ++c) {
            combat::Fighter& f = b.f[B];
            const uint8_t type = f.rec[0xE9], f2 = f.flags2;
            if (c == 0) f.rec[0xE9] = 1;
            if (c == 1) q_add(f, 0x1F, 3);
            if (c == 2) q_add(f, 0x8F, 3);
            if (c == 3) f.flags2 = static_cast<uint8_t>(f2 | 0x10);
            const int before = *f.n_aff;
            const combat::Did r = combat::breathe_cloud(b, B, rng);
            if ((r != combat::Did::Unaffected || *f.n_aff != before) && !bad++) snprintf(first, sizeof first, "case %d: effects %d -> %d", c, before, *f.n_aff);
            f.rec[0xE9] = type;
            f.flags2 = f2;
            for (uint8_t t : {uint8_t(0x1F), uint8_t(0x8F), uint8_t(0x1E)}) {
                while (q_find(f, t) >= 0 && combat::lose_aff(f, t)) {}
            }
        }
        expect(P, "the Dark Queen: the undead, the nauseous, effect 0x8F and flags 2 & 0x18 breathe no stinking cloud (0x805D-0x80AB)", bad == 0, "%s", first);
    }
    reset();
}

// Training's spell list offered at all (read for the Krynn games): a
// magic-user's new level, or - Champions (GAME.OVR 0x25666: [0x4B58] = 0,
// creation's own levelling aside) - a ranger past 8th after any training;
// Death Knights (GAME.OVR 0x199DE, ovl 0x61 +0x49DE) and the Dark Queen
// (ovl 5 +0x563B): a ranger past 8th whose own level went up. Every pair
// of levels before / after for magic-user and ranger 7-11
void train_spell_offer()
{
    // (the others, read 2026-11-09 - GAME.OVR: Curse 0x25534, Secret 0x22837,
    // Gateway 0x2BEFC, Darkness 0x18DF6, Treasures 0x1E6E7: Champions' rule;
    // PoolRad 0x1C8DE: a magic-user's new level only)
    const int g = game();
    int bad = 0, n = 0;
    char first[120] = "";
    party::Character c = pt->m[0];
    for (int mb = 0; mb <= 2; ++mb)
        for (int ma = mb; ma <= mb + 1; ++ma)
            for (int rb = 7; rb <= 11; ++rb)
                for (int ra = rb; ra <= rb + 1; ++ra) {
                    for (int k = 0; k < 8; ++k) c.rec[0x109 + k] = 0;
                    c.rec[0x109 + 5] = static_cast<uint8_t>(ma);
                    c.rec[0x109 + 4] = static_cast<uint8_t>(ra);
                    const bool got = play::train_offers_spell(c, mb, rb);
                    const bool want = ma > mb || (g == 1 ? false : g == 6 || g == 7 ? ra > 8 && ra > rb : ra > 8);
                    ++n;
                    if (got != want && !bad++)
                        snprintf(first, sizeof first, "mu %d->%d ranger %d->%d: offered %d, want %d", mb, ma, rb, ra, got, want);
                }
    expect(P, "training's spell list offered (a magic-user's level; PoolRad nothing more, Death Knights / the Dark Queen a ranger's new level past 8th, the others any ranger past 8th)",
           bad == 0, "game %d: %d of %d wrong (%s)", g, bad, n, first);
}

// Round 19 (tools/devkit/oracles/aistep.py, qstep.py): the step routine's
// tests before its way - the NPC test (control byte 0x80: a d100, + the
// morale routine's figure under the party's health figure: no step, the
// guard routine, unless on the monsters' side; none in PoolRad), the
// magic-user's (class 5 not running: the guard routine - with no armour or
// always; Secret: unless a human past its former class; Gateway: by the
// distance, 0 / 1 the guard, 2 / 3 the way turned round, more on) - and the
// morale routine's 102 test (Darkness, Treasures, the Dark Queen: none)
int r19_seq[8], r19_at = 0;
int r19_die(int n) { return r19_at < 8 && r19_seq[r19_at] > 0 ? (r19_seq[r19_at++] - 1) % n : 0; }
void step19(const uint8_t* row, int npc_ctrl, int mage_armour, const char* mage_then, int morale_cap)
{
    combat::Battle& b = fg->b;
    const auto& t = fg->t;
    const items::Names* nm = b.names;
    int n = 0, bad = 0;
    char first[220] = "";
    auto check = [&](const char* what, bool ok) {
        ++n;
        if (!ok && !bad++) snprintf(first, sizeof first, "%s", what);
    };
    const uint8_t floor = b.ground[3][5];
    static uint8_t inv[1][items::kRecordSize];
    int armour_ty = -1;
    for (int k = 1; nm && k < 256 && armour_ty < 0; ++k)
        if (nm->type(k).slot == items::kSlotArmour) armour_ty = k;
    // me (0 the party's, B a monster) at 10,10, its foe dist squares away the way dir
    auto scene = [&](int me, int dir, int dist) {
        for (int y = 0; y < combat::kH; ++y)
            for (int x = 0; x < combat::kW; ++x) b.ground[y][x] = floor;
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        const int foe = me ? 0 : B;
        mx::put(me, me ? 1 : 0, 10, 10, 60);
        mx::put(foe, me ? 0 : 1, 10 + dist * combat::dx(dir), 10 + dist * combat::dy(dir), 60);
        combat::occupancy(b);
        combat::Fighter& f = b.f[me];
        f.ai_mood = 1;
        f.moves = 12;
        f.target = static_cast<int16_t>(foe);
        f.rec[0x10E] = 0;
        f.rec[0x75] = 2;
        f.items = nullptr;
        f.n_items = 0;
        b.morale_fig = 10;
        b.enemy_health = 60;
        b.taunter = -1;
    };
    auto plan = [&](int me, std::initializer_list<int> rolls) {
        r19_at = 0;
        int k = 0;
        for (int r : rolls) r19_seq[k++] = r;
        while (k < 8) r19_seq[k++] = 1;
        create::g_die_hook = r19_die;
        const combat::Plan p = combat::think(b, t, me, rng);
        create::g_die_hook = nullptr;
        return p;
    };
    const int toward = (row[1 * 5 + 1] + 2) & 7, away = (row[1 * 5 + 1] + 6) & 7;
    char w[160];
    // the NPC test (the party's member 0 stepping toward B, way 2)
    for (int roll : {40, 55}) {
        scene(0, 2, 3);
        b.f[0].rec[0xF7] = 0x80 | 10;
        const combat::Plan p = plan(0, {roll});
        const bool held = npc_ctrl >= 0 && roll + 10 < 60;
        snprintf(w, sizeof w, "NPC (control 0x8A) on the party's side, d100 %d + figure 10 against health 60: %s", roll, held ? "the guard" : "a step");
        check(w, held ? p.act == combat::Act::Guard : (p.act == combat::Act::Step && p.dir == toward));
    }
    scene(0, 2, 3);
    b.f[0].rec[0xF7] = 0x20;
    check("not an NPC (control 0x20), d100 1: a step", [&] { const combat::Plan p = plan(0, {1}); return p.act == combat::Act::Step && p.dir == toward; }());
    scene(B, 2, 3);
    b.f[B].rec[0xF7] = 0x80 | 10;
    check("a monster (control 0x8A), d100 1: a step (the monsters' side)",
          [&] { const combat::Plan p = plan(B, {1}); return p.act == combat::Act::Step && p.dir == toward; }());
    // the magic-user (B, a monster: its NPC roll 100 first)
    auto mage = [&](const char* what, int dist, bool armour, int want_dir, std::function<void(combat::Fighter&)> set) {
        scene(B, 2, dist);
        combat::Fighter& f = b.f[B];
        f.rec[0xF7] = 0x80 | 50;
        f.rec[0x75] = 5;
        if (armour && armour_ty >= 0) {
            memset(inv[0], 0, sizeof inv[0]);
            inv[0][0x2E] = static_cast<uint8_t>(armour_ty);
            inv[0][0x34] = 1;
            f.items = inv;
            f.n_items = 1;
        }
        if (set) set(f);
        const combat::Plan p = plan(B, {100});
        snprintf(w, sizeof w, "a magic-user, %s (distance %d%s): act %d dir %d, want %s %d", what, dist, armour ? ", armour" : "", (int)p.act, p.dir,
                 want_dir < 0 ? "the guard" : "a step", want_dir);
        check(w, want_dir < 0 ? p.act == combat::Act::Guard : (p.act == combat::Act::Step && p.dir == want_dir));
        f.items = nullptr;
        f.n_items = 0;
    };
    const bool known = mage_armour >= 0;
    const bool keep = !strcmp(mage_then, "keep"), dual = !strcmp(mage_then, "dual");
    if (known) {
        mage("class 5, no armour", 5, false, keep ? toward : -1, nullptr);
        if (keep) {
            mage("class 5, no armour", 3, false, away, nullptr);
            mage("class 5, no armour", 2, false, away, nullptr);
        }
        if (armour_ty >= 0) mage("class 5", 3, true, mage_armour ? toward : -1, nullptr);
        if (dual) {
            auto human = [](int former) {
                return [former](combat::Fighter& f) {
                    for (int k = 0; k < 7; ++k) f.rec[0x109 + k] = 0;
                    f.rec[0x74] = 7;
                    f.rec[0x10E] = 6;
                    f.rec[0xE6] = static_cast<uint8_t>(former);
                };
            };
            mage("a human past its former class's level (6 > 4)", 3, false, toward, human(4));
            mage("a human not past it (6, former 6)", 3, false, -1, human(6));
        }
    }
    {
        scene(B, 2, 3);
        b.f[B].rec[0xF7] = 0x80 | 50;
        const combat::Plan p = plan(B, {100});
        check("class 2, no armour: a step", p.act == combat::Act::Step && p.dir == toward);
    }
    // the morale figure: control 0xC0 ((0x40) x 2 = 128), unhurt
    bool mok = morale_cap >= 0;
    if (mok) {
        scene(B, 2, 3);
        b.f[B].rec[0xF7] = 0xC0;
        b.enemy_health = 60;
        b.morale_fig = 0;
        (void)combat::morale(b, t, B);
        mok = b.morale_fig == (morale_cap ? 60 : 128);
        snprintf(w, sizeof w, "morale: control 0xC0 unhurt - figure %d, want %d (cap %d)", b.morale_fig, morale_cap ? 60 : 128, morale_cap);
        check(w, mok);
    }
    for (int y = 0; y < combat::kH; ++y)
        for (int x = 0; x < combat::kW; ++x) b.ground[y][x] = floor;
    for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
    combat::occupancy(b);
    expect(P, "the step routine's NPC and magic-user tests and the morale routine's 102 test, as each original's (oracle_aistep: npc / mage / morale)",
           bad == 0 && known && morale_cap >= 0, "facts npc 0x%X mage armour %d then '%s' morale cap %d; %d of %d wrong (%s)", npc_ctrl, mage_armour,
           mage_then, morale_cap, bad, n, first);
}

// Champions' computer steps (round 11): its step try (GAME.OVR 0xCC6A) reads
// a row by combat byte 0x15 - "mov al,[di+X]" after the mood x 5 + column -
// found in GAME.OVR by that code, the row read from the program's data
// segment there (not the profile's address). For each mood 1-6 and each way
// to the target, the first k - 1 directions of its row walled off: the step
// is the row's k-th (+ the way, & 7); one back the way it came last: the
// mood the next row (m % 6 + 1, 0xD0F0), the second time the target dropped
// (0xD11D), the third its moving over (0xD13E). And the engine reads the
// same row.
void ai_mood_steps()
{
    // Round 15: every DAX game. The oracle (tools/devkit/oracles/aistep.py)
    // reads each GAME.OVR's try and step: the rows' DS place, the flee's
    // facing (+ 1), the cost test, the cloud squares' rules
    static const char* kName[] = {"", "poolrad", "curse", "secret", "darkness", "champions", "knights", "queen", "gateway", "treasures"};
    const int g = game();
    if (g < 1 || g > 9 || !fight()) return;
    char path[256];
    snprintf(path, sizeof path, "%s/oracle_aistep_%s.txt", getenv("ORACLE_DIR") ? getenv("ORACLE_DIR") : "/tmp/claude-0/bt", kName[g]);
    FILE* fo = fopen(path, "r");
    if (!fo) {
        expect(P, "the computer's step: oracle_aistep (tools/devkit/oracles/aistep.py; the Dark Queen's qstep.py)", false, "%s missing", path);
        return;
    }
    // (round 19: the Dark Queen's rows are 6 bytes - its oracle gives the
    // 7 rows' columns 1-5 as rowbytes, read from DQK.EXE)
    uint8_t qrow[36] = {};
    int stride = 5, nq = 0;
    // (round 19: the step routine's NPC and magic-user tests, the morale
    // routine's 102 test)
    int npc_ctrl = -1, mage_armour = -1, morale_cap = -1;
    char mage_then[16] = "";
    struct Cl {
        char kind[16];
        std::vector<int> eff;
        int driven, save, hd7, flags2;
    };
    std::vector<Cl> cls;
    unsigned at = 0, facing = 0;
    int inc = -1;
    char cost[16] = "";
    char line[240];
    while (fgets(line, sizeof line, fo)) {
        unsigned x;
        int v;
        unsigned y1, y2;
        int v2;
        char w1[16];
        if (sscanf(line, "npc %x fig %x health %x side1 %d", &x, &y1, &y2, &v2) == 4) npc_ctrl = static_cast<int>(x);
        else if (sscanf(line, "mage class %x armour %d then %15s", &x, &v, w1) == 3) mage_armour = v, snprintf(mage_then, sizeof mage_then, "%s", w1);
        else if (sscanf(line, "morale fig %x cap %d", &x, &v) == 2) morale_cap = v;
        else if (sscanf(line, "rows %x stride %d", &x, &v) == 2) at = x, stride = v;
        else if (sscanf(line, "rows %x", &x) == 1) at = x;
        else if (!strncmp(line, "rowbytes ", 9)) {
            char* q = line + 9;
            for (int m = 0; m < 7; ++m)
                for (int c = 1; c <= 5; ++c) {
                    char* e;
                    const long b = strtol(q, &e, 16);
                    if (e == q) continue;
                    q = e;
                    qrow[m * 5 + c] = static_cast<uint8_t>(b);
                    ++nq;
                }
        }
        else if (sscanf(line, "facing %x inc %d", &x, &v) == 2) facing = x, inc = v;
        else if (sscanf(line, "cost %15s", cost) == 1) {
        } else if (!strncmp(line, "cloud ", 6)) {
            Cl c{};
            char key[16], effs[120] = "";
            const char* e = strstr(line, " eff ");
            const char* dr = strstr(line, " driven ");
            if (!e || !dr || sscanf(line, "cloud %15s %15s", c.kind, key) != 2) continue;
            snprintf(effs, sizeof effs, "%.*s", static_cast<int>(dr - e - 5), e + 5);
            for (char* tok = strtok(effs, " "); tok; tok = strtok(nullptr, " "))
                if (strcmp(tok, "-")) c.eff.push_back(static_cast<int>(strtol(tok, nullptr, 16)));
            sscanf(dr, " driven %d save %d hd7 %d flags2 %x", &c.driven, &c.save, &c.hd7, &x);
            c.flags2 = static_cast<int>(x);
            cls.push_back(c);
        }
    }
    fclose(fo);
    find_table();
    combat::Battle& b = fg->b;
    const auto& t = fg->t;
    const int base_at = static_cast<int>(d->prof->create.ds_image) + static_cast<int>(at);     // (the fight tables' image - load_fight_tables)
    const bool queen_rows = stride == 6;
    if (queen_rows ? nq != 35 : (!at || prog.empty() || base_at + 36 > static_cast<int>(prog.size()))) {
        expect(P, "the computer's step: the rows in the program", false, "rows 0x%X stride %d (%d bytes), program %d bytes", at, stride, nq,
               (int)prog.size());
        return;
    }
    const uint8_t* row = queen_rows ? qrow : &prog[base_at];
    bool same = t.mood_known;
    for (int k = 0; same && k < 36; ++k) same = (queen_rows && k == 0) || t.mood[k] == row[k];
    const uint8_t floor = b.ground[3][5];
    auto clear = [&]() {
        for (int y = 0; y < combat::kH; ++y)
            for (int x = 0; x < combat::kW; ++x) b.ground[y][x] = floor;
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
    };
    // the one stepping: B at 10,10 (a monster), its enemy the party's 0 three squares away the way dir
    auto setup = [&](int dir, int mood) {
        clear();
        mx::put(0, 0, 10 + 3 * combat::dx(dir), 10 + 3 * combat::dy(dir), 60);
        mx::put(B, 1, 10, 10, 60);
        combat::occupancy(b);
        combat::Fighter& f = b.f[B];
        f.ai_mood = static_cast<uint8_t>(mood);
        f.moves = 12;
        f.target = 0;
        f.rec[0x10E] = 0;
        f.rec[0x75] = 2;                // (not a magic-user: round 19's step tests)
        f.driven = false;
    };
    auto sq = [&](int dir) -> uint8_t& { return b.ground[10 + combat::dy(dir)][10 + combat::dx(dir)]; };
    int bad = 0, n = 0;
    char first[200] = "";
    for (int mood = 1; mood <= 6; ++mood)
        for (int dir = 0; dir < 8; ++dir)
            for (int k = 1; k <= 5; ++k) {
                setup(dir, mood);
                for (int j = 1; j < k; ++j) sq((row[mood * 5 + j] + dir) & 7) = 0;      // (no floor: no step there)
                const int want = (row[mood * 5 + k] + dir) & 7;
                const combat::Plan p = combat::think(b, t, B, rng);
                ++n;
                if ((p.act != combat::Act::Step || p.dir != want) && !bad++)
                    snprintf(first, sizeof first, "mood %d way %d column %d: act %d dir %d, want a step %d", mood, dir, k, (int)p.act, p.dir, want);
            }
    expect(P, "the computer's step - its mood's row of the program's table, the first it can take", same && bad == 0,
           "engine's rows the program's %d (table DS 0x%X); %d of %d wrong (%s)", same, at, bad, n, first);
    // back the way it came: the mood turns, then the target goes (and another
    // is looked for - none: it guards), then the moving ends
    int rev_bad = 0;
    char rev[200] = "";
    // (the second time's finder finding none - 0xCC32 - can't come about
    // here: the turn's own target pick just found one in sight)
    for (int alone = 0; alone < 1; ++alone) {
        setup(2, 3);
        combat::Fighter& f = b.f[B];
        for (int time = 1; time <= 3; ++time) {
            const int m0 = f.ai_mood;
            const int dir_now = (row[m0 * 5 + 1] + 2) & 7;
            f.flee_last = static_cast<uint8_t>((dir_now + 4) & 7);
            f.target = 0;
            f.moves = 12;
            if (alone && time == 2) b.f[0].size = 0;            // (no enemy left to find)
            const combat::Plan p = combat::think(b, t, B, rng);
            bool ok;
            if (time == 1) ok = p.act == combat::Act::Step && f.ai_mood == m0 % 6 + 1 && f.flee_back == 1 && f.target == 0;
            else if (time == 2 && !alone) ok = p.act == combat::Act::Step && f.flee_back == 2 && f.target == 0;
            else ok = p.act != combat::Act::Step && f.moves == 0;
            if (!ok && !rev_bad++)
                snprintf(rev, sizeof rev, "alone %d time %d: act %d mood %d->%d back %d target %d moves %d", alone, time, (int)p.act, m0,
                         (int)f.ai_mood, (int)f.flee_back, f.target, f.moves);
            if (alone && time == 2) break;
            f.ai_mood = static_cast<uint8_t>(m0);           // (the same row again for the next time)
        }
    }
    expect(P, "a step back the way it came - the next row, the target dropped and looked for again the second time, the moving over the third",
           rev_bad == 0, "%s", rev);
    // the cost test: the moves exactly the first column's cost
    int cost_bad = 0;
    char cst[200] = "";
    for (int dir = 0; dir < 8; ++dir) {
        for (int less = 0; less < 2; ++less) {
            setup(dir, 1);
            for (int j = 2; j <= 5; ++j) sq((row[5 + j] + dir) & 7) = 0;
            const int d1 = (row[6] + dir) & 7;
            const int c1 = combat::step_cost(b, t, B, d1, nullptr, nullptr);
            combat::Fighter& f = b.f[B];
            f.moves = static_cast<int16_t>(c1 - less);
            const combat::Plan p = combat::think(b, t, B, rng);
            const bool want = !strcmp(cost, "pool") ? !less : false;
            if ((p.act == combat::Act::Step) != want && !cost_bad++)
                snprintf(cst, sizeof cst, "way %d, cost %d, moves %d: act %d (want %s)", dir, c1, f.moves, (int)p.act, want ? "a step" : "none");
        }
    }
    expect(P, "the computer's step: its cost against the moves left as the original's try (strict: cost < moves; PoolRad: the ground < moves, cost <= moves)",
           cost_bad == 0 && cost[0], "cost rule '%s': %s", cost, cst);
    // the flee's way: the party's facing at the fight's start
    {
        const int fd = d->gs.dir & 6;
        const int want_way = (fd - ((fd + 2) % 4) / 2 + (inc > 0 ? 1 : 0) + 8) & 7;
        int fbad = 0;
        char fl[200] = "";
        for (int team = 0; team < 2; ++team)
            for (int k = 0; k < 6; ++k) {
                clear();
                const int me = team ? B : 0, foe = team ? 0 : B;
                mx::put(me, team, 25, 12, 60);
                mx::put(foe, !team, 25 + 6, 12 + 6, 60);
                combat::occupancy(b);
                combat::Fighter& f = b.f[me];
                f.fleeing = true;
                f.moves = 12;
                const combat::Plan p = combat::think(b, t, me, rng);
                const int r = f.ai_mood;
                const int want = (row[r * 5 + 1] + want_way + (team ? 0 : 4)) & 7;
                if ((p.act != combat::Act::Step || p.dir != want || r < 1 || r > 2) && !fbad++)
                    snprintf(fl, sizeof fl, "side %d facing %d: act %d dir %d mood %d, want %d (way %d)", team, fd, (int)p.act, p.dir, r, want, want_way);
            }
        expect(P, "a run's step: a d2 mood, the way from the party's facing (facing DS place, + 1 by the oracle), + 4 for the party",
               b.flee_dir == want_way && fbad == 0 && inc >= 0, "flee_dir %d want %d (facing DS 0x%X inc %d); %s", b.flee_dir, want_way, facing, inc, fl);
    }
    // the cloud squares: the first column a cloud, the others no floor
    int cbad = 0, cn = 0;
    char cb[240] = "";
    for (const Cl& c : cls) {
        const uint8_t cg = !strcmp(c.kind, "stink") ? (b.fx && b.fx->cloud_ground ? b.fx->cloud_ground : combat::kCloudGround)
                           : !strcmp(c.kind, "poison") ? (b.fx && b.fx->poison_ground ? b.fx->poison_ground : combat::kPoisonGround)
                           : (b.fx ? b.fx->dk.knives : 0);
        // the cases: what, how to set it, stepping wanted
        for (int cs = 0; cs < 8 + static_cast<int>(c.eff.size()); ++cs) {
            setup(2, 1);
            for (int j = 2; j <= 5; ++j) sq((row[5 + j] + 2) & 7) = 0;
            const int d1 = (row[6] + 2) & 7;
            sq(d1) = cg;
            combat::Fighter& f = b.f[B];
            f.rec[0xE5] = 1;
            bool want = false;
            const char* what = "";
            switch (cs) {
            case 0: what = "nothing, the save failed"; mx::dice(1); break;
            case 1: what = "the save made"; mx::dice(20); want = c.save != 0; break;
            case 2: what = "driven"; f.driven = true; mx::dice(1); want = c.driven != 0; break;
            case 3: what = "7 Hit Dice"; f.rec[0xE5] = 7; mx::dice(1); want = c.hd7 != 0; break;
            case 4: what = "6 Hit Dice"; f.rec[0xE5] = 6; mx::dice(1); break;
            case 5: what = "flags2 0x18"; f.flags2 = 0x18; mx::dice(1); want = (c.flags2 & 0x18) != 0; break;
            case 6: what = "flags2 0x10"; f.flags2 = 0x10; mx::dice(1); want = (c.flags2 & 0x10) != 0; break;
            case 7: what = "flags2 0x08"; f.flags2 = 0x08; mx::dice(1); want = (c.flags2 & 0x08) != 0; break;
            default:
                what = "an effect";
                mx::add_aff(B, c.eff[cs - 8]);
                mx::dice(1);
                want = true;
                break;
            }
            const combat::Plan p = combat::think(b, t, B, rng);
            mx::dice_off();
            ++cn;
            const bool stepped = p.act == combat::Act::Step && p.dir == d1;
            if (stepped != want && !cbad++)
                snprintf(cb, sizeof cb, "%s (ground 0x%X): %s%s - act %d dir %d, want %s", c.kind, cg, what,
                         cs >= 8 ? "" : "", (int)p.act, p.dir, want ? "the step" : "none");
            if (cs >= 8 && cbad == 1 && !stepped) snprintf(cb + strlen(cb), sizeof cb - strlen(cb), " (effect 0x%X)", c.eff[cs - 8]);
        }
    }
    for (int y = 0; y < combat::kH; ++y)
        for (int x = 0; x < combat::kW; ++x) b.ground[y][x] = floor;
    expect(P, "the computer's step onto a cloud square: the original's try (its effects, driven, flags 2, Hit Dice, a save) case by case",
           cbad == 0 && !cls.empty(), "%d of %d wrong (%s)", cbad, cn, cb);
    step19(row, npc_ctrl, mage_armour, mage_then, morale_cap);
}

// Round 15: Darkness's / Treasures' / Death Knights' Feeblemind, by their
// handlers (GAME.OVR Darkness 0x2E902 + effect 0x44's 0x1C2F3, Treasures
// 0x37E08 + 0x21F6x, Death Knights 0x2E610 + its effect 0x44 = drain-2's
// 0x1D3F8): the save failed (a d20 of 1) on B with a spell under way, one
// spell being memorized and one memorized, Intelligence / Wisdom in use 15
void feeblemind()
{
    const int g = game();
    if ((g != 4 && g != 9 && g != 6) || !fight()) return;
    combat::Battle& b = fg->b;
    const combat::FightSpell* fs = nullptr;
    for (int k = 0; k < d->prof->fight.n_spells; ++k)
        if (d->prof->fight.spells[k].does == combat::SpellDoes::Feeble) fs = &d->prof->fight.spells[k];
    if (fs && !mrules) load_magic();
    if (!fs || !mrules) {
        expect(P, "Feeblemind: the spell", false, "spell %d rules %d", fs != nullptr, mrules != nullptr);
        return;
    }
    const uint8_t drain2 = b.fx ? b.fx->mon.drain2 : 0;
    for (int fixes = 0; fixes < 2; ++fixes) {
        if (g != 6 && fixes) break;               // (Darkness / Treasures: no fix - the same either way)
        rules::set_fixes(fixes != 0);
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(0, 0, 10, 10, 60);
        mx::put(B, 1, 11, 10, 60);
        mx::put(1, 0, 12, 10, 60);                // (B's own target: party member 1)
        combat::Fighter& f = b.f[B];
        f.target = 1;
        f.spell = 0x0E;
        f.spell_n = 0;
        f.can_cast = true;
        f.rec[magic::kListAt + 10] = 0x85;
        f.rec[magic::kListAt + 11] = 0x07;
        f.rec[0x13] = f.rec[0x15] = 15;
        // (party member 1's levels, to see a drain)
        combat::Fighter& t1 = b.f[1];
        t1.rec[0xE5] = 5;
        const int hd_before = t1.rec[0xE5];
        uint8_t lv_before[8];
        for (int j = 0; j < 8; ++j) lv_before[j] = t1.rec[0x109 + j];
        fg->pages = fg->at = 0;
        fg->ptext_used = 0;
        create::g_die_hook = [](int sides) { return sides == 20 ? 0 : -1; };
        combat::SpellLine lines[8];
        const int tg[1] = {B};
        const int n = combat::cast(b, mrules->tables, 0, fs->spell, *fs, tg, 1, rng, lines, 8, 9);
        create::g_die_hook = nullptr;
        const int fk = q_find(f, 0x44);
        const bool kept = fk >= 0 && f.aff[fk][1] == 0 && f.aff[fk][2] == 0;
        bool lv_same = true;
        for (int j = 0; j < 8; ++j) lv_same = lv_same && t1.rec[0x109 + j] == lv_before[j];
        const bool drained = t1.rec[0xE5] != hd_before || !lv_same;
        if (g == 6 && !fixes) {
            // the original's: effect 0x44 (its drain-2) for ever, the target's
            // target drained, "gets drained" said of it; Int / Wis, the spell
            // and the list untouched
            bool said = false;
            for (int k = 0; k < n; ++k) said = said || (lines[k].who == 1 && lines[k].did == combat::Did::Word2);
            expect(P, "Death Knights: Feeblemind as its handler (GAME.OVR 0x2E610 / 0x1D3F8) - its drain-2 effect 0x44 for ever, the target's own target drained 2 levels (\"gets drained\"), nothing else (Game Fixes off)",
                   drain2 == 0x44 && kept && drained && said && f.rec[0x13] == 15 && f.rec[0x15] == 15 && f.spell == 0x0E &&
                       f.rec[magic::kListAt + 10] == 0x85,
                   "drain2 0x%X, effect %d, drained %d (HD %d->%d), said %d, Int %d Wis %d, spell %d, list %02x", drain2, kept, drained,
                   hd_before, int(t1.rec[0xE5]), said, int(f.rec[0x13]), int(f.rec[0x15]), f.spell, int(f.rec[magic::kListAt + 10]));
        } else {
            const bool ints = f.rec[0x13] == 3 && f.rec[0x15] == 3;
            const bool list = f.rec[magic::kListAt + 10] == 0 && f.rec[magic::kListAt + 11] == 0x07;
            bool said = false;
            for (int k = 0; k < n; ++k) said = said || (lines[k].who == B && lines[k].did == combat::Did::Word);
            char want[48];
            ow(fs->word, want, sizeof want);
            const bool eff_ok = g == 6 ? fk < 0 : kept;      // (Death Knights' fix: no effect - its 0x44 is the drain)
            expect(P, g == 6 ? "Death Knights with the Game Fixes: Feeblemind as Darkness's - Intelligence / Wisdom 3 at once, the spells being memorized forgotten, no effect, no drain"
                             : "Feeblemind as its handler - effect 0x44 for ever, Intelligence / Wisdom in use 3 at once, the spells being memorized forgotten, the spell under way left for \"lost a spell\", \"loses intellect\"",
                   eff_ok && ints && list && !drained && (g == 6 || (said && want[0] && f.spell == 0x0E)),
                   "effect %d (kept %d), Int %d Wis %d, list %02x %02x, drained %d, said %d \"%s\", spell %d; %d lines (first %d did %d)", fk >= 0, kept, int(f.rec[0x13]),
                   int(f.rec[0x15]), int(f.rec[magic::kListAt + 10]), int(f.rec[magic::kListAt + 11]), drained, said, want, f.spell, n,
                   n ? lines[0].who : -1, n ? (int)lines[0].did : -1);
        }
        combat::lose_aff(f, 0x44);
        f.rec[magic::kListAt + 11] = 0;
        f.spell = 0;
    }
    rules::set_fixes(true);
    // Darkness / Treasures: the stats' recalculation (GAME.OVR 0x3307E /
    // 0x3CD87) keeps Intelligence / Wisdom at 3 while effect 0x44 lasts
    if (g != 6) {
        party::Character c = pt->m[0];
        c.n_affects = 0;
        c.rec[0x12] = c.rec[0x14] = 15;
        add_aff(c, 0x44, 0, 9);
        rules::stats(c, d->facts);
        const int i3 = c.rec[0x13], w3 = c.rec[0x15];
        c.n_affects = 0;
        rules::stats(c, d->facts);
        expect(P, "Feeblemind: the stats' recalculation keeps Intelligence / Wisdom in use 3 while effect 0x44 lasts (0x3307E / 0x3CD87), back after",
               i3 == 3 && w3 == 3 && c.rec[0x13] == 15 && c.rec[0x15] == 15, "with %d / %d, without %d / %d", i3, w3, int(c.rec[0x13]),
               int(c.rec[0x15]));
    }
}

// Death Knights' on-hit specials (round 16): oracle_knights_hit.txt from its
// GAME.OVR (tools/devkit/oracles/knights_hit.py - the hook switch's events
// 2 / 3 and the blow's damage test before them). The party's own holder of
// its drain-2 effect (0x44 - the feebleminded's) drains whoever it hits,
// attack 1 or 2 as the lists say; a blow of no damage drains nobody
void knights_hit()
{
    if (game() != 6) return;
    char path[256];
    snprintf(path, sizeof path, "%s/oracle_knights_hit.txt", getenv("ORACLE_DIR") ? getenv("ORACLE_DIR") : "/tmp/claude-0/bt");
    FILE* fo = fopen(path, "r");
    if (!fo) {
        expect(P, "Death Knights' on-hit: oracle_knights_hit (tools/devkit/oracles/knights_hit.py)", false, "%s missing", path);
        return;
    }
    bool ev2 = false, ev3 = false;
    int need = -1;
    char line[200];
    while (fgets(line, sizeof line, fo)) {
        if (!strncmp(line, "event2 ", 7)) ev2 = strstr(line, "0x44") != nullptr;
        if (!strncmp(line, "event3 ", 7)) ev3 = strstr(line, "0x44") != nullptr;
        if (sscanf(line, "needs_damage %d", &need) == 1) continue;
    }
    fclose(fo);
    if (!fight() || !fg->b.fx || fg->b.fx->mon.drain2 != 0x44) {
        expect(P, "Death Knights' on-hit: its drain-2 effect 0x44", false);
        return;
    }
    combat::Battle& b = fg->b;
    int n = 0, bad = 0;
    char first[200] = "";
    auto blow = [&](int slot, int dice) {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(0, 0, 10, 10, 60);
        mx::put(B, 1, 11, 10, 200);
        F(B).rec[0xE5] = 9;
        q_add(F(0), 0x44, 0);
        combat::Fighter& at = F(0);
        at.rec[0x199] = 60;
        at.attacks[0] = slot == 0 ? 1 : 0;
        at.attacks[1] = slot == 1 ? 1 : 0;
        for (int s2 = 0; s2 < 2; ++s2) {
            at.rec[0x19E + s2] = static_cast<uint8_t>(dice);
            at.rec[0x1A0 + s2] = 6;
            at.rec[0x1A2 + s2] = 0;
        }
        create::g_die_hook = [](int nn) { return nn == 20 ? 18 : -1; };
        const combat::Attack r = combat::attack(b, 0, B, nullptr, rng);
        create::g_die_hook = nullptr;
        bool drained = false;
        for (int k = 0; k < r.n_ev; ++k)
            drained = drained || (r.ev[k].who == B && (r.ev[k].ev == combat::Ev::Drained || r.ev[k].ev == combat::Ev::DkDrained));
        return drained;
    };
    auto check = [&](const char* what, bool got, bool want) {
        ++n;
        if (got != want && !bad++) snprintf(first, sizeof first, "%s: drained %d, want %d", what, got, want);
    };
    for (int fixes = 0; fixes < 2; ++fixes) {
        rules::set_fixes(fixes != 0);
        check("attack 1, damage", blow(0, 1), ev2);
        check("attack 2, damage", blow(1, 1), ev3);
        check("attack 1, no damage", blow(0, 0), need == 1 ? false : ev2);
    }
    rules::set_fixes(true);
    expect(P, "Death Knights: a holder of its drain-2 effect (the party's feebleminded too) drains whoever its damaging blow hits (oracle_knights_hit: events 2 / 3, the damage test)",
           bad == 0 && need >= 0, "%d of %d wrong (%s)", bad, n, first);
}

// The second step back's finder (round 16): oracle_finder_<game>.txt from
// GAME.OVR (tools/devkit/oracles/finder.py - the count-2 call's finder and
// its passes). combat::finder_any with set d-rolls: none in sight - one is
// still taken (pass 2 skips the sight test); random-pick games: a pick of one
// already struck spends a try (round 17: not the pass's end - turnfind) and
// pass 2 may take an unseen one beside a seen one; scan games
// (Darkness-type): from any start the seen one is found
int fd_seq[8], fd_at = 0;
int fd_die(int n) { return fd_at < 8 && fd_seq[fd_at] > 0 ? (fd_seq[fd_at++] - 1) % n : 0; }
void finder_check()
{
    static const char* kName[] = {"", "poolrad", "curse", "secret", "darkness", "champions", "knights", "queen", "gateway", "treasures"};
    const int g = game();
    if (g == 7 || g < 1 || g > 9) return;
    char path[256];
    snprintf(path, sizeof path, "%s/oracle_finder_%s.txt", getenv("ORACLE_DIR") ? getenv("ORACLE_DIR") : "/tmp/claude-0/bt", kName[g]);
    FILE* fo = fopen(path, "r");
    if (!fo) {
        expect(P, "the step back's finder: oracle_finder (tools/devkit/oracles/finder.py)", false, "%s missing", path);
        return;
    }
    int tries = -1, scan = -1, skip = -1;
    char line[240];
    while (fgets(line, sizeof line, fo)) {
        const char* q = strstr(line, "tries ");
        if (q) tries = atoi(q + 6);
        if ((q = strstr(line, "scan "))) scan = atoi(q + 5);
        if ((q = strstr(line, "pass2skip "))) skip = atoi(q + 10);
    }
    fclose(fo);
    if (!fight() || !fg->b.fx || !fg->b.fx->invisible) return;
    combat::Battle& b = fg->b;
    const uint8_t inv = b.fx->invisible;
    auto scene = [&]() {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(B, 1, 10, 10, 60);
        mx::put(0, 0, 3, 3, 60);
        mx::put(1, 0, 16, 3, 60);
        F(B).target = -1;
    };
    auto run = [&](std::initializer_list<int> rolls) {
        fd_at = 0;
        int k = 0;
        for (int r : rolls) fd_seq[k++] = r;
        while (k < 8) fd_seq[k++] = 1;
        create::g_die_hook = fd_die;
        F(B).target = -1;                   // (the count-2 call: its target dropped before)
        const int got = combat::finder_any(b, fg->t, B, rng);
        create::g_die_hook = nullptr;
        return got;
    };
    int n = 0, bad = 0;
    char first[200] = "";
    auto check = [&](const char* what, int got, int want) {
        ++n;
        if (got != want && !bad++) snprintf(first, sizeof first, "%s: %d, want %d", what, got, want);
    };
    // nobody in sight: pass 2 takes its pick all the same
    scene();
    q_add(F(0), inv, 0);
    q_add(F(1), inv, 0);
    // (pass 1: a scan takes one roll, the random picks one each - here two, both struck)
    check("none in sight, pass 2 picks the first", scan == 1 ? run({1, 1}) : run({1, 2, 1}), skip ? 0 : -1);
    check("none in sight, pass 2 picks the second", scan == 1 ? run({1, 2}) : run({1, 2, 2}), skip ? 1 : -1);
    if (scan == 1) {
        // Darkness-type: 0 unseen, 1 seen - from either start the scan finds 1
        scene();
        q_add(F(0), inv, 0);
        check("scan from the unseen one", run({1}), 1);
        check("scan from the seen one", run({2}), 1);
    } else {
        scene();
        q_add(F(0), inv, 0);
        check("the seen one picked", run({2}), 1);
        // the unseen one picked, struck, picked again (each a try) to the
        // tries' end - pass 2 takes it unseen
        check("a struck one picked again: pass 2, its pick unseen", run({1, 1, 1}), skip ? 0 : 1);
        check("the unseen one struck, then the seen one", run({1, 2}), 1);
    }
    expect(P, "the second step back's finder as the original's (oracle_finder: its passes, the second without the sight test)",
           bad == 0 && tries >= 0 && skip == 1, "tries %d scan %d skip %d; %d of %d wrong (%s)", tries, scan, skip, bad, n, first);
}

// The computer's target at its turn (round 17): oracle_turnfind_<game>.txt
// from GAME.OVR (tools/devkit/oracles/turnfind.py - the AI loop's finder
// call, the turn's first, the finder's keep step, a struck pick's branch,
// the first pass's walls, Champions' taunt and the moves test's place).
// combat::finder_any / think with set d-rolls on a cleared field with one
// wall square: the target kept behind the wall and out of reach; an
// invisible one dropped; the first pass's list only those in a sight line;
// a struck pick spending its try; Champions' kender first in the list taken
// without a roll, not otherwise; Darkness-type: no finder at moves 0; the
// turn's first finder where the original has one
void turnfind_check()
{
    static const char* kName[] = {"", "poolrad", "curse", "secret", "darkness", "champions", "knights", "queen", "gateway", "treasures"};
    const int g = game();
    if (g == 7 || g < 1 || g > 9) return;
    char path[256];
    snprintf(path, sizeof path, "%s/oracle_turnfind_%s.txt", getenv("ORACLE_DIR") ? getenv("ORACLE_DIR") : "/tmp/claude-0/bt", kName[g]);
    FILE* fo = fopen(path, "r");
    if (!fo) {
        expect(P, "the computer's turn target: oracle_turnfind (tools/devkit/oracles/turnfind.py)", false, "%s missing", path);
        return;
    }
    char line[400] = "";
    while (fgets(line, sizeof line, fo) && line[0] == '#') {}
    fclose(fo);
    const bool moves_first = strstr(line, "order moves_first") != nullptr;
    const bool early = strstr(line, "early 0x") != nullptr;
    const bool facts_ok = strstr(line, "seen 1") && strstr(line, "struck continue") && strstr(line, "pass1walls 1");
    int taunt = 0;
    if (const char* q = strstr(line, "taunt ")) taunt = static_cast<int>(strtol(q + 6, nullptr, 16));
    if (!fight() || !fg->b.fx || !fg->b.fx->invisible) return;
    combat::Battle& b = fg->b;
    const combat::Tables& t = fg->t;
    const uint8_t inv = b.fx->invisible;
    // a wall: a ground value whose obstacle stands over the floor's eye
    const uint8_t floor = b.ground[10][10];
    int wall = -1;
    for (int v = 1; v < combat::kGroundValues && wall < 0; ++v)
        if (t.ground[v][2] > t.ground[floor][1]) wall = v;
    auto scene = [&](bool walled) {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        b.ground[10][7] = floor;
        mx::put(B, 1, 10, 10, 60);
        mx::put(0, 0, 4, 10, 60);
        mx::put(1, 0, 16, 10, 60);
        if (walled && wall > 0) b.ground[10][7] = static_cast<uint8_t>(wall);
        F(B).target = -1;
        b.taunter = -1;
    };
    auto run = [&](std::initializer_list<int> rolls) {
        fd_at = 0;
        int k = 0;
        for (int r : rolls) fd_seq[k++] = r;
        while (k < 8) fd_seq[k++] = 1;
        create::g_die_hook = fd_die;
        const int got = combat::finder_any(b, t, B, rng);
        create::g_die_hook = nullptr;
        return got;
    };
    int n = 0, bad = 0;
    char first[200] = "";
    auto check = [&](const char* what, int got, int want) {
        ++n;
        if (got != want && !bad++) snprintf(first, sizeof first, "%s: %d, want %d", what, got, want);
    };
    scene(true);
    const bool walled = wall > 0 && !combat::range(b, t, B, 0, false, nullptr) && combat::range(b, t, B, 1, false, nullptr);
    if (walled) {
        // kept behind the wall (no sight line, no range in the keep step)
        F(B).target = 0;
        check("target kept behind a wall", run({2}), 0);
        F(B).target = -1;
        // no target: pass 1's list is member 1 alone (member 0 behind the wall)
        check("pass 1: only those in a sight line", run({1}), 1);
        // the one in sight invisible: struck, pass 2 takes either (walls passed over)
        scene(true);
        mx::add_aff(1, inv);
        check("pass 2 over the wall, its first", run({1, 1}), 0);
        F(B).target = -1;
        check("pass 2 over the wall, its second", run({1, 2}), 1);
    }
    // an invisible target dropped
    scene(false);
    mx::add_aff(1, inv);
    F(B).target = 1;
    check("an invisible target dropped", run({1}), 0);
    {
        // a struck pick spends its try: 0 unseen picked three times, then 1
        // (old reading: the pass over at the second, pass 2's roll 1 -> 0)
        scene(false);
        mx::add_aff(0, inv);
        check("a struck pick spends its try", t.finder_scan ? run({1}) : run({1, 1, 1, 2}), 1);
    }
    if (taunt && b.fx->taunted == taunt) {
        // Champions: the kender first in the list taken on the first try
        scene(false);
        mx::add_aff(B, taunt);
        b.taunter = 0;
        check("taunted: the kender first in the list, no roll", run({2}), 0);
        scene(false);
        mx::add_aff(B, taunt);
        b.taunter = 1;
        check("taunted: the kender second in the list, a roll", run({1}), 0);
    } else if (taunt && b.fx->taunted) {
        check("taunt effect as the oracle's", b.fx->taunted, taunt);
    }
    // (a finder with the taunt test in a game with no Yell - Secret: its
    // taunter word DS 0x8E23 is never written, so the list's first, a
    // fighter, never matches it - moot; round 18)
    // the moves test's place: think at moves 0 with no target
    scene(false);
    F(B).moves = 0;
    create::g_die_hook = fd_die;
    fd_at = 0;
    for (int k = 0; k < 8; ++k) fd_seq[k] = 1;
    (void)combat::think(b, t, B, rng);
    create::g_die_hook = nullptr;
    check("moves 0: the finder asked (Curse-type) or not (Darkness-type)", F(B).target >= 0, !moves_first);
    check("the turn's first finder", t.finder_first, early);
    scene(false);
    for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
    combat::occupancy(b);
    expect(P, "the computer's turn target as the original's (oracle_turnfind: kept while seen, the first pass in sight lines, struck picks, the moves' place, Champions' kender)",
           bad == 0 && facts_ok && walled, "facts %d walled %d; %d of %d wrong (%s)", facts_ok, walled, bad, n, first);
}

// The on-hit specials after a blow of no damage (round 17): oracle_onhit_
// <game>.txt (tools/devkit/oracles/onhit.py - each blow's test between the
// target's in-the-fight test and the hook event slot + 1). A hit (d20 18)
// with 0 dice and with 1d6 by one holding the game's poison (attack 2's -
// Gateway's attack 1's), the save a d20 of 1: poisoned after a damaging blow
// always, after one of none only where the original has no damage test; the
// Dark Queen's 0xB8 holder poisons with none too; Darkness-type's vorpal
// blow (attack 1's, a d20 of 20) the same
int oh_n20 = 0, oh_first = 18, oh_next = 1;
int oh_die(int nn) { return nn == 20 ? (oh_n20++ == 0 ? oh_first : oh_next) - 1 : -1; }
// The computer at moves 0 and its guard (round 18): oracle_guard_<game>.txt
// (tools/devkit/oracles/guard.py - each original's AI loop: the move-and-
// attack step only with moves over 0, else the guard routine; that routine's
// tests: the held list, a readied weapon of range over 1, the moves - any:
// the turn just ends, no guard). A cleared field, the monster next to member
// 0: think at moves 0 (and fleeing at moves 0) plans the guard, not the blow;
// guard_takes false at moves 0, for each effect of the list, with a readied
// missile weapon (a type of the game's own ITEMS, range over 1); true with
// moves and a melee weapon (range 1 or none). The Dark Queen also: its
// finder's facts (qfinder: as many tries as the list, a roll before them,
// the taunted kender moved to the front - taken without a roll at a try with
// 0x13 left only) and the taunted effect the engine gives
void guard0_check()
{
    static const char* kName[] = {"", "poolrad", "curse", "secret", "darkness", "champions", "knights", "queen", "gateway", "treasures"};
    const int g = game();
    if (g < 1 || g > 9) return;
    char path[256];
    snprintf(path, sizeof path, "%s/oracle_guard_%s.txt", getenv("ORACLE_DIR") ? getenv("ORACLE_DIR") : "/tmp/claude-0/bt", kName[g]);
    FILE* fo = fopen(path, "r");
    if (!fo) {
        expect(P, "the computer at moves 0 / its guard: oracle_guard (tools/devkit/oracles/guard.py)", false, "%s missing", path);
        return;
    }
    char line[400] = "", qline[400] = "";
    while (fgets(line, sizeof line, fo) && line[0] == '#') {}
    if (!fgets(qline, sizeof qline, fo)) qline[0] = 0;
    fclose(fo);
    const bool moves0 = strstr(line, "moves0 1") != nullptr, movetest = strstr(line, "movetest 1") != nullptr;
    const bool missile = strstr(line, "missile range") != nullptr;
    int eff[8], ne = 0;
    if (const char* q = strstr(line, "eff ")) {
        q += 4;
        while (ne < 8 && q[0] == '0' && q[1] == 'x') {
            char* e;
            eff[ne++] = static_cast<int>(strtol(q, &e, 16));
            q = e;
            while (*q == ' ') ++q;
        }
    }
    if (!fight() || !fg->b.fx || !fg->b.names) return;
    combat::Battle& b = fg->b;
    const combat::Tables& t = fg->t;
    const items::Names& nm = *b.names;
    int n = 0, bad = 0;
    char first[200] = "";
    auto check = [&](const char* what, bool ok) {
        ++n;
        if (!ok && !bad++) snprintf(first, sizeof first, "%s", what);
    };
    static uint8_t inv[1][items::kRecordSize];
    auto scene = [&]() {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(0, 0, 11, 10, 60);
        mx::put(B, 1, 10, 10, 60);
        F(B).items = nullptr;
        F(B).n_items = 0;
        b.taunter = -1;
    };
    auto plan = [&]() {
        create::g_die_hook = fd_die;
        fd_at = 0;
        for (int k = 0; k < 8; ++k) fd_seq[k] = 1;
        const combat::Plan p = combat::think(b, t, B, rng);
        create::g_die_hook = nullptr;
        return p.act;
    };
    scene();
    check("moves left, one next to it: the blow", plan() == combat::Act::Attack);
    scene();
    F(B).moves = 0;
    check("moves 0, one next to it: the guard (no blow)", (plan() == combat::Act::Guard) == moves0);
    scene();
    F(B).moves = 0;
    F(B).fleeing = true;
    check("fleeing at moves 0, one next to it: the guard", (plan() == combat::Act::Guard) == moves0);
    scene();
    F(B).moves = 0;
    check("the guard routine at moves 0: the turn just ends", combat::guard_takes(b, t, B) == !movetest);
    scene();
    check("the guard routine with moves, no weapon: guards", combat::guard_takes(b, t, B));
    // each effect of the original's list: no guard; Facts::held's others
    // (Death Knights' snake charm, 0x33): its holder has no turn
    for (int k = 0; k < ne; ++k) {
        scene();
        bool known = false;
        for (uint8_t h : b.fx->held) known = known || h == eff[k];
        mx::add_aff(B, eff[k]);
        char w2[80];
        snprintf(w2, sizeof w2, "effect 0x%02X of the list: no guard (and in Facts::held)", eff[k]);
        check(w2, known && !combat::guard_takes(b, t, B));
    }
    // a readied missile weapon (range over 1) / a melee one
    int far_ty = -1, near_ty = -1;
    for (int k = 1; k < 256; ++k) {
        const items::TypeInfo& ti = nm.type(k);
        if (ti.slot != items::kSlotWeapon) continue;
        if (ti.range > 1 && far_ty < 0) far_ty = k;
        if (ti.range <= 1 && near_ty < 0) near_ty = k;
    }
    for (int pass = 0; pass < 2; ++pass) {
        const int ty = pass ? near_ty : far_ty;
        if (ty < 0) continue;
        scene();
        memset(inv[0], 0, sizeof inv[0]);
        inv[0][0x2E] = static_cast<uint8_t>(ty);
        inv[0][0x34] = 1;
        F(B).items = inv;
        F(B).n_items = 1;
        check(pass ? "a melee weapon readied: guards" : "a missile weapon readied (range over 1): no guard",
              combat::guard_takes(b, t, B) == (pass ? true : !missile));
        F(B).items = nullptr;
        F(B).n_items = 0;
    }
    bool qok = true;
    char qwhy[120] = "";
    if (g == 7) {
        // the Dark Queen's finder: oracle facts and the engine's
        unsigned qt = 0, qat = 0;
        if (const char* q = strstr(qline, "taunt 0x")) qt = static_cast<unsigned>(strtol(q + 6, nullptr, 16));
        if (const char* q = strstr(qline, " at 0x")) qat = static_cast<unsigned>(strtol(q + 4, nullptr, 16));
        qok = strstr(qline, "tries list") && strstr(qline, "firstroll 1") && strstr(qline, "swap 1") && qat == 0x13 &&
              strstr(qline, "early 0x") && b.fx->taunted == qt && t.finder_queen && t.finder_first;
        if (!qok) snprintf(qwhy, sizeof qwhy, "qfinder facts: taunted %02X want %02X, finder_queen %d first %d", b.fx->taunted, qt,
                           t.finder_queen, t.finder_first);
        // two foes in sight, the first invisible: rolls 1 (the roll before
        // the tries), then 1 (struck), 2 (taken) - two tries, the list's two
        auto run = [&](std::initializer_list<int> rolls) {
            fd_at = 0;
            int k = 0;
            for (int r : rolls) fd_seq[k++] = r;
            while (k < 8) fd_seq[k++] = 1;
            create::g_die_hook = fd_die;
            const int got = combat::finder_any(b, t, B, rng);
            create::g_die_hook = nullptr;
            return got;
        };
        auto two = [&]() {
            scene();
            mx::put(1, 0, 16, 10, 60);
            F(B).target = -1;
        };
        two();
        mx::add_aff(0, b.fx->invisible);
        const int a1 = run({2, 1, 2});
        check("the Queen's finder: a roll before the tries, a struck pick, the next taken", a1 == 1);
        // a struck pick spends a try: list of 2, tries 2 - rolls (first) 1,
        // 1 (0 struck), 1 (struck again: spent) - pass 2 then (its roll 2,
        // then 2): member 1... with both unseen pass 2 takes its pick
        two();
        mx::add_aff(0, b.fx->invisible);
        mx::add_aff(1, b.fx->invisible);
        const int a2 = run({1, 1, 1, 1, 2});
        check("the Queen's finder: tries as many as the list, pass 2 takes its pick", a2 == 1);
        // taunted: the kender (member 1) moved to the front - a list of 2,
        // so no free take: the pick's roll still decides
        if (b.fx->taunted) {
            two();
            mx::add_aff(B, b.fx->taunted);
            b.taunter = 1;
            const int a3 = run({1, 2});
            check("the Queen's taunted finder: the kender first in its list (roll 2 -> member 0)", a3 == 0);
            two();
            mx::add_aff(B, b.fx->taunted);
            b.taunter = 1;
            const int a4 = run({1, 1});
            check("the Queen's taunted finder: roll 1 -> the kender", a4 == 1);
        }
    }
    scene();
    for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
    combat::occupancy(b);
    expect(P, "the computer at moves 0 and its guard as the original's (oracle_guard: the AI loop's moves test, the guard routine's held list / missile / moves; the Dark Queen's finder)",
           bad == 0 && qok && moves0 && movetest && missile && ne > 0, "facts moves0 %d movetest %d missile %d effects %d; %d of %d wrong (%s) %s",
           moves0, movetest, missile, ne, bad, n, first, qwhy);
}

// Round 19 (tools/devkit/oracles/yell.py): the Krynn games' Yell - each of
// the other side okay, Intelligence over 3, NOT HELD (the guard routine's
// own list test, the same far call: oracle_guard's effects) and failing a
// save against spells gets the taunted effect; the held asked before the
// save (no die for a held one). An effect of Facts::held outside the
// original's list (Death Knights' 0x33) doesn't keep one from it.
void yell_check()
{
    static const char* kName[] = {"", "poolrad", "curse", "secret", "darkness", "champions", "knights", "queen", "gateway", "treasures"};
    const int g = game();
    if (g < 5 || g > 7) return;
    const char* od = getenv("ORACLE_DIR") ? getenv("ORACLE_DIR") : "/tmp/claude-0/bt";
    char path[256], yl[300] = "", gl[400] = "";
    snprintf(path, sizeof path, "%s/oracle_yell_%s.txt", od, kName[g]);
    FILE* fo = fopen(path, "r");
    if (fo) {
        while (fgets(yl, sizeof yl, fo) && yl[0] == '#') {}
        fclose(fo);
    }
    snprintf(path, sizeof path, "%s/oracle_guard_%s.txt", od, kName[g]);
    fo = fopen(path, "r");
    if (fo) {
        while (fgets(gl, sizeof gl, fo) && gl[0] == '#') {}
        fclose(fo);
    }
    if (!yl[0] || !gl[0]) {
        expect(P, "the Yell: oracle_yell / oracle_guard (tools/devkit/oracles/yell.py, guard.py)", false, "missing for %s", kName[g]);
        return;
    }
    int eff[8], ne = 0;
    if (const char* q = strstr(gl, "eff ")) {
        q += 4;
        while (ne < 8 && q[0] == '0' && q[1] == 'x') {
            char* e;
            eff[ne++] = static_cast<int>(strtol(q, &e, 16));
            q = e;
            while (*q == ' ') ++q;
        }
    }
    unsigned taunted = 0;
    if (const char* q = strstr(yl, "taunted ")) taunted = static_cast<unsigned>(strtoul(q + 8, nullptr, 16));
    const bool same = strstr(yl, "guard_same 1") != nullptr, held_first = strstr(yl, "order held-save") != nullptr;
    if (!fight() || !fg->b.fx) return;
    combat::Battle& b = fg->b;
    int n = 0, bad = 0;
    char first[200] = "";
    auto check = [&](const char* what, bool ok) {
        ++n;
        if (!ok && !bad++) snprintf(first, sizeof first, "%s", what);
    };
    auto scene = [&]() {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(0, 0, 10, 10, 60);
        mx::put(B, 1, 14, 10, 60);
        F(B).rec[0x13] = 10;
        b.taunter = -1;
    };
    auto yell = [&]() {
        r19_at = 0;
        for (int k = 0; k < 8; ++k) r19_seq[k] = 1;          // (a d20 of 1: the save failed)
        create::g_die_hook = r19_die;
        int out[8];
        const int got = combat::taunt(b, 0, rng, out, 8);
        create::g_die_hook = nullptr;
        return got == 1 && out[0] == B && F(B).has(b.fx->taunted);
    };
    char w[120];
    check("the taunted effect as the original's", b.fx->taunted == taunted);
    scene();
    check("not held, the save failed: taunted", yell());
    for (int k = 0; k < ne; ++k) {
        scene();
        mx::add_aff(B, eff[k]);
        const bool t = yell();
        snprintf(w, sizeof w, "held (0x%02X of the guard routine's list): not taunted, no save rolled", eff[k]);
        check(w, !t && (!held_first || r19_at == 0));
    }
    for (uint8_t h : b.fx->held) {
        bool listed = false;
        for (int k = 0; k < ne; ++k) listed = listed || eff[k] == h;
        if (!h || listed) continue;
        scene();
        mx::add_aff(B, h);
        snprintf(w, sizeof w, "0x%02X (held here, not in the original's list): taunted", h);
        check(w, yell());
    }
    scene();
    F(B).rec[0x13] = 3;
    check("Intelligence 3: not taunted", !yell());
    for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
    combat::occupancy(b);
    expect(P, "the Yell as the original's (oracle_yell: the other side, okay, Intelligence over 3, not held - the guard routine's list - then the save)",
           bad == 0 && same && held_first && ne > 0, "facts guard_same %d held first %d effects %d; %d of %d wrong (%s)", same, held_first, ne, bad, n,
           first);
}

void onhit_check()
{
    static const char* kName[] = {"", "poolrad", "curse", "secret", "darkness", "champions", "knights", "queen", "gateway", "treasures"};
    const int g = game();
    if (g < 1 || g > 9) return;
    char path[256];
    snprintf(path, sizeof path, "%s/oracle_onhit_%s.txt", getenv("ORACLE_DIR") ? getenv("ORACLE_DIR") : "/tmp/claude-0/bt", kName[g]);
    FILE* fo = fopen(path, "r");
    if (!fo) {
        expect(P, "the on-hit specials' damage test: oracle_onhit (tools/devkit/oracles/onhit.py)", false, "%s missing", path);
        return;
    }
    char line[240] = "";
    while (fgets(line, sizeof line, fo) && line[0] == '#') {}
    fclose(fo);
    const char* q = strstr(line, "needs_damage ");
    const int need = q ? atoi(q + 13) : -1;
    int freefx = 0;
    if ((q = strstr(line, "free "))) freefx = static_cast<int>(strtol(q + 5, nullptr, 16));
    if (!fight() || !fg->b.fx) return;
    combat::Battle& b = fg->b;
    const combat::MonFx& mf = b.fx->mon;
    const bool ways = b.fx->gw.ways != 0;
    auto blow = [&](uint8_t eff, int slot, int dice, bool with_free, int second, combat::Ev want) {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(0, 0, 10, 10, 60);
        mx::put(B, 1, 11, 10, 200);
        mx::add_aff(0, eff);
        if (with_free && freefx) mx::add_aff(0, freefx);
        combat::Fighter& at = F(0);
        at.rec[0x199] = 60;
        at.attacks[0] = slot == 0 ? 1 : 0;
        at.attacks[1] = slot == 1 ? 1 : 0;
        for (int s2 = 0; s2 < 2; ++s2) {
            at.rec[0x19E + s2] = static_cast<uint8_t>(dice);
            at.rec[0x1A0 + s2] = 6;
            at.rec[0x1A2 + s2] = 0;
        }
        oh_n20 = 0;
        oh_next = second;
        create::g_die_hook = oh_die;
        const combat::Attack r = combat::attack(b, 0, B, nullptr, rng);
        create::g_die_hook = nullptr;
        bool got = false;
        for (int k = 0; k < r.n_ev; ++k) got = got || (r.ev[k].who == B && r.ev[k].ev == want);
        return got;
    };
    int n = 0, bad = 0;
    char first[200] = "";
    auto check = [&](const char* what, bool got, bool want) {
        ++n;
        if (got != want && !bad++) snprintf(first, sizeof first, "%s: %d, want %d", what, got, want);
    };
    for (int fixes = 0; fixes < 2; ++fixes) {
        rules::set_fixes(fixes != 0);
        if (mf.poison) {
            const int slot = ways ? 0 : 1;
            check("poison, a damaging blow", blow(mf.poison, slot, 1, false, 1, combat::Ev::Poisoned), true);
            check("poison, a blow of no damage", blow(mf.poison, slot, 0, false, 1, combat::Ev::Poisoned), need == 0);
            if (freefx)
                check("poison, no damage, the 0xB8 holder", blow(mf.poison, slot, 0, true, 1, combat::Ev::Poisoned), true);
        }
        if (b.fx->dk.vorpal) {
            check("vorpal, a damaging blow", blow(b.fx->dk.vorpal, 0, 1, false, 20, combat::Ev::Beheaded), true);
            check("vorpal, a blow of no damage", blow(b.fx->dk.vorpal, 0, 0, false, 20, combat::Ev::Beheaded), need == 0);
        }
    }
    rules::set_fixes(true);
    for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
    combat::occupancy(b);
    expect(P, "the on-hit specials after a blow of no damage as the original's (oracle_onhit: the blow's damage test, the Dark Queen's 0xB8)",
           bad == 0 && need >= 0 && n > 0, "needs_damage %d free 0x%02X; %d of %d wrong (%s)", need, freefx, bad, n, first);
}

// The stats' recalculation's effects (round 16): oracle_statfx_<game>.txt
// from GAME.OVR (tools/devkit/oracles/statfx.py - Secret 0x374A8, Darkness
// 0x3307E, Treasures 0x3CD87: the Strength effects in order, the levels
// that make an 18/xx, Constitution's effect, Charisma's, Intelligence /
// Wisdom's). Each made on a character with rules::stats: Strength 16 + 4 by
// the first (18/20 for those levels, else 18), giant 21, Enlarge 18/50,
// Charisma + 3, the healing on at 20 (its minutes) and off under it,
// Intelligence / Wisdom set
void stat_effects()
{
    const int g = game();
    if (g != 3 && g != 4 && g != 9) return;
    static const char* kName[] = {"", "poolrad", "curse", "secret", "darkness", "champions", "knights", "queen", "gateway", "treasures"};
    char path[256];
    snprintf(path, sizeof path, "%s/oracle_statfx_%s.txt", getenv("ORACLE_DIR") ? getenv("ORACLE_DIR") : "/tmp/claude-0/bt", kName[g]);
    FILE* fo = fopen(path, "r");
    if (!fo) {
        expect(P, "the stats' effects: oracle_statfx (tools/devkit/oracles/statfx.py)", false, "%s missing", path);
        return;
    }
    unsigned str[3] = {}, con[4] = {}, cha = 0, iw[2] = {};
    std::vector<unsigned> ftr;
    bool has_iw = false;
    char line[200];
    while (fgets(line, sizeof line, fo)) {
        if (sscanf(line, "str %x %x %x", &str[0], &str[1], &str[2]) == 3) continue;
        if (sscanf(line, "con %x %u %u %x", &con[0], &con[1], &con[2], &con[3]) == 4) continue;
        if (sscanf(line, "cha %x", &cha) == 1) continue;
        if (sscanf(line, "intwis %x %u", &iw[0], &iw[1]) == 2) {
            has_iw = true;
            continue;
        }
        if (!strncmp(line, "fighter ", 8)) {
            char* q = line + 8;
            unsigned v;
            int used;
            while (sscanf(q, "%x%n", &v, &used) == 1) {
                ftr.push_back(v);
                q += used;
            }
        }
    }
    fclose(fo);
    const rules::ItemFacts& f = d->facts;
    // the game record's level places: current / former (class k at + k)
    const unsigned cur0 = g == 3 ? 0x111 : 0x151, old0 = g == 3 ? 0x118 : 0x158;
    int n = 0, bad = 0;
    char first[200] = "";
    auto check = [&](const char* what, int got, int want) {
        ++n;
        if (got != want && !bad++) snprintf(first, sizeof first, "%s: %d, want %d", what, got, want);
    };
    party::Character c = pt->m[0];
    auto fresh = [&]() {
        c.n_items = 0;
        c.n_affects = 0;
        for (int k = 0; k < 16; ++k) c.rec[0x109 + k] = 0;
        c.rec[0x10] = 16;
        c.rec[0x1D] = 0;
        c.rec[0x12] = c.rec[0x14] = 15;
        c.rec[0x18] = 15;
        c.rec[0x1A] = 10;
    };
    char what[96];
    for (int old = 0; old < 2; ++old)
        for (int k = 0; k < 8; ++k) {
            fresh();
            c.rec[(old ? 0x111 : 0x109) + k] = 5;
            add_aff(c, str[0], 0, 104);
            rules::stats(c, f);
            bool fighter = false;
            for (unsigned o : ftr) fighter = fighter || o == (old ? old0 : cur0) + static_cast<unsigned>(k);
            snprintf(what, sizeof what, "Strength 0x%02X +4, %s level of class %d", str[0], old ? "a former" : "a", k);
            check(what, c.rec[0x11] * 1000 + c.rec[0x1C], fighter ? 18020 : 18000);
        }
    fresh();
    add_aff(c, str[1], 0, 121);
    rules::stats(c, f);
    snprintf(what, sizeof what, "giant strength 0x%02X (21)", str[1]);
    check(what, c.rec[0x11], 21);
    fresh();
    add_aff(c, str[2], 0, 51);
    rules::stats(c, f);
    snprintf(what, sizeof what, "Enlarge 0x%02X (18/50)", str[2]);
    check(what, c.rec[0x11] * 1000 + c.rec[0x1C], 18050);
    fresh();
    add_aff(c, cha, 0, 3);
    rules::stats(c, f);
    snprintf(what, sizeof what, "Charisma 0x%02X + 3", cha);
    check(what, c.rec[0x1B], 13);
    fresh();
    c.rec[0x18] = static_cast<uint8_t>(con[1]);
    rules::stats(c, f);
    int k = -1;
    for (int j = 0; j < c.n_affects; ++j)
        if (c.affects[j][0] == con[0]) k = j;
    snprintf(what, sizeof what, "Constitution %u: effect 0x%02X for %u minutes", con[1], con[0], con[2]);
    check(what, k >= 0 ? c.affects[k][1] | c.affects[k][2] << 8 : -1, static_cast<int>(con[2]));
    c.rec[0x18] = static_cast<uint8_t>(con[1] - 1);
    rules::stats(c, f);
    k = -1;
    for (int j = 0; j < c.n_affects; ++j)
        if (c.affects[j][0] == con[0]) k = j;
    snprintf(what, sizeof what, "Constitution %u: effect 0x%02X gone", con[1] - 1, con[0]);
    check(what, k, -1);
    if (has_iw) {
        fresh();
        add_aff(c, iw[0], 0, 9);
        rules::stats(c, f);
        snprintf(what, sizeof what, "Intelligence / Wisdom while 0x%02X", iw[0]);
        check(what, c.rec[0x13] * 100 + c.rec[0x15], static_cast<int>(iw[1] * 101));
    }
    expect(P, "the stats' recalculation's effects as the original's (oracle_statfx: Strength's three, the 18/xx levels, Constitution, Charisma, Intelligence / Wisdom)",
           bad == 0 && str[0] && !ftr.empty(), "%d of %d wrong (%s)", bad, n, first);
}

// The Dark Queen's to-hit hooks (round 16): oracle_qhooks.txt from DQK.EXE
// (tools/devkit/oracles/qhooks.py - its hook switch 0x7202's events 0xA, the
// attacker's effects, 0x10, the target's, and 0xB, the target's AC after its
// recalculation; each effect's handler read and told: the roll's change and
// its condition, a miss, the AC's change). Each line made into a blow: the
// smallest d20 that hits (to-hit 40 against stored AC 50: 10 with nothing)
// against the oracle's; Game Fixes off. Also: the hooked roll under 0
// misses; the dragons' fear (0x4F361) without a save
int qh_roll = 10;
int qh_die(int n) { return n == 20 ? qh_roll - 1 : -1; }
void queen_hooks()
{
    if (game() != 7) return;
    char path[256];
    snprintf(path, sizeof path, "%s/oracle_qhooks.txt", getenv("ORACLE_DIR") ? getenv("ORACLE_DIR") : "/tmp/claude-0/bt");
    FILE* fo = fopen(path, "r");
    if (!fo) {
        expect(P, "the Dark Queen's hooks: oracle_qhooks (tools/devkit/oracles/qhooks.py)", false, "%s missing", path);
        return;
    }
    if (!fight()) return;
    combat::Battle& b = fg->b;
    const int A = 0, T = B;
    auto scene = [&]() {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        mx::put(A, 0, 10, 10, 60);
        mx::put(T, 1, 11, 10, 60);
        F(A).target = T;
        F(T).target = A;
        F(T).delay = 5;
        F(A).rec[0x199] = 40;
        F(T).rec[0x19A] = F(T).rec[0x19B] = 50;
        b.to_hit_party = b.to_hit_monsters = 0;
    };
    // the smallest d20 that hits (21: none, not even 20); `rear`: a blow from behind
    auto thresh = [&](bool rear, bool keep = false) {
        create::g_die_hook = qh_die;
        int got = 21;
        uint8_t save[4][party::kAffectSize];
        int n_save = F(T).n_aff ? *F(T).n_aff : 0;
        for (int k = 0; k < n_save && k < 4; ++k) memcpy(save[k], F(T).aff[k], party::kAffectSize);
        for (int r = 2; r <= 20; ++r) {
            qh_roll = r;
            F(A).attacks[0] = 1;
            F(A).attacks[1] = 0;
            F(T).received = 0;
            F(T).rec[0x1A4] = 60;
            F(T).rec[0x195] = party::Okay;
            const combat::Attack at = combat::attack(b, A, T, nullptr, rng, rear);
            const bool hit = at.n > 0 && at.hits[0].hit;
            if (!keep)
                for (int k = 0; k < n_save && k < 4; ++k) memcpy(F(T).aff[k], save[k], party::kAffectSize);
            if (hit) {
                got = r;
                break;
            }
            if (keep) break;
        }
        create::g_die_hook = nullptr;
        return got;
    };
    rules::set_fixes(false);
    scene();
    const int base = thresh(false);
    int n = 0, bad = 0;
    char first[200] = "";
    auto check = [&](const char* what, int got, int want) {
        ++n;
        if (got != want && !bad++) snprintf(first, sizeof first, "%s: smallest d20 %d, want %d", what, got, want);
    };
    check("nothing", base, 10);
    char line[240];
    int lines = 0;
    bool clock_line = false;
    while (fgets(line, sizeof line, fo)) {
        char tag;
        unsigned eff;
        char rest[200] = "";
        if (!strncmp(line, "clock round_start", 17)) {
            clock_line = true;
            continue;
        }
        if (sscanf(line, "%c 0x%x %199[^#]", &tag, &eff, rest) < 3 || line[0] == '#') continue;
        ++lines;
        int roll = 0, front = 0, rear = 0;
        const char* r = strstr(rest, "roll ");
        if (r) roll = atoi(r + 5);
        if ((r = strstr(rest, "acfront "))) front = atoi(r + 8);
        if ((r = strstr(rest, "acrear "))) rear = atoi(r + 7);
        const bool miss = strstr(rest, "miss") != nullptr, cloak = strstr(rest, "cloak") != nullptr;
        char what[64];
        snprintf(what, sizeof what, "%c 0x%02X", tag, eff);
        scene();
        if (strstr(rest, "nohandler")) continue;     // (none set: never on anyone)
        if (strstr(rest, "tohit base")) {
            // (round 17; 0x83 - 0x543D8): the to-hit in use made the record's
            // THAC0 base + Strength's to-hit next to the target, + Dexterity's
            // missile bonus further off (oracle: the bonus routines run at
            // Strength 18/00 and Dexterity 21); kept in the record
            const int near = atoi(strstr(rest, "near_str23 ") + 11), far = atoi(strstr(rest, "far_dex21 ") + 10);
            const int cut = atoi(strstr(rest, "cut ") + 4);
            q_add(F(A), static_cast<uint8_t>(eff), 0);
            F(A).rec[0x73] = 30;
            F(A).rec[0x11] = 18;
            F(A).rec[0x1C] = 100;
            F(A).rec[0x17] = 21;
            (void)thresh(false, true);
            check("0x83 next to it: the to-hit in use", F(A).rec[0x199], 30 + near);
            F(T).x = static_cast<int16_t>(10 + cut + 2);
            combat::occupancy(b);
            F(A).rec[0x199] = 40;
            (void)thresh(false, true);
            check("0x83 further off: the to-hit in use", F(A).rec[0x199], 30 + far);
            // (round 18) its damage words: lost (the blow's own damage) -
            // with the Game Fixes the handler's (oracle: 1dS + Strength's
            // damage at 18/00 + the weapon's byte + N next to it, 1dS + the
            // byte + N further off); the dice 20 / 5 on a d6 / 3 on a d4
            if (const char* dn = strstr(rest, "dmg_near 1d")) {
                const int ns = atoi(dn + 11), np = atoi(strchr(dn + 11, '+') + 1);
                const char* dfp = strstr(rest, "dmg_far 1d");
                const int fs = dfp ? atoi(dfp + 10) : 0, fp = dfp ? atoi(strchr(dfp + 10, '+') + 1) : 0;
                const char* sdp = strstr(rest, "near_strdmg23 ");
                const int sdm = sdp ? atoi(sdp + 14) : 0;
                auto die = [](int k) { return k == 6 ? 5 : k == 4 ? 3 : 1; };
                auto blow = [&](bool hoop, bool fixes, bool near) {
                    scene();
                    if (hoop) q_add(F(A), static_cast<uint8_t>(eff), 0);
                    F(A).rec[0x73] = 30;
                    F(A).rec[0x11] = 18;
                    F(A).rec[0x1C] = 100;
                    F(A).rec[0x17] = 21;
                    if (!near) {
                        F(T).x = static_cast<int16_t>(10 + cut + 2);
                        combat::occupancy(b);
                    }
                    rules::set_fixes(fixes);
                    create::g_die_hook = [](int k) { return k == 20 ? 19 : k == 6 ? 4 : k == 4 ? 2 : 0; };
                    F(A).attacks[0] = 1;
                    F(A).attacks[1] = 0;
                    F(T).rec[0x1A4] = 60;
                    const combat::Attack at = combat::attack(b, A, T, nullptr, rng, false);
                    create::g_die_hook = nullptr;
                    rules::set_fixes(false);
                    return at.n > 0 && at.hits[0].hit ? at.hits[0].damage : -1;
                };
                const int plain = blow(false, false, true), plain_far = blow(false, false, false);
                check("0x83's damage lost, next to it (the blow's own)", blow(true, false, true), plain);
                check("0x83's damage lost, further off (the blow's own)", blow(true, false, false), plain_far);
                check("0x83's damage with the Game Fixes, next to it", blow(true, true, true), die(ns) + sdm + np);
                check("0x83's damage with the Game Fixes, further off", blow(true, true, false), die(fs) + fp);
            }
            continue;
        }
        if (tag == 'A') {
            if (strstr(rest, "prayer")) {
                const int same = atoi(strstr(rest, "same ") + 5);
                q_add(F(A), static_cast<uint8_t>(eff), 0);
                check(what, thresh(false), 10 - same);
                scene();
                q_add(F(T), static_cast<uint8_t>(eff), 0);
                F(T).aff[q_find(F(T), static_cast<uint8_t>(eff))][3] = 0x10;    // (the engine's: the caster's side, 1)
                check(what, thresh(false), 10 - atoi(strstr(rest, "other ") + 6));
                continue;
            }
            q_add(F(A), static_cast<uint8_t>(eff), 0);
            if (strstr(rest, "holder.target_flags&0x80")) {
                check(what, thresh(false), 10);
                F(T).rec[0x14B] = 0x80;
            }
            // (its own AC's change is undone: the target's AC is recalculated before each blow)
            check(what, thresh(false), 10 - roll);
            if (strstr(rest, "acfront ")) {
                // (round 17) the attacker's own stored AC bytes changed by its
                // blow, back when a blow on it recalculates them
                const int af = atoi(strstr(rest, "acfront ") + 8), ar = atoi(strstr(rest, "acrear ") + 7);
                scene();
                q_add(F(A), static_cast<uint8_t>(eff), 0);
                (void)thresh(false, true);
                char w2[80];
                snprintf(w2, sizeof w2, "%s: its own stored AC after its blow", what);
                check(w2, F(A).rec[0x19A] * 1000 + F(A).rec[0x19B], (50 + af) * 1000 + 50 + ar);
                F(T).attacks[0] = 1;
                F(T).attacks[1] = 0;
                F(A).received = 0;
                create::g_die_hook = qh_die;
                qh_roll = 2;
                (void)combat::attack(b, T, A, nullptr, rng, false);
                create::g_die_hook = nullptr;
                snprintf(w2, sizeof w2, "%s: its stored AC once hit at", what);
                check(w2, F(A).rec[0x19A] * 1000 + F(A).rec[0x19B], 50 * 1000 + 50);
            }
        } else if (tag == 'T') {
            q_add(F(T), static_cast<uint8_t>(eff), 0);
            if (cloak) {
                check(what, thresh(false, true), 21);    // (the first blow: a miss, even a 20)
                check(what, thresh(false), 10);          // (then marked)
                continue;
            }
            if (strstr(rest, "holder_acted")) {
                check(what, thresh(false), 10);           // (not acted yet: there)
                F(T).delay = 0;
                check(what, thresh(false), 21);
                continue;
            }
            if (strstr(rest, "attacker_lacks 0x18")) {
                q_add(F(A), 0x18, 0);
                check(what, thresh(false), 10);
                combat::lose_aff(F(A), 0x18);
            }
            if (strstr(rest, "holder_flags&0x45")) {
                check(what, thresh(false), 10);
                F(T).rec[0x14B] = 0x45;
                if (strstr(rest, "holder_lacks 0x18")) {
                    q_add(F(T), 0x18, 0);
                    check(what, thresh(false), 10);
                    combat::lose_aff(F(T), 0x18);
                }
            }
            if (strstr(rest, "attacker_flags&0x04")) {
                check(what, thresh(false), 10);
                F(A).rec[0x14B] = 4;
            }
            check(what, thresh(false), miss ? 21 : 10 - roll);
        } else if (tag == 'B') {
            q_add(F(T), static_cast<uint8_t>(eff), 0);
            if (strstr(rest, "attacker_flags&0x01")) F(A).rec[0x14B] = 1;
            // (the roll's changes are lost: the d20 is rolled after; the AC's count)
            const char* fl = strstr(rest, "floorfront ");
            if (fl) {
                const int floor = static_cast<int>(strtol(fl + 11, nullptr, 16));
                check(what, thresh(false), 10 + (floor - 50));
                check(what, thresh(true), 10);
                continue;
            }
            check(what, thresh(false), 10 + front);
            check(what, thresh(true), 10 + rear);
        }
    }
    fclose(fo);
    // the roll under 0 misses: an invisible target (-4) and a big to-hit - a 2 misses, a 4 hits
    scene();
    q_add(F(T), 0x19, 0);
    F(A).rec[0x199] = 80;
    check("invisible, to-hit 80: roll - 4 under 0 misses", thresh(false), 4);
    // the Shield lost to coughing's recalculation; kept with the Game Fixes
    scene();
    q_add(F(T), 0x11, 0);
    q_add(F(T), 0x1E, 0);
    check("Shield + coughing (fixes off): the Shield lost, 2 worse", thresh(false), 8);
    rules::set_fixes(true);
    check("Shield + coughing (Game Fixes): AC 3 at worst, 2 worse", thresh(false), 15);
    // Invisible to Animals with the Game Fixes: the attacker's flags / sight
    scene();
    q_add(F(T), 0x45, 0);
    F(T).rec[0x14B] = 0x45;
    check("0x45 (Game Fixes): the target's own flags no longer count", thresh(false), 10);
    F(A).rec[0x14B] = 0x45;
    check("0x45 (Game Fixes): an attacker of flags & 0x45 not seeing invisible -4", thresh(false), 14);
    rules::set_fixes(false);
    expect(P, "the Dark Queen's to-hit hooks as DQK.EXE's handlers (oracle_qhooks: events 0xA / 0x10 / 0xB), the hooked roll under 0 a miss",
           lines > 20 && bad == 0, "%d lines, %d of %d wrong (%s)", lines, bad, n, first);
    // The dragons' fear (effect 0x64, its handler 0x4F361): no save - the
    // other side's okay ones under 7 Hit Dice without 0x78 / 0x5C get 0x78
    scene();
    mx::put(1, 0, 10, 12, 60);
    mx::put(2, 0, 10, 13, 60);
    F(A).rec[0xE5] = 3;
    F(1).rec[0xE5] = 7;
    F(2).rec[0xE5] = 1;
    q_add(F(2), 0x5C, 0);
    q_add(F(T), 0x64, 0);
    for (int k = 0; k < 5; ++k) F(A).rec[0xDF + k] = 1;     // (a save made always - none asked)
    combat::Event ev[16];
    create::g_die_hook = [](int nn) { return nn == 20 ? 19 : -1; };
    combat::start_round(b, rng, ev, 16);
    create::g_die_hook = nullptr;
    expect(P, "the Dark Queen's dragon fear (0x4F361): no save - under 7 Hit Dice terrified (0x78), 7 and over, immune (0x5C) not",
           q_find(F(A), 0x78) >= 0 && q_find(F(1), 0x78) < 0 && q_find(F(2), 0x78) < 0, "HD 3 %d, HD 7 %d, immune %d",
           q_find(F(A), 0x78) >= 0, q_find(F(1), 0x78) >= 0, q_find(F(2), 0x78) >= 0);
    // Its clock (oracle: the round's start calls the time step with 1 minute):
    // the effects a minute on as a round begins - one of 1 minute gone before
    // anyone acts, one of 3 left with 2; nothing at the round's end (the
    // next start's step only)
    scene();
    q_add(F(A), 0x01, 1);
    q_add(F(A), 0x02, 3);
    begin_round(C);
    const bool one_gone = q_find(F(A), 0x01) < 0, two_left = q_find(F(A), 0x02) >= 0 && F(A).aff[q_find(F(A), 0x02)][1] == 2;
    round_end(C);                           // (on into the next round's start: one step more, not two)
    const bool end_kept = q_find(F(A), 0x02) >= 0;
    expect(P, "the Dark Queen's clock (0x5EFF via 0x61AD from the round's start 0x2336C): effects a minute on as each round begins, not at its end",
           clock_line && one_gone && two_left && end_kept, "oracle %d, 1-minute gone %d, 3-minute at 2 %d, kept past the end %d", clock_line,
           one_gone, two_left, end_kept);
    rules::set_fixes(true);
}

// The sides' to-hit bonuses (round 11): oracle_tohit_<game>.txt from the
// original's attacker-against-target roll (tools/devkit/oracles/tohit.py -
// the bonus bytes it loads and its difficulty switch walked for each value
// 0-6). The script places set (party 3, the others -2) and the difficulty
// to each value; play::side_bonuses' sums against the original's, the Game
// Fixes off - and on: Secret's / Death Knights' switch on their difficulty
// (fixadj: the switch walked as if it tested it), the rest unchanged
void to_hit_sides()
{
    static const char* kName[] = {"", "poolrad", "curse", "secret", "darkness", "champions", "knights", "queen", "gateway", "treasures"};
    const int g = game();
    char path[256];
    snprintf(path, sizeof path, "%s/oracle_tohit_%s.txt", getenv("ORACLE_DIR") ? getenv("ORACLE_DIR") : "/tmp/claude-0/bt",
             g >= 1 && g <= 9 ? kName[g] : "?");
    FILE* fo = fopen(path, "r");
    if (!fo) {
        expect(P, "to-hit sides: oracle_tohit (tools/devkit/oracles/tohit.py)", false, "%s missing", path);
        return;
    }
    unsigned party_at = 0, mons_at = 0, diff_at = 0;
    int adj[7] = {}, fixadj[7] = {};
    bool has_fix = false;
    char line[160];
    while (fgets(line, sizeof line, fo)) {
        int v, dv;
        unsigned x;
        if (sscanf(line, "party %x", &x) == 1) party_at = x;
        else if (sscanf(line, "monsters %x", &x) == 1) mons_at = x;
        else if (sscanf(line, "difficulty const %x (area word %x)", &v, &x) == 2) diff_at = x;
        else if (sscanf(line, "difficulty %x", &x) == 1) diff_at = x;
        else if (sscanf(line, "adj %d %d", &v, &dv) == 2 && v >= 0 && v < 7) adj[v] = dv;
        else if (sscanf(line, "fixadj %d %d", &v, &dv) == 2 && v >= 0 && v < 7) {
            fixadj[v] = dv;
            has_fix = true;
        }
    }
    fclose(fo);
    if (!fight()) return;
    combat::Battle& b = fg->b;
    const uint16_t keep_p = party_at ? vm->get(static_cast<uint16_t>(party_at)) : 0;
    const uint16_t keep_m = mons_at ? vm->get(static_cast<uint16_t>(mons_at)) : 0;
    const uint16_t keep_d = diff_at ? vm->get(static_cast<uint16_t>(diff_at)) : 0;
    // (Secret and Death Knights have a difficulty to give: their Level menu)
    const bool fix_game = g == 3 || g == 6;
    int bad = 0, n = 0;
    char first[160] = "";
    for (int fixes = 0; fixes <= 1; ++fixes) {
        rules::set_fixes(fixes != 0);
        for (int v = 0; v <= 6; ++v) {
            if (!diff_at && v) break;
            if (party_at) vm->set(static_cast<uint16_t>(party_at), 3);
            if (mons_at) vm->set(static_cast<uint16_t>(mons_at), 0xFE);
            if (diff_at) vm->set(static_cast<uint16_t>(diff_at), static_cast<uint16_t>(v));
            b.to_hit_party = b.to_hit_monsters = 99;
            play::side_bonuses(b);
            const int want_p = 3 + (fixes && fix_game && has_fix ? fixadj[v] : adj[v]);
            ++n;
            if ((b.to_hit_party != want_p || b.to_hit_monsters != -2) && !bad++)
                snprintf(first, sizeof first, "fixes %d difficulty %d: party %d others %d, want %d / -2", fixes, v, b.to_hit_party,
                         b.to_hit_monsters, want_p);
        }
    }
    rules::set_fixes(true);
    if (party_at) vm->set(static_cast<uint16_t>(party_at), keep_p);
    if (mons_at) vm->set(static_cast<uint16_t>(mons_at), keep_m);
    if (diff_at) vm->set(static_cast<uint16_t>(diff_at), keep_d);
    expect(P, "to-hit: each side's bonus as the original's roll takes it (the script bytes, the party's difficulty switch)",
           bad == 0, "game %d: %d of %d wrong (%s)", g, bad, n, first);
}

#include "behave_aicast.inc"

void run()
{
    if (!monkey_boot() || !play::load_rules(m) || !m) {
        expect(P, "the game booted, rule tables", false);
        return;
    }
    rules::set_fixes(true);
    const char* only = getenv("ONLY");
    auto want = [&](const char* s) { return !only || strstr(only, s); };
    if (!fight()) {
        expect(P, "a fight", false);
        return;
    }
    if (want("table")) table();
    if (want("turn")) turning();
    if (want("former")) former();
    if (want("haste")) haste();
    if (want("moons")) moons();
    if (want("caster")) caster();
    if (want("learn")) {
        learning();
        if (game() == 1) {                  // (PoolRad's spell 56: the original's rule, without the Game Fixes)
            rules::set_fixes(false);
            learning();
            rules::set_fixes(true);
        }
    }
    if (want("use")) using_items();
    if (want("aiorder")) ai_pending();
    if (want("bandorder")) ai_bandage_order();
    if (want("quickacts")) quick_actions();
    if (want("wrate")) {
        using G = games::Game;
        static const struct { G g; const char* s; } shorts[] = {
            {G::PoolOfRadiance, "poolrad"}, {G::CurseOfTheAzureBonds, "curse"}, {G::SecretOfTheSilverBlades, "secret"},
            {G::PoolsOfDarkness, "darkness"}, {G::GatewayToTheSavageFrontier, "gateway"},
            {G::TreasuresOfTheSavageFrontier, "treasures"}, {G::ChampionsOfKrynn, "champions"},
            {G::DeathKnightsOfKrynn, "knights"}, {G::DarkQueenOfKrynn, "queen"}};
        for (const auto& e : shorts)
            if (e.g == d->prof->game) wrate_check(e.s);
    }
    if (want("quickpage")) quick_page_check();
    if (want("aiturn")) ai_turn_types();
    if (want("trainspell")) train_spell_offer();
    if (want("tohit")) to_hit_sides();
    if (want("aimood")) ai_mood_steps();
    if (want("feeble")) feeblemind();
    if (want("canuse")) {
        krynn_can();
        armour_can();
    }
    if (want("wrap")) queen_wrap();
    if (want("qpages")) queen_pages();
    if (want("qlost")) queen_lost();
    if (want("qwater")) queen_water();
    if (want("gwater")) gateway_water();
    if (want("barkskin")) barkskin_saves();
    if (want("sfeeble")) secret_feeble();
    if (want("qacid")) queen_acid();
    if (want("qbozak")) queen_bozak();
    if (want("qaurak")) queen_aurak();
    if (want("qmummy")) queen_mummy();
    if (want("qpudding")) queen_pudding();
    if (want("qspore")) queen_spore();
    if (want("qmound")) queen_mound();
    if (want("halves")) pool_halves();
    if (want("stench")) stench();
    if (want("yeti")) yeti();
    if (want("dmg22")) dmg22();
    if (want("cbreath")) champ_breath();
    if (want("qbreath")) queen_breath();
    if (want("qsweep22")) queen_sweep22();
    if (want("qhooks")) queen_hooks();
    if (want("statfx")) stat_effects();
    if (want("finder")) finder_check();
    if (want("turnfind")) turnfind_check();
    if (want("khit")) knights_hit();
    if (want("onhit")) onhit_check();
    if (want("guard0")) guard0_check();
    if (want("yell")) yell_check();
    if (want("aicast")) aicast::run();
    if (want("restore")) restore();
    if (want("rest")) rest();
    rules::set_fixes(true);
}

} // namespace r4

void behave_round4() { r4::run(); }
