// Behaviour checks, round 4 (sweep's `behave rules4`, -DBEHAVE_ROUND4 with
// -DBEHAVE_MATRIX - its cleared field and plain fighters; any game: GAME,
// BOOTSLOT, MAREA / MBLOCK, FIGHTAT as the fuzzer's; ONLY= table, turn,
// former, restore, rest, haste, moons, caster, learn, use, canuse). Expectations from the originals'
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
};
// The reference: Curse / Pool (rule 0), Gateway (1), Champions (2),
// Darkness's (11 columns - its facts R13; Death Knights' the same with
// Majere's + 2, read 2026-11-07; the Dark Queen's assumed Darkness's)
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
        const int L = std::max(lvl[0], lvl[3] - 2) + (rule == 3 && c.god == 2 ? 2 : 0);
        const int col = L <= 8 ? L : L < 14 ? 9 : 10;
        int limit = 13;
        count = -1;
        for (;;) {
            int best = -1, type = 0;
            for (size_t k = 0; k < c.types.size(); ++k)
                if (!gone[k] && c.types[k] > type && c.types[k] <= limit) best = static_cast<int>(k), type = c.types[k];
            if (best < 0 || count == 0) break;
            const int v = tv(type, col);
            if (count < 0) count = v < 0 ? (c.c6 ? c.c6 : 1) + 6 : (c.c12 ? c.c12 : 1);
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
    play::fg->b.turn_minus = 0;
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
    for (int c12 : {1, 3}) cases.push_back({{{0, 12}}, {}, 0, 0, c12, 20, 6, {1, 1, 1, 1, 1, 1, 1, 1}});
    cases.push_back({{{0, 4}}, {}, 0, 0, 12, 9, 1, {8, 2, 2, 11}});
    // paladins (Gateway: as a cleric 2 lower; Darkness's the same), Majere (Champions + 2)
    for (int P2 : {2, 3, 6, 11}) cases.push_back({{{3, P2}}, {}, 0, 0, 4, 12, 1, {1, 2, 3}});
    cases.push_back({{{0, 3}}, {}, 0, 2, 4, 12, 1, {1, 2, 3, 4}});
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
        // Death Knights (0x94CC) by levels
        if (game() == 4 || game() == 9 || game() == 6) {
            const bool sk = game() != 6;
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

void moons()
{
    if (!m->tables.lay.krynn) return;
    if (m->tables.lay.queen) queen_moons();
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
    if (g == 7) return;                     // (the Dark Queen: not read)
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
}

// ---- Who may learn / memorize a spell in the Krynn games (their
// can-learn routine: Champions GAME.OVR 0x2DC8E, Death Knights 0x29C84,
// by the game's own class codes 0 / 2 cleric, 1 druid, 3 magic-user):
// cleric spells - Wisdom over 8 and a cleric, a knight past 5th (Death
// Knights also a paladin past 8th; Champions the levels before a human's
// dual class too); druid spells - a ranger past 6th (Death Knights 7th);
// magic-user spells - Intelligence over 8 (outside fights)
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
    int bad = 0, n = 0, edge = 0;
    char first[160] = "";
    for (int k = 0; k < 3000; ++k) {
        char s[96] = "";
        const int words = 1 + rnd(7);
        for (int w2 = 0; w2 < words; ++w2) {
            if (w2) strcat(s, rnd(8) ? " " : "  ");
            strcat(s, kWords[rnd(sizeof kWords / sizeof kWords[0])]);
        }
        if (rnd(4) == 0) strcat(s, " ");
        const int row0 = 11 + rnd(4);
        // the original's loop (0-based; L = the last index)
        C2.clear(0);
        const int L = static_cast<int>(strlen(s)) - 1, x0 = 23, x1 = 38, y1 = 21;
        int col = x0, row = row0, di = 0;
        bool full = false;
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
                    full = true;
                    break;
                }
                continue;
            }
            for (; di <= si; ++di, ++col) {
                const char one[2] = {s[di], 0};
                play::put(C2, one, col, row, 10);
            }
        }
        px2 = px;
        if (full) continue;
        C.clear(0);
        play::panel_box_text(C, s, row0, 10, 21, false, true);
        ++n;
        if (memcmp(px.data(), px2.data(), px2.size()) != 0) {
            ++bad;
            if (!*first) snprintf(first, sizeof first, "\"%s\" from row %d", s, row0);
        }
    }
    snprintf(w, sizeof w, "the Dark Queen's fight words wrap as its res 0xCD31 (letter by letter; %d words' space just past the box)", edge);
    expect(P, w, bad == 0 && n > 0 && edge > 0, "%d of %d differ (%s)", bad, n, first);
}

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
    if (want("canuse")) krynn_can();
    if (want("wrap")) queen_wrap();
    if (want("restore")) restore();
    if (want("rest")) rest();
    rules::set_fixes(true);
}

} // namespace r4

void behave_round4() { r4::run(); }
