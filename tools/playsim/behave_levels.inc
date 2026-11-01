// Behaviour checks: levels and spell slots in every game (sweep's `behave
// levels`, -DBEHAVE_LEVELS, any game - GAME, BOOTSLOT and the rest as the
// fuzzer's). Each single class the game's tables have (a human, every
// ability 18 unless the check sets one) is trained from 1st level to its
// last by the training routine, and each level is checked against rules
// that hold whatever a game's tables say:
//   L1 the class trained goes up one level, the others stay
//   L2 the to-hit figure (record 0x73, 60 - THAC0) never gets worse
//   L3 no saving throw gets worse
//   L4 hit points: the maximum and the hit points now both rise, by the
//      same amount, at least 1
//   L5 no spell slot is lost
// (parts "values" and "multi", further down: every value at every level
// against the original program's own tables - oracle_lv_<game>.txt)
// and the cleric's Wisdom bonus (AD&D's Players Handbook, the table every
// game's training follows - coab's / the listings' cleric slot routine):
// Wisdom 13 +1 1st, 14 +1 1st, 15 +1 2nd, 16 +1 2nd, 17 +1 3rd, 18 +1 4th,
// each only at a spell level the cleric already has slots for; none for
// paladins, rangers or druids (their slots never follow Wisdom). The
// expectations are the rules', never the engine's tables.

#include <fstream>
#include <map>
#include <sstream>

