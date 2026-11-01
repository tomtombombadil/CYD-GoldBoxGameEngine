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
// and the cleric's Wisdom bonus (AD&D's Players Handbook, the table every
// game's training follows - coab's / the listings' cleric slot routine):
// Wisdom 13 +1 1st, 14 +1 1st, 15 +1 2nd, 16 +1 2nd, 17 +1 3rd, 18 +1 4th,
// each only at a spell level the cleric already has slots for; none for
// paladins, rangers or druids (their slots never follow Wisdom). The
// expectations are the rules', never the engine's tables.

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
}

} // namespace blv

void behave_levels() { blv::run(); }
