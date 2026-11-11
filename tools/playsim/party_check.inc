// The party checker (rule tests beyond fights, 2026-11-01): the rules
// every party member's record keeps whatever the player does - exploring,
// camp, spells, items, shops, training - checked after each of the
// monkey's actions (and the camp fuzz's). The rules come from the games'
// own limits and AD&D's, never from the engine's code:
//   P1 hit points never above the maximum (a record stored over it may stay
//      down to there), never wrapped round (a byte: 255 after 0)
//   P2 one Okay stands with hit points; one with none is not Okay
//   P3 coins never wrap round (u16: 65,535 after 0) nor go up out of
//      nothing outside a shop, a treasure, a script or Take
//   P4 an item's charges / count never wrap round (255 after 0)
//   P5 no more spells memorized of a kind and level than its slots
//   P6 abilities in use 1-25
//   P7 levels within the game's last level
//   P8 (Tom, 2026-11-11: statuses last as long as they should) out of a
//      fight, effects follow the game clock: a timed one loses the minutes it
//      moved, one at or under that step ends, a lasting one (0) stays as it
//      is - checked when no fight came between (-DCYD_TEST_HOOKS)
// Breaks print "!! PARTY Pn" (counted in `problems`).

namespace pcheck {

using namespace play;
long checks = 0;
std::map<std::string, int> broken;
int print_max = 6;

struct Seen {
    char name[16] = {};
    int  hp = -1, hp_cap = 0;
    int  money[7] = {};
    uint8_t charges[party::kMaxItems] = {}, count[party::kMaxItems] = {};
    int  n_items = 0;
    std::vector<std::pair<uint8_t, int>> fx;    // (P8: type, minutes)
};
long fx_clock = -1;                     // (P8: the game clock's minutes at the last look)
bool fought = false;                    // (a fight since the last look)
long p8_ticked = 0, p8_ended = 0, p8_new = 0;
Seen seen[party::kMaxParty];

void bad(const char* rule, const std::string& what)
{
    const int n = ++broken[rule];
    ++problems;
    if (n <= print_max) printf("!! PARTY %s: %s [screen %d]\n", rule, what.c_str(), (int)play::screen);
}

std::string who(const party::Character& c)
{
    char nm[24];
    c.name(nm, sizeof nm);
    char t[96];
    snprintf(t, sizeof t, "%s (HP %d/%d, status %d)", nm, c.hp(), c.hp_max(), c.rec[0x195]);
    return t;
}

void seen_fx_of(const party::Character& c)
{
    for (int i = 0; i < pt->count && i < party::kMaxParty; ++i)
        if (&pt->m[i] == &c) {
            seen[i].fx.clear();
            for (int k = 0; k < c.n_affects; ++k) seen[i].fx.push_back({c.affects[k][0], c.affects[k][1] | c.affects[k][2] << 8});
        }
}

void check()
{
    if (!pt || !d) return;
    if (fg) {                           // (a fight's fighters are the fight checker's)
        fought = true;
        return;
    }
    ++checks;
#ifdef CYD_TEST_HOOKS
    const long clock = vm ? vm->minutes_all() : -1;
    const int dt = fx_clock >= 0 && clock >= 0 ? static_cast<int>(clock - fx_clock) : -1;
    const bool p8 = dt >= 0 && !fought;
#endif
    for (int i = 0; i < pt->count && i < party::kMaxParty; ++i) {
        const party::Character& c = pt->m[i];
        Seen& s = seen[i];
        char nm[16];
        c.name(nm, sizeof nm);
        const bool same = !strcmp(nm, s.name) && s.hp >= 0;
        if (!same) {
            s = Seen{};
            strncpy(s.name, nm, sizeof s.name - 1);
            s.hp_cap = c.hp() > c.hp_max() ? c.hp() : c.hp_max();
        }
#ifdef CYD_TEST_HOOKS
        // P8 (each type's times as a set: one type may be held twice)
        if (p8 && same) {
            std::map<int, std::vector<int>> was_t, now_t;
            for (auto& w : s.fx) was_t[w.first].push_back(w.second);
            for (int k = 0; k < c.n_affects; ++k) now_t[c.affects[k][0]].push_back(c.affects[k][1] | c.affects[k][2] << 8);
            for (auto& [type, wl] : was_t) {
                std::multiset<int> got;
                if (now_t.count(type)) got.insert(now_t[type].begin(), now_t[type].end());
                // (first the lasting and the counted-down, then the rest judged)
                std::vector<int> left;
                for (int w : wl) {
                    const int want = w == 0 ? 0 : w - dt;
                    auto g = (w == 0 || w > dt) ? got.find(want) : got.end();
                    if (g != got.end()) {
                        got.erase(g);
                        if (w && dt) ++p8_ticked;
                    } else left.push_back(w);
                }
                for (int w : left) {
                    if (w == 0) continue;                       // (a lasting one taken: an act - a cure)
                    if (w > dt) {
                        // (not a minute less: taken by an act, or - still at w - not ticked)
                        if (dt > 0 && got.count(w)) {
                            bad("P8", "effect " + std::to_string(type) + " still " + std::to_string(w) + " minutes after the clock moved " + std::to_string(dt) + ": " + who(c));
                            got.erase(got.find(w));
                        }
                        continue;
                    }
                    ++p8_ended;
                    for (auto g = got.begin(); g != got.end(); ++g)
                        if (*g && *g <= w) {
                            bad("P8", "effect " + std::to_string(type) + " at " + std::to_string(w) + " minutes still there (" + std::to_string(*g) + ") after the clock moved " + std::to_string(dt) + ": " + who(c));
                            got.erase(g);
                            break;
                        }
                }
                p8_new += static_cast<long>(got.size());
            }
        }
        s.fx.clear();
        for (int k = 0; k < c.n_affects; ++k) s.fx.push_back({c.affects[k][0], c.affects[k][1] | c.affects[k][2] << 8});
#endif
        // P1
        const int cap = c.hp_max() > s.hp_cap ? c.hp_max() : s.hp_cap;
        if (c.hp() > cap) bad("P1", "hit points over the maximum: " + who(c));
        if (same && s.hp >= 0 && s.hp <= 5 && c.hp() >= 250 && c.hp_max() < 250) bad("P1", "hit points wrapped round: " + who(c));
        if (c.hp() <= c.hp_max()) s.hp_cap = c.hp_max();
        s.hp = c.hp();                  // (until 2026-11-11 never kept: every check against the last look - P1's wrap, P3, P4 - was off)
        // P2
        if (c.rec[0x195] == party::Okay && c.hp() <= 0) bad("P2", "Okay at 0 hit points: " + who(c));
        // P3
        for (int k = 0; k < 7; ++k) {
            const int v = c.money(k);
            if (same && s.money[k] <= 1000 && v >= 60000) bad("P3", "coins wrapped round (kind " + std::to_string(k) + "): " + who(c));
            s.money[k] = v;
        }
        // P4
        for (int k = 0; k < c.n_items && k < party::kMaxItems; ++k) {
            const uint8_t ch = c.items[k][0x3C], ct = c.items[k][0x39];
            if (same && k < s.n_items && s.charges[k] <= 3 && ch >= 250 && ch != s.charges[k])
                bad("P4", "an item's charges wrapped round: " + who(c));
            if (same && k < s.n_items && s.count[k] <= 3 && ct >= 250 && ct != s.count[k])
                bad("P4", "an item's count wrapped round: " + who(c));
            s.charges[k] = ch;
            s.count[k] = ct;
        }
        s.n_items = c.n_items;
        // P5 (with the magic tables in hand)
        if (mrules) {
            const classes::Tables& t = mrules->tables;
            int mem[3][9] = {};
            for (int k = 0; k < magic::kListSize; ++k) {
                const int sp = c.rec[magic::kListAt + k] & 0x7F;
                if (!sp || sp >= t.lay.spell_count) continue;
                const int kind = t.spell_class(sp), lvl = t.spell_level(sp);
                if (kind < 0 || kind > 2 || lvl < 1 || lvl > 9) continue;
                ++mem[kind][lvl - 1];
            }
            // (Krynn: a mage's moon gives up to 2 more among levels 1-5 -
            // one phase from full 1, full 2; Champions' GAME.OVR 0x1B31A)
            int moon_left = t.lay.krynn ? 2 : 0;
            for (int kind = 0; kind < 3; ++kind)
                for (int l = 0; l < 9; ++l) {
                    if (!mem[kind][l]) continue;
                    const int slots = l < 5 ? c.rec[0x12D + kind * 5 + l] : 0;
                    if (kind == 2 && l < 5 && slots > 0 && mem[kind][l] > slots && mem[kind][l] - slots <= moon_left) {
                        moon_left -= mem[kind][l] - slots;
                        continue;
                    }
                    if (l < 5 && mem[kind][l] > slots) {
                        char t2[96];
                        snprintf(t2, sizeof t2, "%d memorized of kind %d level %d, %d slots: ", mem[kind][l], kind, l + 1, slots);
                        bad("P5", t2 + who(c));
                    }
                }
        }
        // P6
        for (int k = 0; k < 6; ++k) {
            const int v = c.rec[0x11 + 2 * k];
            if (c.rec[0x195] == party::Okay && (v < 1 || v > 25))
                bad("P6", "ability " + std::to_string(k) + " in use " + std::to_string(v) + ": " + who(c));
        }
        // P7
        if (mrules)
            for (int k = 0; k < 8; ++k)
                if (c.level(k) > mrules->tables.top_level())
                    bad("P7", "class " + std::to_string(k) + " level " + std::to_string(c.level(k)) + ": " + who(c));
    }
#ifdef CYD_TEST_HOOKS
    fx_clock = vm ? vm->minutes_all() : -1;
    // P8FX=1: now and then a timed effect put on a member (the charm, the
    // Protection - nothing they do out of a fight), 1-120 minutes, to follow
    if (getenv("P8FX") && pt->count && pick(15) == 0) {
        const auto& ff = d->prof->fight.facts;
        const uint8_t t = pick(2) ? ff.charm : ff.prot_evil;
        party::Character& c = pt->m[pick(pt->count)];
        if (t && c.n_affects + 1 < party::kMaxAffects) {
            spells::add_affect(c, t, 1 + pick(120), 0, false);
            seen_fx_of(c);
        }
    }
#endif
    fought = false;
}

void report()
{
    printf("== party checks: %ld looks; P8 effects %ld counted down, %ld ran out, %ld new;", checks, p8_ticked, p8_ended, p8_new);
    if (broken.empty()) printf(" no rule broken\n");
    else {
        for (const auto& kv : broken) printf(" %s x%d", kv.first.c_str(), kv.second);
        printf("\n");
    }
}

} // namespace pcheck
