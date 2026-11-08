#include "magic.h"

#include <cstring>

#include "rules.h"

namespace magic {

namespace {

constexpr int kInt = 0x13, kWis = 0x15;

// Spell slots (levels 1-5; Secret's 6-7 too)
int cast_count(const party::Character& c, const classes::Tables& t, int cls, int level)
{
    return level <= t.slot_levels() ? classes::slots(c, cls, level) : 0;
}

bool usable(const classes::Tables& t, int s)
{
    return s >= 1 && s < t.lay.spell_count && t.spell_class(s) < 3 && t.spell_level(s) >= 1 &&
           t.spell_level(s) <= t.slot_levels();
}

void sort_by_level(const classes::Tables& t, uint8_t* ids, int n)
{
    for (int i = 1; i < n; ++i)
        for (int j = i; j > 0; --j) {
            const int a = t.spell_level(ids[j - 1]), b = t.spell_level(ids[j]);
            if (a < b || (a == b && ids[j - 1] <= ids[j])) break;
            const uint8_t x = ids[j];
            ids[j] = ids[j - 1];
            ids[j - 1] = x;
        }
}

// The first spell being memorized: its list slot, or -1 (in list order)
int first_learning(const party::Character& c)
{
    for (int i = kListSize - 1; i >= 0; --i)
        if (c.rec[kListAt + i] & 0x80) return i;
    return -1;
}

} // namespace

// A class's level over n, or (a dual-class human past the class before)
// the level it had (Champions' 0xF7:0x48 test with the old levels)
static bool over(const party::Character& c, int cls, int n)
{
    if (c.level(cls) > n) return true;
    return classes::skill_level(c, cls) != c.level(cls) && c.old_level(cls) > n;
}

bool can_use(const party::Character& c, const classes::Tables& t, int s)
{
    if (t.lay.queen && s >= 1 && s < t.lay.spell_count) {
        // The Dark Queen (DQK.EXE ovl 0xA file 0x5A311, by its spell row's
        // class word; levels, no former ones): cleric spells and the gods'
        // specials - Wisdom over 8 and a cleric, a knight past 5th or a
        // paladin past 8th; druid spells - Wisdom over 8 and a ranger past
        // 7th; magic-user spells - Intelligence over 8 and a magic-user or a
        // ranger past 8th (a human magic-user in armour, ranger to 8th, not
        // in fights - not here); the items' none
        constexpr int kKnight = 7;
        switch (t.spell_code(s)) {
        case 0:
        case 5:
            if (t.spell_code(s) == 0 && !usable(t, s)) return false;
            return c.rec[kWis] > 8 && (c.level(classes::Cleric) > 0 || c.level(kKnight) > 5 || c.level(classes::Paladin) > 8);
        case 1: return usable(t, s) && c.rec[kWis] > 8 && c.level(classes::Ranger) > 7;
        case 3:
            return usable(t, s) && c.rec[kInt] > 8 &&
                   (c.level(classes::MagicUser) > 0 || c.level(classes::Ranger) > 8);
        default: return false;
        }
    }
    if (!usable(t, s)) return false;
    if (t.lay.krynn && !t.lay.queen) {
        // Champions (GAME.OVR 0x2DC8E) / Death Knights (0x29C84): clerics,
        // knights past 5th level (Death Knights: paladins past 8th; not
        // Champions'); druid spells rangers past 6th (Death Knights: 7th);
        // magic-user spells as the others' outside fights
        constexpr int kKnight = 7;
        switch (t.spell_class(s)) {
        case Cleric:
            if (t.lay.dkk) return c.rec[kWis] > 8 && (c.level(classes::Cleric) > 0 || c.level(kKnight) > 5 ||
                                                     c.level(classes::Paladin) > 8);
            return c.rec[kWis] > 8 && (over(c, classes::Cleric, 0) || over(c, kKnight, 5));
        case Druid:
            return c.rec[kWis] > 8 && (t.lay.dkk ? c.level(classes::Ranger) > 7 : over(c, classes::Ranger, 6));
        case MagicUser: return c.rec[kInt] > 8;
        default: return false;
        }
    }
    switch (t.spell_class(s)) {
    case Cleric:
        return c.rec[kWis] > 8 &&
               (classes::skill_level(c, classes::Cleric) > 0 || classes::skill_level(c, classes::Paladin) > 8);
    case Druid: return c.rec[kWis] > 8 && classes::skill_level(c, classes::Ranger) > 6;
    case MagicUser:
        // Secret (GAME.OVR 0x2C71E): a magic-user, or a ranger over 8th level
        // (its human magic-users in armour can't in fights - not here)
        if (t.lay.secret)
            return c.rec[kInt] > 8 && (classes::skill_level(c, classes::MagicUser) > 0 ||
                                       classes::skill_level(c, classes::Ranger) > 8);
        return c.rec[kInt] > 8;
    default: return false;
    }
}

int held(const party::Character& c, const classes::Tables& t, int cls, int level)
{
    int n = 0;
    for (int i = 0; i < kListSize; ++i) {
        const int s = c.rec[kListAt + i] & 0x7F;
        if (s && t.spell_class(s) == cls && t.spell_level(s) == level) ++n;
    }
    return n;
}

namespace {
int g_moons[3] = {};
}

void set_moons(int white, int red, int black)
{
    g_moons[0] = white;
    g_moons[1] = red;
    g_moons[2] = black;
}

int moon(int i) { return i >= 0 && i < 3 ? g_moons[i] : 0; }

int moon_bonus(const party::Character& c, const classes::Tables& t)
{
    // (Champions' memorizing, GAME.OVR 0x1B31A: a mage's moon - the robe's
    // - one phase from full gives 1 more spell, full 2, among levels 1-5;
    // what is already over the slots there is taken from it, all of it
    // when more)
    if (!t.lay.krynn || c.level(classes::MagicUser) <= 0) return 0;
    const int robe = c.ext[party::kExtRobe];
    if (robe < 1 || robe > 3) return 0;
    const int ph = g_moons[robe - 1];
    int bonus = ph == 1 ? 1 : ph == 2 ? 2 : 0;
    for (int lv = 1; lv <= 5 && bonus > 0; ++lv) {
        const int over = held(c, t, MagicUser, lv) - cast_count(c, t, MagicUser, lv);
        if (over <= 0) continue;
        bonus = over > bonus ? 0 : bonus - over;
    }
    return bonus;
}

int room(const party::Character& c, const classes::Tables& t, int cls, int level)
{
    int r = cast_count(c, t, cls, level) - held(c, t, cls, level);
    if (r < 0) r = 0;
    if (cls == MagicUser && level <= 5 && cast_count(c, t, cls, level) > 0) r += moon_bonus(c, t);
    return r;
}

bool any_room(const party::Character& c, const classes::Tables& t)
{
    for (int cls = 0; cls < 3; ++cls)
        for (int lv = 1; lv <= t.slot_levels(); ++lv)
            if (cast_count(c, t, cls, lv) > 0) return true;
    return false;
}

int in_memory(const party::Character& c, const classes::Tables& t, bool learning, uint8_t* ids, int cap)
{
    int n = 0;
    for (int i = kListSize - 1; i >= 0 && n < cap; --i) {
        const uint8_t v = c.rec[kListAt + i];
        if (!v || ((v & 0x80) != 0) != learning) continue;
        // (the Dark Queen's god-given spells: memorized, cast from a level of
        // their own, "Special" - spell_class 4; its can-learn takes them as
        // a cleric's)
        if (!can_use(c, t, v & 0x7F)) continue;
        ids[n++] = v & 0x7F;
    }
    sort_by_level(t, ids, n);
    return n;
}

int known(const party::Character& c, const classes::Tables& t, uint8_t* ids, int cap)
{
    int n = 0;
    for (int s = 1; s < t.lay.spell_count && n < cap; ++s)
        if (classes::in_book(c, s) && can_use(c, t, s)) ids[n++] = static_cast<uint8_t>(s);
    sort_by_level(t, ids, n);
    return n;
}

int learnable(const party::Character& c, const classes::Tables& t, uint8_t* ids, int cap)
{
    int n = 0;
    if (t.lay.queen && t.lay.q_mu_lists) {
        // The Dark Queen's (its list mode 5, ovl 0xA +0x41C0 - queen_facts_4.md
        // 8): for each spell level with magic-user slots, the spells of its
        // robe's list (White's, else the Red / Black one) not yet known
        const int list = c.ext[party::kExtRobe] == 1 ? 0 : 1;
        for (int lv = 1; lv <= 9 && n < cap; ++lv) {
            if (cast_count(c, t, 2, lv) <= 0) continue;
            for (int i = 0; i < 10 && n < cap; ++i) {
                const int s = t.u8(static_cast<uint16_t>(t.lay.q_mu_lists + list * 90 + (lv - 1) * 10 + i));
                if (!s) break;
                if (!classes::in_book(c, s)) ids[n++] = static_cast<uint8_t>(s);
            }
        }
        return n;
    }
    if (t.lay.mu_lists) {
        // Secret / Champions / Death Knights: the magic-user's lists (classes::Layout)
        const bool krynn = t.lay.krynn;
        const int levels = krynn ? (t.lay.dkk ? 8 : 4) : 7, per = krynn ? 10 : 11;
        const int base = t.lay.mu_lists + (krynn ? (c.ext[party::kExtRobe] == 1 ? 2 : 3) * (t.lay.dkk ? 80 : 40) : 0);
        for (int lv = 1; lv <= levels && n < cap; ++lv) {
            if (cast_count(c, t, MagicUser, lv) <= 0) continue;
            for (int i = 1; i <= per && n < cap; ++i) {
                const int s = t.u8(static_cast<uint16_t>(base + lv * per + i));
                if (!s) break;
                if (!classes::in_book(c, s)) ids[n++] = static_cast<uint8_t>(s);
            }
        }
        return n;
    }
    // PoolRad (GAME.OVR 0x276CA): spells 1-56 of a class and level with
    // slots, not known; Curse / Gateway (0x356F2): 1-100, also one the
    // character can learn; Darkness / Treasures (0x2A8BE, 0x337FC): 1-126,
    // magic-user spells only, of a level with magic-user slots, not known
    const int top = t.lay.dk ? 126 : t.lay.pool ? 56 : 100;
    for (int s = 1; s < t.lay.spell_count && s <= top && n < cap; ++s) {
        if (!usable(t, s) || classes::in_book(c, s)) continue;
        if (t.lay.dk ? t.spell_class(s) == MagicUser && cast_count(c, t, MagicUser, t.spell_level(s)) > 0
                     : cast_count(c, t, t.spell_class(s), t.spell_level(s)) > 0 && (t.lay.pool || can_use(c, t, s)))
            ids[n++] = static_cast<uint8_t>(s);
    }
    sort_by_level(t, ids, n);
    return n;
}

void learn(party::Character& c, int s) { classes::to_book(c, s); }

bool add(party::Character& c, const classes::Tables& t, int s)
{
    if (!usable(t, s) || room(c, t, t.spell_class(s), t.spell_level(s)) <= 0) return false;
    for (int i = kListSize - 1; i >= 0; --i)
        if (!c.rec[kListAt + i]) {
            c.rec[kListAt + i] = static_cast<uint8_t>(s | 0x80);
            return true;
        }
    return false;
}

void cancel(party::Character& c)
{
    for (int i = 0; i < kListSize; ++i)
        if (c.rec[kListAt + i] & 0x80) c.rec[kListAt + i] = 0;
    c.rec[kToLearnAt] = 0;
}

bool memorizing(const party::Character& c) { return first_learning(c) >= 0; }

bool remove(party::Character& c, int s, bool relearn)
{
    for (int i = 0; i < kListSize; ++i)
        if (c.rec[kListAt + i] == s) {
            c.rec[kListAt + i] = static_cast<uint8_t>(relearn ? s | 0x80 : 0);
            return true;
        }
    return false;
}

bool reads_spell(const party::Character& c, const classes::Tables& t, int s)
{
    if (!usable(t, s)) return false;
    using classes::skill_level;
    switch (t.spell_class(s)) {
    case Cleric:
        return c.rec[kWis] > 8 && (skill_level(c, classes::Cleric) > 0 || skill_level(c, classes::Paladin) > 8);
    case Druid: return c.rec[kWis] > 8 && skill_level(c, classes::Ranger) > 6;
    case MagicUser:
        return c.rec[kInt] > 8 && (skill_level(c, classes::MagicUser) > 0 || skill_level(c, classes::Ranger) > 8 ||
                                   skill_level(c, classes::Thief) > 10);
    default: return false;
    }
}

bool is_scroll(const Scrolls& sc, const uint8_t* it)
{
    if (sc.types[0]) return it[0x2E] == sc.types[0] || it[0x2E] == sc.types[1];
    if (!sc.names) return false;
    const int slot = sc.names->type(it[0x2E]).slot;
    return slot >= 11 && slot <= 13;
}

int scroll_spells(party::Character& c, const classes::Tables& t, const Scrolls& sc, bool scribing, uint8_t* ids,
                  int cap)
{
    int n = 0;
    const bool cleric = classes::skill_level(c, classes::Cleric) > 0;
    for (int i = 0; i < c.n_items; ++i) {
        uint8_t* it = c.items[i];
        if (!is_scroll(sc, it)) continue;
        if ((sc.read_magic && c.has_affect(sc.read_magic)) || (cleric && sc.names->type(it[0x2E]).slot == 12))
            it[0x35] = 0;                   // read: its spells are known
        if (it[0x35]) continue;
        for (int k = 0; k < 3 && n < cap; ++k) {
            const uint8_t v = it[kScrollAt + k];
            if (sc.readers && !reads_spell(c, t, v & 0x7F)) continue;
            if (scribing ? v > 0x80 : v > 0) ids[n++] = v & 0x7F;
        }
    }
    if (!sc.unsorted) sort_by_level(t, ids, n);
    return n;
}

Scribe scribe(party::Character& c, const classes::Tables& t, const Scrolls& sc, int s)
{
    if (classes::in_book(c, s)) return Scribe::Known;
    for (int i = 0; i < c.n_items; ++i)
        if (is_scroll(sc, c.items[i]))
            for (int k = 0; k < 3; ++k)
                if (c.items[i][kScrollAt + k] == (s | 0x80)) return Scribe::Already;
    if (cast_count(c, t, t.spell_class(s), t.spell_level(s)) <= 0) return Scribe::Cannot;
    for (int i = 0; i < c.n_items; ++i)
        if (is_scroll(sc, c.items[i]))
            for (int k = 0; k < 3; ++k)
                if (c.items[i][kScrollAt + k] == s) {
                    c.items[i][kScrollAt + k] = static_cast<uint8_t>(s | 0x80);
                    return Scribe::Ok;
                }
    return Scribe::Cannot;
}

bool scribing(const party::Character& c, const Scrolls& sc)
{
    for (int i = 0; i < c.n_items; ++i)
        if (is_scroll(sc, c.items[i]))
            for (int k = 0; k < 3; ++k)
                if (c.items[i][kScrollAt + k] > 0x80) return true;
    return false;
}

void cancel_scribes(party::Character& c, const Scrolls& sc)
{
    for (int i = 0; i < c.n_items; ++i)
        if (is_scroll(sc, c.items[i]))
            for (int k = 0; k < 3; ++k) c.items[i][kScrollAt + k] &= 0x7F;
}

void scribed(party::Character& c, const Scrolls& sc, int i, int k)
{
    if (i < 0 || i >= c.n_items || k < 0 || k > 2) return;
    uint8_t* it = c.items[i];
    classes::to_book(c, it[kScrollAt + k] & 0x7F);
    it[kScrollAt + k] = 0;
    it[sc.count_at] = static_cast<uint8_t>(it[sc.count_at] - 1);
    if (it[sc.count_at] < sc.one_spell) {
        // Used up
        for (int j = i; j + 1 < c.n_items; ++j) memcpy(c.items[j], c.items[j + 1], party::kItemSize);
        --c.n_items;
        memset(c.items[c.n_items], 0, party::kItemSize);
    }
}

namespace {
void remove_at(party::Character& c, int i)
{
    for (int j = i; j + 1 < c.n_items; ++j) memcpy(c.items[j], c.items[j + 1], party::kItemSize);
    --c.n_items;
    memset(c.items[c.n_items], 0, party::kItemSize);
}
} // namespace

bool usable(const Scrolls& sc, const uint8_t* it)
{
    return !is_scroll(sc, it) && it[0x3D] != 0 && it[0x3E] < 0x80;
}

bool use_charge(uint8_t* it)
{
    if (it[0x3C] == 0) return false;            // never runs out
    if (it[0x39] > 1) {
        --it[0x39];
        return false;
    }
    --it[0x3C];
    return it[0x3C] == 0;
}

void used(party::Character& c, int i)
{
    if (i < 0 || i >= c.n_items) return;
    if (use_charge(c.items[i])) remove_at(c, i);
}

int scroll_list(party::Character& c, const classes::Tables& t, const Scrolls& sc, int i, uint8_t* ids, int cap)
{
    if (i < 0 || i >= c.n_items || !is_scroll(sc, c.items[i])) return 0;
    uint8_t* it = c.items[i];
    const bool cleric = classes::skill_level(c, classes::Cleric) > 0;
    if ((sc.read_magic && c.has_affect(sc.read_magic)) || (cleric && sc.names->type(it[0x2E]).slot == 12))
        it[0x35] = 0;
    if (it[0x35]) return 0;
    int n = 0;
    for (int k = 0; k < 3 && n < cap; ++k)
        if ((it[kScrollAt + k] & 0x7F) && (!sc.readers || reads_spell(c, t, it[kScrollAt + k] & 0x7F)))
            ids[n++] = it[kScrollAt + k] & 0x7F;
    sort_by_level(t, ids, n);
    return n;
}

ScrollReader scroll_reader(const party::Character& c, const classes::Tables& t)
{
    // PoolRad (GAME.OVR 0x22B30): the scroll's spell cast with no test
    if (t.lay.pool) return kReads;
    // Darkness (0x257E5) / Treasures (0x2E2B6): skill levels - magic-user or
    // cleric, else a thief's over 9th and the d100
    if (t.lay.dk) {
        if (classes::skill_level(c, classes::MagicUser) > 0 || classes::skill_level(c, classes::Cleric) > 0) return kReads;
        return classes::skill_level(c, classes::Thief) > 9 ? kD100 : kOops;
    }
    // Curse (0x29613), Secret, Gateway, Champions, Death Knights: the class
    // (or the class before) a magic-user or cleric, else a thief over 9th
    if (reads_scroll(c, 100)) return kReads;
    return c.level(classes::Thief) >= 10 ? kD100 : kOops;
}

bool reads_scroll(const party::Character& c, int d100)
{
    if (c.level(classes::Cleric) > 0 || c.level(classes::MagicUser) > 0 || c.old_level(classes::Cleric) > 0 ||
        c.old_level(classes::MagicUser) > 0)
        return true;
    return c.level(classes::Thief) >= 10 && d100 <= 75;
}

void scroll_used(party::Character& c, const Scrolls& sc, int i, int spell)
{
    if (i < 0 || i >= c.n_items) return;
    uint8_t* it = c.items[i];
    int line = -1;
    for (int k = 0; k < 3; ++k)
        if ((it[kScrollAt + k] & 0x7F) == spell) line = k;
    if (line < 0) return;
    it[kScrollAt + line] = 0;
    it[sc.count_at] = static_cast<uint8_t>(it[sc.count_at] - 1);
    if (it[sc.count_at] < sc.one_spell) remove_at(c, i);
}

// The next spell to scribe (item, slot) in the games' order, or false
static bool next_scribe(const party::Character& c, const Scrolls& sc, int* item, int* slot)
{
    for (int i = 0; i < c.n_items; ++i)
        if (is_scroll(sc, c.items[i]))
            for (int k = 0; k < 3; ++k)
                if (c.items[i][kScrollAt + k] > 0x80) {
                    *item = i;
                    *slot = k;
                    return true;
                }
    return false;
}

int rest_minutes(party::Character& c, const classes::Tables& t, const Scrolls& sc)
{
    int max_level = 0, total = 0;
    for (int i = 0; i < kListSize; ++i) {
        const uint8_t v = c.rec[kListAt + i];
        if (!(v & 0x80)) continue;
        const int lv = t.spell_level(v & 0x7F);
        if (lv > max_level) max_level = lv;
        total += lv;
    }
    for (int i = 0; i < c.n_items; ++i)
        if (is_scroll(sc, c.items[i]))
            for (int k = 0; k < 3; ++k) {
                const uint8_t v = c.items[i][kScrollAt + k];
                if (v <= 0x80) continue;
                const int lv = t.spell_level(v & 0x7F);
                if (lv > max_level) max_level = lv;
                total += lv;
            }
    int hours = 0;
    if (total > 0) hours = 4;
    if (max_level > 2) hours = 6;
    c.rec[kToLearnAt] = static_cast<uint8_t>(hours);
    return hours * 60 + total * 15;
}

void begin(Rest& r) { r = Rest{}; }

int tick_affects(party::Character& c, int minutes, uint8_t paladin_cure)
{
    int ended = 0;
    for (int i = 0; i < c.n_affects;) {
        uint8_t* a = c.affects[i];
        const int m = a[1] | a[2] << 8;
        if (m == 0) {
            ++i;
            continue;
        }
        if (m > minutes) {
            const int left = m - minutes;
            a[1] = static_cast<uint8_t>(left);
            a[2] = static_cast<uint8_t>(left >> 8);
            ++i;
            continue;
        }
        if (paladin_cure && a[0] == paladin_cure && c.level(3) > 0)
            c.rec[0x191] = static_cast<uint8_t>((c.level(3) - 1) / 5 + 1);
        for (int k = i; k + 1 < c.n_affects; ++k) memcpy(c.affects[k], c.affects[k + 1], party::kAffectSize);
        --c.n_affects;
        memset(c.affects[c.n_affects], 0, party::kAffectSize);
        ++ended;
    }
    return ended;
}

// The level of what comes next (a spell to scribe first, then one to
// memorize), 0: nothing
static int next_level(const party::Character& c, const classes::Tables& t, const Scrolls& sc)
{
    int i, k;
    if (next_scribe(c, sc, &i, &k)) return t.spell_level(c.items[i][kScrollAt + k] & 0x7F);
    const int next = first_learning(c);
    return next >= 0 ? t.spell_level(c.rec[kListAt + next] & 0x7F) : 0;
}

Step step(Rest& r, party::Party& p, const classes::Tables& t, const Scrolls& sc, bool tick)
{
    Step st;
    for (int& l : st.learnt) l = 0;
    for (int& l : st.scribed) l = 0;
    // A day's rest heals a point
    if (++r.steps >= 8 * 36) {
        r.steps = 0;
        for (int i = 0; i < p.count; ++i) rules::heal(p.m[i], 1);
        st.healed = true;
    }
    // Spells come one after another once the start has passed: each
    // takes 3 steps a level
    for (int i = 0; i < p.count; ++i) {
        party::Character& c = p.m[i];
        if (r.wait[i] > 0) --r.wait[i];
        if (r.wait[i] == 0 && c.rec[kToLearnAt] == 0) {
            int item, slot;
            const int at = first_learning(c);
            if (next_scribe(c, sc, &item, &slot)) {
                st.scribed[i] = c.items[item][kScrollAt + slot] & 0x7F;
                scribed(c, sc, item, slot);
                r.wait[i] = next_level(c, t, sc) * 3;
            } else if (at >= 0) {
                const int s = c.rec[kListAt + at] & 0x7F;
                c.rec[kListAt + at] = static_cast<uint8_t>(s);
                st.learnt[i] = s;
                r.wait[i] = next_level(c, t, sc) * 3;
            }
        }
    }
    // An hour: the start counts down
    if (++r.hour >= 12) {
        r.hour = 0;
        for (int i = 0; i < p.count; ++i) {
            party::Character& c = p.m[i];
            if (c.rec[kToLearnAt] > 0 && --c.rec[kToLearnAt] == 0) r.wait[i] = next_level(c, t, sc) * 2;
        }
    }
    for (int i = 0; tick && i < p.count; ++i) tick_affects(p.m[i], 5, r.paladin_cure);
    return st;
}

} // namespace magic