namespace blv {

using namespace play;
const char* P = "levels";
Making* m = nullptr;

void set_stat(party::Character& c, int i, int v)
{
    c.rec[0x10 + i * 2] = static_cast<uint8_t>(v);
    c.rec[0x11 + i * 2] = static_cast<uint8_t>(v);
}
void put32(uint8_t* r, int o, uint32_t v)
{
    for (int i = 0; i < 4; ++i) r[o + i] = static_cast<uint8_t>(v >> (8 * i));
}
// A human of class `cls` at 1st level, every ability `stat`
void fresh(party::Character& c, int cls, int stat = 18)
{
    c = party::Character{};
    c.rec[0x74] = 7;                    // (a human: the record's race numbers)
    c.rec[0x75] = static_cast<uint8_t>(cls);
    for (int i = 0; i < 6; ++i) set_stat(c, i, stat);
    c.rec[0x11C] = 2;
    c.rec[0x124] = 50;
    c.rec[0x125] = 1;
    c.rec[0xE4] = 12;
    c.rec[0x196] = 1;
    c.rec[0x109 + cls] = 1;
    c.rec[0xE5] = 1;
    c.rec[0x78] = c.rec[0x1A4] = c.rec[0x12C] = 8;
    c.ext[party::kExtGod] = 1;          // (Krynn: a god, a moon, an order)
    c.ext[party::kExtRobe] = 1;
    c.ext[party::kExtKnight] = 1;
    classes::class_bonuses(c, m->tables);
}

// Every spell slot place: cleric, druid, magic-user x spell levels 1-9
int slot(const party::Character& c, int kind, int lvl)
{
    if (lvl < 5) return c.rec[0x12D + kind * 5 + lvl];
    static const uint8_t kAt[3] = {party::kExtCleric67, party::kExtDruidMore, party::kExtMage67};
    static const uint8_t kAt89[3] = {party::kExtCleric89, party::kExtDruid89, party::kExtMage89};
    return lvl < 7 ? c.ext[kAt[kind] + (lvl - 5)] : c.ext[kAt89[kind] + (lvl - 7)];
}

int fails = 0;
char w[200];
void bad(const char* rule, int cls, int L, const char* fmt, ...) __attribute__((format(printf, 4, 5)));
void bad(const char* rule, int cls, int L, const char* fmt, ...)
{
    ++fails;
    if (fails > 40) return;
    char t[160];
    va_list a;
    va_start(a, fmt);
    vsnprintf(t, sizeof t, fmt, a);
    va_end(a);
    snprintf(w, sizeof w, "%s class %d -> level %d", rule, cls, L);
    expect(P, w, false, "%s", t);
}

// Trains class `cls` one level; false: it can't (its last level, or none)
bool train(party::Character& c, int cls)
{
    const int bit = m->tables.u8(static_cast<uint16_t>(m->tables.lay.class_masks + cls));
    put32(c.rec, 0x127, 0x3FFFFFFF);
    if (!bit || !(create::trainable(c, m->tables) & bit)) return false;
    create::train_classes(c, m->tables, m->facts, m->dice, bit, false);
    return true;
}

void classes_up()
{
    static party::Character c, was;
    int classes_done = 0, levels_done = 0;
    for (int cls = 0; cls < 8; ++cls) {
        if (!m->tables.u8(static_cast<uint16_t>(m->tables.lay.class_masks + cls))) continue;
        fresh(c, cls);
        ++classes_done;
        for (int step = 0; step < 45; ++step) {
            was = c;
            if (!train(c, cls)) break;
            ++levels_done;
            const int L = c.level(cls);
            if (L != was.level(cls) + 1) bad("L1 one level", cls, L, "level %d after %d", L, was.level(cls));
            for (int k = 0; k < 8; ++k)
                if (k != cls && c.level(k) != was.level(k)) bad("L1 others kept", cls, L, "class %d %d -> %d", k, was.level(k), c.level(k));
            if (c.rec[0x73] < was.rec[0x73]) bad("L2 to-hit", cls, L, "%d -> %d", was.rec[0x73], c.rec[0x73]);
            for (int s = 0; s < 5; ++s)
                if (c.rec[0xDF + s] > was.rec[0xDF + s]) bad("L3 saves", cls, L, "save %d: %d -> %d", s, was.rec[0xDF + s], c.rec[0xDF + s]);
            const int up = c.hp_max() - was.hp_max(), up_now = c.hp() - was.hp();
            if (up < 1 || up != up_now) bad("L4 hit points", cls, L, "max +%d, now +%d", up, up_now);
            for (int kind = 0; kind < 3; ++kind)
                for (int l = 0; l < 9; ++l)
                    if (slot(c, kind, l) < slot(was, kind, l))
                        bad("L5 slots", cls, L, "kind %d level %d: %d -> %d", kind, l + 1, slot(was, kind, l), slot(c, kind, l));
        }
    }
    snprintf(w, sizeof w, "every class trained to its last level (%d classes, %d levels): L1-L5", classes_done, levels_done);
    expect(P, w, classes_done > 0 && levels_done > classes_done);
}

// The Wisdom bonus: a class trained to level L with Wisdom 12 and with
// Wisdom wis - the slots' difference
void wisdom()
{
    static party::Character a, b;
    static const int kBonus[19][4] = {{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {1, 0, 0, 0}, {2, 0, 0, 0},
                                      {2, 1, 0, 0}, {2, 2, 0, 0}, {2, 2, 1, 0}, {2, 2, 1, 1}};
    int checked = 0, wrong = 0;
    for (int cls : {0, 1, 3, 4}) {
        if (!m->tables.u8(static_cast<uint16_t>(m->tables.lay.class_masks + cls))) continue;
        for (int wis = 13; wis <= 18; ++wis) {
            fresh(a, cls);
            fresh(b, cls);
            set_stat(a, 2, 12);
            set_stat(b, 2, wis);
            classes::class_bonuses(a, m->tables);
            classes::class_bonuses(b, m->tables);
            for (int step = 0; step < 45; ++step) {
                const int L = a.level(cls);
                for (int l = 0; l < 4; ++l) {
                    const int base = slot(a, cls == 1 ? 1 : 0, l), got = slot(b, cls == 1 ? 1 : 0, l);
                    const int want = base + (cls == 0 && base > 0 ? kBonus[wis][l] : 0);
                    ++checked;
                    if (got != want) {
                        ++wrong;
                        if (wrong <= 12) {
                            snprintf(w, sizeof w, "Wisdom %d class %d level %d: spell level %d slots", wis, cls, L, l + 1);
                            expect(P, w, false, "%d, Wisdom 12's %d -> the rules' %d", got, base, want);
                        }
                    }
                }
                if (!train(a, cls) || !train(b, cls)) break;
            }
        }
    }
    snprintf(w, sizeof w, "the Wisdom bonus: clerics only, at the spell levels they have (%d cases)", checked);
    expect(P, w, checked > 0 && wrong == 0, "%d wrong", wrong);
}

// ---- Money with the Game Fixes' coin exchange (SPEC 12 "Money"; AD&D's
// worths, copper 1, silver 10, electrum 100, gold 200, platinum 1000 - the
// games' own rates): random purses, random prices. M1 a price paid takes
// exactly its worth (from the payer, the rest from the counter), nothing
// when the two together haven't enough; M2 no coin below 0 or past 65,535;
// M3 the carried weight follows the coins' count; M4 coins made the highest
// keep their worth and are never more coins; M5 pooling and sharing keep
// the party's and the pool's worth together
void money()
{
    std::mt19937 r(7);
    auto rnd_purse = [&](int* m) {
        for (int k = 0; k < 5; ++k) m[k] = r() % 4 == 0 ? 0 : r() % 8 == 0 ? static_cast<int>(r() % 65536) : static_cast<int>(r() % 2000);
        m[5] = static_cast<int>(r() % 5);
        m[6] = static_cast<int>(r() % 5);
    };
    auto count = [](const int* m) { return m[0] + m[1] + m[2] + m[3] + m[4]; };
    static party::Character c;
    int bad1 = 0, bad2 = 0, bad3 = 0, bad4 = 0, bad5 = 0, cases = 0;
    std::string first;
    auto note = [&](int& n, const char* what) {
        if (!n++ && first.empty()) first = what;
    };
    for (int k = 0; k < 20000; ++k) {
        ++cases;
        int m[7], other[7];
        rnd_purse(m);
        rnd_purse(other);
        c = party::Character{};
        for (int i = 0; i < 7; ++i) {
            c.rec[0xFB + i * 2] = static_cast<uint8_t>(m[i]);
            c.rec[0xFC + i * 2] = static_cast<uint8_t>(m[i] >> 8);
        }
        // (the carried weight: the coins' count and 1,000 more - the u16 a record holds)
        const int weight0 = std::min(count(m) + 1000, 0xFFFF);
        c.rec[0x187] = weight0 & 0xFF;
        c.rec[0x188] = weight0 >> 8;
        const long mine = rules::copper_worth(c), theirs = rules::copper_worth(other);
        const bool with_other = r() % 2;
        const int gold = static_cast<int>(r() % ((mine + (with_other ? theirs : 0)) / 200 + 20 + 1));
        const long cost = gold * 200L;
        int before[7];
        for (int i = 0; i < 7; ++i) before[i] = c.money(i);
        int other_before[7];
        memcpy(other_before, other, sizeof other);
        const bool paid = rules::pay_any(c, with_other ? other : nullptr, gold);
        const long mine2 = rules::copper_worth(c), theirs2 = rules::copper_worth(other);
        const bool can = mine + (with_other ? theirs : 0) >= cost;
        if (paid != can) note(bad1, "paid when it couldn't / refused when it could");
        if (paid && mine + theirs - cost != mine2 + theirs2) note(bad1, "the worth taken is not the price");
        if (!paid && (mine2 != mine || theirs2 != theirs)) note(bad1, "refused, yet coins moved");
        if (paid && with_other && mine >= cost && theirs2 != theirs) note(bad1, "the counter paid though the payer could");
        int now[7];
        for (int i = 0; i < 7; ++i) now[i] = c.money(i);
        for (int i = 0; i < 7; ++i)
            if (now[i] < 0 || now[i] > 0xFFFF || other[i] < 0 || other[i] > 0xFFFF) note(bad2, "a coin out of 0-65535");
        if (now[5] != before[5] || now[6] != before[6]) note(bad2, "gems / jewellery touched by a payment");
        const int w = c.rec[0x187] | c.rec[0x188] << 8;
        if (count(before) + 1000 <= 0xFFFF && w != weight0 - (count(before) - count(now))) note(bad3, "the weight doesn't follow the coins");
        // M4
        int up[7];
        rnd_purse(up);
        int up0[7];
        memcpy(up0, up, sizeof up);
        rules::coins_up(up);
        if (rules::copper_worth(up) != rules::copper_worth(up0)) note(bad4, "coins made higher changed their worth");
        if (count(up) > count(up0)) note(bad4, "coins made higher are more coins");
    }
    // M5: pool and share
    static party::Party p;
    for (int k = 0; k < 2000; ++k) {
        p.clear();
        p.count = 1 + static_cast<int>(r() % 6);
        long total = 0;
        int pool[7] = {};
        for (int i = 0; i < p.count; ++i) {
            party::Character& ch = p.m[i];
            ch = party::Character{};
            int m[7];
            rnd_purse(m);
            for (int j = 0; j < 7; ++j) {
                ch.rec[0xFB + j * 2] = static_cast<uint8_t>(m[j]);
                ch.rec[0xFC + j * 2] = static_cast<uint8_t>(m[j] >> 8);
            }
            total += rules::copper_worth(ch);
        }
        rules::pool(p, pool);
        long after = rules::copper_worth(pool);
        for (int i = 0; i < p.count; ++i) after += rules::copper_worth(p.m[i]);
        if (after != total) note(bad5, "pooling changed the party's worth");
        rules::share(p, pool);
        long shared = rules::copper_worth(pool);
        for (int i = 0; i < p.count; ++i) {
            shared += rules::copper_worth(p.m[i]);
            for (int j = 0; j < 7; ++j)
                if (p.m[i].money(j) < 0 || p.m[i].money(j) > 0xFFFF) note(bad5, "a shared coin out of 0-65535");
        }
        if (shared != total) note(bad5, "sharing changed the party's worth");
    }
    snprintf(w, sizeof w, "money: M1 prices paid exactly (%d purses)", cases);
    expect(P, w, bad1 == 0, "%d wrong; %s", bad1, first.c_str());
    expect(P, "money: M2 coins within 0-65535, gems kept", bad2 == 0, "%d wrong; %s", bad2, first.c_str());
    expect(P, "money: M3 the weight follows the coins", bad3 == 0, "%d wrong; %s", bad3, first.c_str());
    expect(P, "money: M4 coins made highest keep their worth, never more coins", bad4 == 0, "%d wrong; %s", bad4, first.c_str());
    expect(P, "money: M5 pooling and sharing keep the worth", bad5 == 0, "%d wrong; %s", bad5, first.c_str());
}

// ---- The camp's cures (every game's camp spells by what they do; AD&D's
// spells, the dice the game's own): C1 a cure of wounds gives its dice's
// hit points - all lowest, all highest - never past the maximum, nothing
// to the dead (each cast by the one it heals: the potions' are their
// drinker's); C2 Cure Blindness / Cure Disease / Neutralize Poison /
// Remove Curse take their effects away; Slow Poison puts its own on
int die_low(int) { return 0; }
int die_high(int n) { return n - 1; }
void camp()
{
    if (!load_magic() || !mrules || !pt || pt->count < 2) {
        expect(P, "camp: the magic tables and a party of two", false);
        return;
    }
    const auto& mg = d->prof->magic;
    const rules::CureFacts& cu = d->prof->cures;
    static party::Party p;
    int checked = 0, wrong = 0;
    auto fresh_party = [&]() {
        p = *pt;
        for (int i = 0; i < 2; ++i) {
            party::Character& c = p.m[i];
            c.rec[0x195] = party::Okay;
            c.rec[0x78] = 60;
            c.rec[0x1A4] = 60;
            c.n_affects = 0;
        }
    };
    auto say_bad = [&](const char* what, int sp, const char* fmt, int a, int b) {
        ++wrong;
        if (wrong > 16) return;
        snprintf(w, sizeof w, "camp spell %02X: %s", sp, what);
        expect(P, w, false, fmt, a, b);
    };
    for (int k = 0; k < mg.n_camp; ++k) {
        const spells::CampSpell& cs = mg.camp[k];
        spells::Line lines[16];
        party::Character& t = p.m[1];
        switch (cs.does) {
        case spells::Does::Heal: {
            if (!cs.n && !cs.plus) break;
            for (int hi = 0; hi < 2; ++hi) {
                fresh_party();
                t.rec[0x1A4] = 1;
                create::g_die_hook = hi ? die_high : die_low;
                spells::cast(p, 1, 1, cs, mrules->tables, cu, mg.facts, rng, lines, 16);
                create::g_die_hook = nullptr;
                const int want = std::min(60, 1 + cs.n * (hi ? cs.sides : 1) + cs.plus);
                ++checked;
                if (t.hp() != want) say_bad(hi ? "C1 heals its dice, all highest" : "C1 heals its dice, all lowest", cs.spell, "HP %d, the dice's %d", t.hp(), want);
            }
            fresh_party();
            t.rec[0x1A4] = 58;
            create::g_die_hook = die_high;
            spells::cast(p, 1, 1, cs, mrules->tables, cu, mg.facts, rng, lines, 16);
            create::g_die_hook = nullptr;
            ++checked;
            if (t.hp() != 60) say_bad("C1 never past the maximum", cs.spell, "HP %d of %d", t.hp(), 60);
            fresh_party();
            t.rec[0x1A4] = 0;
            t.rec[0x195] = party::Dead;
            spells::cast(p, 1, 1, cs, mrules->tables, cu, mg.facts, rng, lines, 16);
            ++checked;
            if (t.hp() != 0 || t.rec[0x195] != party::Dead) say_bad("C1 nothing to the dead", cs.spell, "HP %d status %d", t.hp(), t.rec[0x195]);
            break;
        }
        case spells::Does::CureBlind:
        case spells::Does::CureDisease:
        case spells::Does::Neutralize:
        case spells::Does::RemoveCurse: {
            uint8_t types[8];
            int nt = 0;
            if (cs.does == spells::Does::CureBlind && cu.blinded) types[nt++] = cu.blinded;
            if (cs.does == spells::Does::CureDisease)
                for (int i = 0; i < 3; ++i)
                    if (mg.facts.disease[i]) types[nt++] = mg.facts.disease[i];
            if (cs.does == spells::Does::Neutralize && cu.poisoned) types[nt++] = cu.poisoned;
            if (cs.does == spells::Does::RemoveCurse && cu.curse) types[nt++] = cu.curse;
            for (int i = 0; i < nt; ++i) {
                fresh_party();
                spells::add_affect(t, types[i], 60, 0, false);
                spells::cast(p, 1, 1, cs, mrules->tables, cu, mg.facts, rng, lines, 16);
                ++checked;
                if (spells::find_affect(t, types[i]) >= 0) say_bad("C2 takes its effect away", cs.spell, "effect %02X still there (%d)", types[i], 0);
            }
            break;
        }
        case spells::Does::SlowPoison:
            if (cu.poisoned && cu.slow_poison) {
                fresh_party();
                spells::add_affect(t, cu.poisoned, 60, 0, false);
                spells::cast(p, 1, 1, cs, mrules->tables, cu, mg.facts, rng, lines, 16);
                ++checked;
                if (spells::find_affect(t, cu.slow_poison) < 0) say_bad("C2 Slow Poison's effect on", cs.spell, "none (%d %d)", 0, 0);
            }
            break;
        default: break;
        }
    }
    snprintf(w, sizeof w, "camp: the cures (%d cases, %d camp spells)", checked, mg.n_camp);
    expect(P, w, checked > 0 && wrong == 0, "%d wrong", wrong);
}

// ---- The values at every level (part "values"; Tom, 2026-11-01: "did you
// check if they were correct?"): each single class the game has - and the
// Krynn games' cleric by god, magic-user by robe, knight by order - trained
// from 1st level to its last by the training routine, dice all highest and
// all lowest, Constitution 18 / 16 / 12 / 5, Game Fixes off (the original)
// and on; at every level against the oracle oracle_lv_<game>.txt (scratch
// lv_oracle.py: the original program's own tables read from the player's
// program, composed by the facts' rules - never the engine's):
//   V1 the level reached, and the last one (no training past it)
//   V2 the experience training wants: one short is refused, the figure trains
//   V3 THAC0 (record 0x73)            V4 the five saves (Game Fixes: each the
//   best of the levels up to it)      V5 half attacks a round
//   V6 hit points gained (the dice's, the Constitution's, past the hit dice
//      the fixed gain; Game Fixes: at least 1)
//   V7 spell slots of every kind and level (Wisdom and Intelligence 18, 16,
//      13, 9: the cleric's Wisdom bonus, the 6th / 7th levels' Wisdom, the
//      magic-user's high levels' Intelligence)
//   V8 thief skills (Dexterity 18, 14, 9)
// Multi-classes (part "multi"): every multi-class of the game's creation
// lists, a class trained at a time: THAC0 the best of the classes', each save
// the best (V3 / V4), attacks the best (V5), slots the sum of the classes'
// (V7), hit points (V6m) the rolled classes' dice and Constitution shared by
// the classes held - by each game's rule (oracle `hpmulti`)
std::map<std::string, std::vector<std::vector<int>>> lvo;
bool lvo_load(const char* game)
{
    lvo.clear();
    const char* dir = getenv("ORACLE_DIR");
    std::string path = std::string(dir ? dir : "/tmp/claude-0/bt") + "/oracle_lv_" + game + ".txt";
    std::ifstream in(path);
    if (!in) return false;
    std::string line;
    while (std::getline(in, line)) {
        const size_t h = line.find('#');
        if (h != std::string::npos) line.erase(h);
        std::istringstream s(line);
        std::string key;
        if (!(s >> key)) continue;
        std::vector<int> row;
        int v;
        while (s >> v) row.push_back(v);
        lvo[key].push_back(row);
    }
    return !lvo.empty();
}
// The row of `key` whose first values are `pre`
const std::vector<int>* lvr(const char* key, std::initializer_list<int> pre)
{
    auto it = lvo.find(key);
    if (it == lvo.end()) return nullptr;
    for (const auto& r : it->second) {
        if (r.size() < pre.size()) continue;
        size_t i = 0;
        bool ok = true;
        for (int p : pre) ok = ok && r[i++] == p;
        if (ok) return &r;
    }
    return nullptr;
}
const char* game_name()
{
    static const char* kNames[10] = {"", "pool", "curse", "secret", "darkness", "champions", "deathknights", "queen",
                                     "gateway", "treasures"};
    const int g = getenv("GAME") ? atoi(getenv("GAME")) : 2;
    return g >= 1 && g <= 9 ? kNames[g] : "";
}
bool krynn() { return m->tables.lay.krynn; }
// The Krynn variants: tag 0 god 1 / robe 1 / knight 1, tag 1 god 5 / robe 2 /
// knight 2 (the Sword), tag 2 knight 3 (the Rose)
void variant(party::Character& c, int tag)
{
    c.ext[party::kExtGod] = tag == 1 ? 5 : 1;
    c.ext[party::kExtRobe] = tag == 1 ? 2 : 1;
    c.ext[party::kExtKnight] = tag == 1 ? 2 : tag == 2 ? 3 : 1;
    if (krynn()) c.ext[party::kExtRace] = 6;            // (a human in its own numbers)
}
int vfails = 0;
long vchecks = 0;
void vbad(const char* what, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
void vbad(const char* what, const char* fmt, ...)
{
    ++vfails;
    if (vfails > 60) return;
    char t[300];
    va_list a;
    va_start(a, fmt);
    vsnprintf(t, sizeof t, fmt, a);
    va_end(a);
    expect(P, what, false, "%s", t);
}
// The saves the oracle wants at level L (with Game Fixes the best of the levels up to it)
// (the oracle's `saveorig`: the original's own quirk where it differs from
// its table - Champions' knights; `savefix`: the Game Fixes' row where the
// table's is a mistake - the Dark Queen's empty rows past 18th)
bool want_saves(int k, int L, int out[5])
{
    auto row = [&](int l) {
        const std::vector<int>* r = rules::fixes() ? lvr("savefix", {k, l}) : lvr("saveorig", {k, l});
        return r ? r : lvr("save", {k, l});
    };
    const auto* r = row(L);
    if (!r || r->size() < 7) return false;
    for (int s = 0; s < 5; ++s) out[s] = (*r)[2 + s];
    for (int l = 1; rules::fixes() && l < L; ++l)
        if (const auto* e = row(l))
            for (int s = 0; s < 5; ++s) out[s] = std::min(out[s], (*e)[2 + s]);
    return true;
}
// Slots of every kind and level as the oracle's 27 (cleric, druid, magic-user x 9)
void got_slots(const party::Character& c, int out[27])
{
    for (int kind = 0; kind < 3; ++kind)
        for (int l = 0; l < 9; ++l) out[kind * 9 + l] = slot(c, kind, l);
}
std::string slot_text(const int* s)
{
    std::string t;
    for (int kind = 0; kind < 3; ++kind) {
        t += kind ? " |" : "";
        for (int l = 0; l < 9; ++l) t += " " + std::to_string(s[l + kind * 9]);
    }
    return t;
}
// Every value of a single class at its level against the oracle (V3-V5, V7, V8)
void check_level(const party::Character& c, int k, int tag, int L, int stat, const char* how)
{
    char w[200];
    if (tag == 0) {
        if (const auto* t = lvr("thac0", {k, L})) {
            ++vchecks;
            if (c.rec[0x73] != (*t)[2]) {
                snprintf(w, sizeof w, "V3 THAC0 class %d level %d (%s)", k, L, how);
                vbad(w, "%d (shown %d), the original's %d", c.rec[0x73], 60 - c.rec[0x73], (*t)[2]);
            }
        }
        int sv[5];
        if (want_saves(k, L, sv)) {
            ++vchecks;
            bool ok = true;
            for (int s = 0; s < 5; ++s) ok = ok && c.rec[0xDF + s] == sv[s];
            if (!ok) {
                snprintf(w, sizeof w, "V4 saves class %d level %d (%s)", k, L, how);
                vbad(w, "%d %d %d %d %d, the original's %d %d %d %d %d", c.rec[0xDF], c.rec[0xE0], c.rec[0xE1], c.rec[0xE2],
                     c.rec[0xE3], sv[0], sv[1], sv[2], sv[3], sv[4]);
            }
        }
        if (const auto* a = lvr("att", {k, L})) {
            ++vchecks;
            if (c.rec[0x11C] != (*a)[2]) {
                snprintf(w, sizeof w, "V5 attacks class %d level %d (%s)", k, L, how);
                vbad(w, "%d half attacks, the original's %d", c.rec[0x11C], (*a)[2]);
            }
        }
        if (k == 6)
            if (const auto* t = lvr("thief", {L, stat == 18 ? 18 : stat == 16 ? 14 : 9})) {
                ++vchecks;
                // (the originals' byte wraps below 0; Game Fixes: 0)
                int want[8];
                bool ok = true;
                for (int s = 0; s < 8; ++s) {
                    want[s] = rules::fixes() ? std::max(0, (*t)[2 + s]) : (*t)[2 + s] & 0xFF;
                    ok = ok && c.rec[0xEA + s] == want[s];
                }
                if (!ok) {
                    snprintf(w, sizeof w, "V8 thief skills level %d Dexterity %d (%s)", L, (*t)[1], how);
                    vbad(w, "%d %d %d %d %d %d %d %d, the original's %d %d %d %d %d %d %d %d", c.rec[0xEA], c.rec[0xEB],
                         c.rec[0xEC], c.rec[0xED], c.rec[0xEE], c.rec[0xEF], c.rec[0xF0], c.rec[0xF1], want[0], want[1],
                         want[2], want[3], want[4], want[5], want[6], want[7]);
                }
            }
    }
    if (const auto* s = lvr("slots", {k, tag, L, stat})) {
        ++vchecks;
        int got[27];
        got_slots(c, got);
        bool ok = true;
        for (int i = 0; i < 27; ++i) ok = ok && got[i] == (*s)[4 + i];
        if (!ok) {
            snprintf(w, sizeof w, "V7 slots class %d/%d level %d Wis/Int %d (%s)", k, tag, L, stat, how);
            vbad(w, "%s, the original's %s", slot_text(got).c_str(), slot_text(s->data() + 4).c_str());
        }
    }
}
// A human of class k (variant tag), every ability 18 but `stat` for Wisdom,
// Intelligence, Dexterity and `con` for Constitution
void fresh_v(party::Character& c, int k, int tag, int stat, int con)
{
    fresh(c, k);
    variant(c, tag);
    set_stat(c, 1, stat);
    set_stat(c, 2, stat);
    set_stat(c, 3, stat == 16 ? 14 : stat == 18 ? 18 : 9);
    set_stat(c, 4, con);
    classes::class_bonuses(c, m->tables);
}
void values()
{
    const char* g = game_name();
    if (!lvo_load(g)) {
        expect(P, "values: the oracle oracle_lv_<game>.txt", false, "none for %s", g);
        return;
    }
    static party::Character c, was;
    int classes_done = 0, levels_done = 0;
    const bool fixes0 = rules::fixes();
    for (const auto& cr : lvo["class"]) {
        const int k = cr[0], tag = cr[1], last = cr[2];
        if (k == 1 || (k == 7 && !krynn()) || last < 2) continue;      // (no druids; Curse's monk unused)
        ++classes_done;
        for (int fx = 0; fx < 2; ++fx) {
            rules::set_fixes(fx == 1);
            for (int con : {18, 16, 12, 5})
                for (int hi = 0; hi < 2; ++hi) {
                    if (tag && con != 18) continue;
                    char how[48];
                    snprintf(how, sizeof how, "fixes %s, Con %d, dice %s", fx ? "on" : "off", con, hi ? "high" : "low");
                    fresh_v(c, k, tag, 18, con);
                    if (con == 18 && hi) check_level(c, k, tag, 1, 18, how);
                    for (int L = 1; L < 50; ++L) {
                        char w[160];
                        const auto* xr = lvr("xp", {k, tag, L});
                        const int need = xr ? (*xr)[3] : -1;
                        const int bit = m->tables.u8(static_cast<uint16_t>(m->tables.lay.class_masks + k));
                        // V1 / V2
                        if (need < 0) {
                            put32(c.rec, 0x127, 0x3FFFFFFF);
                            ++vchecks;
                            if (create::trainable(c, m->tables) & bit) {
                                snprintf(w, sizeof w, "V1 last level class %d/%d (%s)", k, tag, how);
                                vbad(w, "trains past %d (the original's last %d)", L, last);
                            }
                            if (L != last) {
                                snprintf(w, sizeof w, "V1 last level class %d/%d", k, tag);
                                vbad(w, "stopped at %d, the original's %d", L, last);
                            }
                            break;
                        }
                        put32(c.rec, 0x127, static_cast<uint32_t>(need - 1));
                        const bool short_ok = !(create::trainable(c, m->tables) & bit);
                        put32(c.rec, 0x127, static_cast<uint32_t>(need));
                        const bool at_ok = (create::trainable(c, m->tables) & bit) != 0;
                        ++vchecks;
                        if (!short_ok || !at_ok) {
                            snprintf(w, sizeof w, "V2 experience class %d/%d level %d -> %d (%s)", k, tag, L, L + 1, how);
                            vbad(w, "%s at %d", !short_ok ? "trains one short" : "won't train", need);
                            put32(c.rec, 0x127, 0x3FFFFFFF);
                            if (!(create::trainable(c, m->tables) & bit)) break;
                        }
                        was = c;
                        create::g_die_hook = hi ? die_high : die_low;
                        create::train_classes(c, m->tables, m->facts, m->dice, bit, false);
                        create::g_die_hook = nullptr;
                        ++levels_done;
                        ++vchecks;
                        if (c.level(k) != L + 1) {
                            snprintf(w, sizeof w, "V1 one level class %d/%d level %d (%s)", k, tag, L, how);
                            vbad(w, "level %d after training", c.level(k));
                            break;
                        }
                        // (`hpfix`: the Game Fixes' gain where the original's is a mistake -
                        // the Krynn games' paladins, not fighters for their hit points)
                        const auto* hr = tag ? lvr("hpv", {k, tag, L + 1}) : nullptr;
                        if (!tag && rules::fixes()) hr = lvr("hpfix", {k, L + 1, con});
                        if (!tag && !hr) hr = lvr("hp", {k, L + 1, con});
                        if (hr) {
                            int want = (*hr)[tag ? 3 + (hi ? 0 : 1) : 3 + (hi ? 0 : 1)];
                            if (rules::fixes() && want < 1) want = 1;
                            const int up = c.hp_max() - was.hp_max();
                            ++vchecks;
                            if (up != want || c.hp() - was.hp() != up) {
                                snprintf(w, sizeof w, "V6 hit points class %d/%d level %d (%s)", k, tag, L + 1, how);
                                vbad(w, "+%d (now +%d), the original's +%d", up, c.hp() - was.hp(), want);
                            }
                        }
                        if (con == 18 && hi) check_level(c, k, tag, L + 1, 18, how);
                    }
                }
            // V7 / V8 at the other Wisdoms, Intelligences and Dexterities
            for (int st : {16, 13, 9}) {
                char how[48];
                snprintf(how, sizeof how, "fixes %s", fx ? "on" : "off");
                fresh_v(c, k, tag, st, 18);
                for (int L = 1; L <= last; ++L) {
                    if (lvr("slots", {k, tag, L, st}) || (k == 6 && st != 13)) check_level(c, k, tag, L, st, how);
                    if (L == last) break;
                    put32(c.rec, 0x127, 0x3FFFFFFF);
                    const int bit = m->tables.u8(static_cast<uint16_t>(m->tables.lay.class_masks + k));
                    if (!(create::trainable(c, m->tables) & bit)) break;
                    create::train_classes(c, m->tables, m->facts, m->dice, bit, false);
                }
            }
        }
    }
    rules::set_fixes(fixes0);
    char w[200];
    snprintf(w, sizeof w, "values: %d classes trained to their last levels (%d levels), %ld checks: V1-V8", classes_done,
             levels_done, vchecks);
    expect(P, w, classes_done > 0 && vfails == 0, "%d wrong", vfails);
}

// The classes a multi-class id holds (bits by class)
int multi_bits(int id)
{
    static const uint8_t bits[17] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
                                     0x05, 0x25, 0x11, 0x21, 0x41, 0x24, 0x44, 0x64, 0x60};
    return id >= 0 && id < 17 ? bits[id] : 0;
}
int ov(const char* key, std::initializer_list<int> pre, int at, int dflt)
{
    const auto* r = lvr(key, pre);
    return r && static_cast<int>(r->size()) > at ? (*r)[at] : dflt;
}
void multi()
{
    const char* g = game_name();
    if (lvo.empty() && !lvo_load(g)) {
        expect(P, "multi: the oracle oracle_lv_<game>.txt", false, "none for %s", g);
        return;
    }
    const int fam = ov("hpfam", {}, 0, -1);
    static party::Character c, was;
    const bool fixes0 = rules::fixes();
    int combos = 0, steps = 0;
    long checks0 = vchecks;
    const int fails0 = vfails;
    for (int id = 8; id <= 16; ++id) {
        // (a race whose list has it - its creation lists, the program's)
        int race = -1;
        for (int rc = 1; rc <= 7 && race < 0; ++rc) {
            int list[24];
            const int n = create::classes_for(m->tables, krynn() ? rc - 1 + classes::kKrynnRace : rc, list, 24);
            for (int i = 0; i < n; ++i)
                if (list[i] == id) race = rc;
        }
        if (race < 0) continue;
        const int bits = multi_bits(id);
        bool have = true;
        for (int k = 0; k < 8; ++k)
            if (bits >> k & 1) have = have && lvr("class", {k, 0}) != nullptr;
        if (!have) continue;
        ++combos;
        for (int fx = 0; fx < 2; ++fx)
            for (int con : {18, 16, 12, 5})
                for (int hi = 0; hi < 2; ++hi) {
                    rules::set_fixes(fx == 1);
                    char how[64];
                    snprintf(how, sizeof how, "fixes %s, Con %d, dice %s", fx ? "on" : "off", con, hi ? "high" : "low");
                    // A half-elf (no race saves; its limits only end the run sooner) of the multi-class, each class 1st
                    fresh(c, 0);
                    variant(c, 0);
                    c.rec[0x74] = 4;
                    c.rec[0x75] = static_cast<uint8_t>(id);
                    for (int k = 0; k < 8; ++k) c.rec[0x109 + k] = static_cast<uint8_t>(bits >> k & 1);
                    set_stat(c, 4, con);
                    classes::class_bonuses(c, m->tables);
                    int held = 0;
                    for (int k = 0; k < 8; ++k) held += bits >> k & 1;
                    for (int step = 0; step < 150; ++step) {
                        // the lowest class that may train
                        put32(c.rec, 0x127, 0x3FFFFFFF);
                        const int ready = create::trainable(c, m->tables);
                        int pick = -1;
                        for (int k = 0; k < 8; ++k)
                            if ((bits >> k & 1) && (m->tables.u8(static_cast<uint16_t>(m->tables.lay.class_masks + k)) & ready) &&
                                (pick < 0 || c.level(k) < c.level(pick)))
                                pick = k;
                        if (pick < 0) break;
                        const int bit = m->tables.u8(static_cast<uint16_t>(m->tables.lay.class_masks + pick));
                        was = c;
                        create::g_die_hook = hi ? die_high : die_low;
                        create::train_classes(c, m->tables, m->facts, m->dice, bit, false);
                        create::g_die_hook = nullptr;
                        ++steps;
                        char w[200];
                        int lv[8];
                        for (int k = 0; k < 8; ++k) lv[k] = c.level(k);
                        ++vchecks;
                        if (lv[pick] != was.level(pick) + 1) {
                            snprintf(w, sizeof w, "M1 one level multi %d class %d (%s)", id, pick, how);
                            vbad(w, "%d -> %d", was.level(pick), lv[pick]);
                            break;
                        }
                        // V6m: hit points
                        if (fam >= 0) {
                            const int L = lv[pick];
                            const int sides = ov("hd", {pick}, 2, 0), stop = ov("hd", {pick}, 3, 99);
                            const bool dice = fam == 3 || L < stop;
                            const int n = fam == 3 ? ov("hd", {pick}, 1, 1) : 1;
                            int past = ov("past", {pick}, 1, 0);
                            if (rules::fixes() && lvr("pastfix", {pick})) past = ov("pastfix", {pick}, 1, past);
                            int fig = ov("mfig", {pick, con}, 2, 0);
                            if (rules::fixes() && lvr("mfigfix", {pick, con})) fig = ov("mfigfix", {pick, con}, 2, fig);
                            const int roll = dice ? n * (hi ? sides : 1) : past;
                            int want;
                            if (fam == 0 || fam == 3) {
                                int adj = 0;
                                for (int k = 0; k < 8; ++k)
                                    if (lv[k] > 0 && (fam == 3 || lv[k] < ov("hd", {k}, 3, 99))) adj += ov("mfig", {k, con}, 2, 0);
                                want = std::max(1, (roll + adj) / held);
                            } else {
                                const int inc = std::max(1, roll / held);
                                const int cb = (dice ? fig : 0) & 0xFF;
                                int cp = rules::fixes() ? static_cast<int8_t>(cb) / held : cb / held;
                                if (rules::fixes() && inc + cp < 1) cp = 1 - inc;
                                want = inc + cp;
                            }
                            const int up = static_cast<uint8_t>(c.hp_max() - was.hp_max());
                            ++vchecks;
                            if (up != (want & 0xFF)) {
                                snprintf(w, sizeof w, "V6m hit points multi %d class %d level %d (%s)", id, pick, L, how);
                                vbad(w, "+%d, the original's +%d", up, want);
                            }
                        }
                        if (con != 18 || !hi) continue;
                        // V3 / V4 / V5 / V7: the classes' best, the slots' sum
                        int t0 = 0, sv[5] = {99, 99, 99, 99, 99}, att = 0, sl[27] = {};
                        bool ok = true;
                        for (int k = 0; k < 8 && ok; ++k) {
                            if (!lv[k]) continue;
                            const auto* t = lvr("thac0", {k, lv[k]});
                            const auto* a = lvr("att", {k, lv[k]});
                            const auto* s = lvr("slots", {k, 0, lv[k], 18});
                            int s5[5];
                            if (!t || !a || !s || !want_saves(k, lv[k], s5)) ok = false;
                            if (!ok) break;
                            t0 = std::max(t0, (*t)[2]);
                            att = std::max(att, (*a)[2]);
                            for (int i = 0; i < 5; ++i) sv[i] = std::min(sv[i], s5[i]);
                            for (int i = 0; i < 27; ++i) sl[i] += (*s)[4 + i];
                        }
                        if (!ok) continue;
                        // (the Wisdom bonus once: each class's row has it only where it is the cleric's)
                        vchecks += 4;
                        if (c.rec[0x73] != t0) {
                            snprintf(w, sizeof w, "V3 THAC0 multi %d levels %d/%d/%d", id, lv[0], lv[2], lv[5]);
                            vbad(w, "%d, the classes' best %d", c.rec[0x73], t0);
                        }
                        bool sok = true;
                        for (int i = 0; i < 5; ++i) sok = sok && c.rec[0xDF + i] == sv[i];
                        if (!sok) {
                            snprintf(w, sizeof w, "V4 saves multi %d levels %d/%d/%d/%d/%d (%s)", id, lv[0], lv[2], lv[4], lv[5],
                                     lv[6], how);
                            vbad(w, "%d %d %d %d %d, the classes' best %d %d %d %d %d", c.rec[0xDF], c.rec[0xE0], c.rec[0xE1],
                                 c.rec[0xE2], c.rec[0xE3], sv[0], sv[1], sv[2], sv[3], sv[4]);
                        }
                        if (c.rec[0x11C] != att) {
                            snprintf(w, sizeof w, "V5 attacks multi %d levels %d/%d/%d", id, lv[2], lv[3], lv[4]);
                            vbad(w, "%d, the classes' best %d", c.rec[0x11C], att);
                        }
                        int got[27];
                        got_slots(c, got);
                        if (memcmp(got, sl, sizeof got) != 0) {
                            snprintf(w, sizeof w, "V7 slots multi %d levels %d/%d/%d/%d", id, lv[0], lv[4], lv[5], lv[3]);
                            vbad(w, "%s, the classes' sum %s", slot_text(got).c_str(), slot_text(sl).c_str());
                        }
                    }
                }
    }
    rules::set_fixes(fixes0);
    char w[200];
    snprintf(w, sizeof w, "multi: %d multi-classes trained a class at a time (%d trainings), %ld checks: V3-V7, V6m", combos,
             steps, vchecks - checks0);
    expect(P, w, combos > 0 && vfails == fails0, "%d wrong", vfails - fails0);
}

void run()
{
    if (!monkey_boot()) {
        expect(P, "the game booted", false);
        return;
    }
    if (!play::load_rules(m) || !m) {
        expect(P, "rule tables", false);
        return;
    }
    if (getenv("LVDUMP")) {                 // (the save table of one class, levels 1-40)
        const int k = atoi(getenv("LVDUMP"));
        for (int L = 1; L <= 40; ++L)
            printf("  save table class %d level %d: %d %d %d %d %d\n", k, L, m->tables.save(k, L, 0), m->tables.save(k, L, 1),
                   m->tables.save(k, L, 2), m->tables.save(k, L, 3), m->tables.save(k, L, 4));
    }
    const char* only = getenv("ONLY");
    if (!only || strstr(only, "classes")) classes_up();
    if (!only || strstr(only, "wisdom")) wisdom();
    if (!only || strstr(only, "money")) money();
    if (!only || strstr(only, "camp")) camp();
    if (!only || strstr(only, "values")) values();
    if (!only || strstr(only, "multi")) multi();
}

} // namespace blv

void behave_levels() { blv::run(); }
