// Behaviour checks: money, goods and weight in every game (sweep's `behave
// economy`, -DBEHAVE_ECONOMY, any game - GAME, BOOTSLOT and the rest as the
// fuzzer's; ONLY= weight, armour, carry, sell, buy, id, share, take,
// appraise, cheats, wear, rest). Each part drives the engine's own screens'
// code (Sell / Id / Buy / Share / Take / Appraise) on a booted party and
// checks what the originals' programs do - the routines read in each
// game's GAME.OVR (addresses at each rule) - never the engine's numbers:
//   W1 the carried weight: every item's weight x its count (0: one) and
//      every coin (Curse GAME.OVR 0x38322, Darkness 0x34DBA)
//   A1 readied armour: 150 or less the base movement, 151-399 9, more 6;
//      +3 at 9 or less only for armour with a plus (byte 0x32 not 0)
//   C1 carried past the Strength allowance: 0x200 or less no change, to
//      0x300 9, to 0x400 6, more 3 (Curse 0x37A9A, every game the same)
//   S1 Sell's price: value / 2; a pile: Curse's family count x / 20 (every
//      pile), Secret / Darkness / Treasures arrows and quarrels / 20, the
//      others x count (Secret's 0x49 one's price); 16-bit products
//   S2 Sell's coins: Curse / Pool / Gateway platinum + the odd gold, the
//      platinum the seller has room for (the rest on the counter);
//      Champions steel; Secret its own (platinum first); Darkness's engine
//      platinum - nothing lost, nobody past the load
//   I1 Id's price: Curse / Pool / Gateway 200 gold, Secret 100 gold,
//      Champions 100 steel, Darkness's engine 20 platinum - the seller's,
//      else the counter's; not enough: nothing changes; the item known
//   B1 Buy's price by the shop's factor (1/16 .. x8, 0 counts 1; Darkness's
//      engine x the count); the item added; nothing when too poor / heavy
//   T1 Share (combat_rules_facts 6.5): coins kept; nobody past the load;
//      even shares (each the same or one more, the odd ones one each);
//      coins left only when nobody has room; NPCs none
//   T2 Take: never more than lies there; too heavy: nothing moves
//   G1 Appraise: a gem / jewel less, its value paid or kept as an item
//   H1 the Cheats tab: the Healer's cures free, the Merchant's prices at
//      a shop's factor (never above the value)
//   R1 items worn and put away: the record and effects as before
//   R2 rest: a point of healing a day; an effect of N minutes gone after N

namespace beco {

using namespace play;
const char* P = "economy";
char w[256];
std::mt19937 r(11);
int rn(int n) { return n > 0 ? static_cast<int>(r() % static_cast<unsigned>(n)) : 0; }

enum Fam { kCurseF, kSecretF, kChampF, kDarkF, kOtherF };
Fam fam()
{
    if (secret_files()) return kSecretF;
    if (dk_ui()) return kDarkF;
    if (krynn()) return kChampF;
    return kCurseF;                     // Curse, Pool, Gateway
}

std::vector<std::vector<uint8_t>> recs;     // the game's own item records

void collect()
{
    recs.clear();
    setenv("ALLITEMS", "1", 1);
    for (const TestItem& t : test_items()) recs.emplace_back(t.rec, t.rec + 63);
    for (int i = 0; i < pt->count; ++i)
        for (int k = 0; k < pt->m[i].n_items; ++k) {
            std::vector<uint8_t> v(pt->m[i].items[k], pt->m[i].items[k] + 63);
            v[0x34] = 0;
            recs.push_back(v);
        }
}

static party::Party saved;
void keep() { saved = *pt; }
void back() { *pt = saved; pt->selected = 0; }

int u16(const uint8_t* p) { return p[0] | p[1] << 8; }
void set_money(party::Character& c, int k, int v)
{
    c.rec[0xFB + k * 2] = static_cast<uint8_t>(v);
    c.rec[0xFC + k * 2] = static_cast<uint8_t>(v >> 8);
}
// A plain member to work on: member 0, no items, no effects, no coins
party::Character& bare()
{
    party::Character& c = pt->m[0];
    pt->selected = 0;
    c.n_items = 0;
    c.n_affects = 0;
    for (int k = 0; k < 7; ++k) set_money(c, k, 0);
    c.rec[0x195] = party::Okay;
    if (c.hp() <= 0) c.rec[0x1A4] = 1;
    rules::recalc(c, *names, d->facts);
    return c;
}
int fails = 0;
void bad(const char* rule, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
void bad(const char* rule, const char* fmt, ...)
{
    ++fails;
    static std::map<std::string, int> per;
    if (++per[rule] > 6) return;
    char t[200];
    va_list a;
    va_start(a, fmt);
    vsnprintf(t, sizeof t, fmt, a);
    va_end(a);
    expect(P, rule, false, "%s", t);
}
void summary(const char* what, int cases, int before)
{
    snprintf(w, sizeof w, "%s (%d cases)", what, cases);
    expect(P, w, cases > 0 && fails == before, "%d wrong", fails - before);
}
long worth(const int* m) { return rules::copper_worth(m); }
long worth(const party::Character& c) { return rules::copper_worth(c); }

// ---- W1
void weight()
{
    const int f0 = fails;
    int cases = 0;
    for (int k = 0; k < 400 && !recs.empty(); ++k) {
        party::Character& c = bare();
        const int n = rn(std::min(party::kMaxItems, 12) + 1);
        long want = 0;
        for (int i = 0; i < n; ++i) {
            rules::add_item(c, recs[rn(static_cast<int>(recs.size()))].data());
            uint8_t* it = c.items[c.n_items - 1];
            if (it[0x39] > 0 && rn(2)) it[0x39] = static_cast<uint8_t>(1 + rn(40));
            const int wt = u16(it + 0x37), cnt = it[0x39];
            want += (cnt > 0 ? (wt * cnt) & 0xFFFF : wt);
        }
        for (int m = 0; m < 7; ++m) {
            const int v = rn(3) ? rn(300) : 0;
            set_money(c, m, v);
            want += v;
        }
        rules::recalc(c, *names, d->facts);
        ++cases;
        if (c.encumbrance() != (want & 0xFFFF)) bad("W1 the carried weight", "%d, items x count + coins %ld", c.encumbrance(), want);
    }
    back();
    summary("W1 the carried weight: items x count and every coin", cases, f0);
}

// ---- A1 / C1
void armour()
{
    const int f0 = fails;
    int cases = 0;
    const uint8_t* plate = nullptr;
    for (const auto& g : recs)
        if (names->type(g[0x2E]).slot == items::kSlotArmour && (names->type(g[0x2E]).ac & 0x7F)) plate = g.data();
    if (!plate) {
        expect(P, "A1 an armour among the game's items", false);
        return;
    }
    for (int wt : {100, 150, 151, 399, 400, 450, 600})
        for (int plus : {0, 1, 3, -1}) {
            party::Character& c = bare();
            c.rec[0xE4] = 12;
            rules::add_item(c, plate);
            uint8_t* it = c.items[0];
            it[0x34] = 1;
            it[0x36] = 0;
            it[0x37] = static_cast<uint8_t>(wt);
            it[0x38] = static_cast<uint8_t>(wt >> 8);
            it[0x32] = static_cast<uint8_t>(plus);
            it[0x39] = 0;
            rules::recalc(c, *names, d->facts);
            // (a load past the allowance would slow too: the armour's weight alone)
            if (wt - rules::max_encumbrance(c) > 0x200) continue;
            int want = wt <= 150 ? 12 : wt <= 399 ? 9 : 6;
            if (plus != 0 && want <= 9) want += 3;
            ++cases;
            if (c.movement() != want) bad("A1 armour's movement", "weight %d plus %d: %d, the programs' %d", wt, plus, c.movement(), want);
        }
    back();
    summary("A1 armour's movement: by its weight, +3 for magic armour only", cases, f0);
    const int f1 = fails;
    cases = 0;
    for (int over : {0, 1, 0x200, 0x201, 0x300, 0x301, 0x400, 0x401, 0x900}) {
        party::Character& c = bare();
        c.rec[0xE4] = 12;
        const int coins = rules::max_encumbrance(c) + over;
        if (coins < 0 || coins > 0xFFFF) continue;
        set_money(c, dk_ui() ? 4 : 3, coins);
        rules::recalc(c, *names, d->facts);
        const int want = over <= 0x200 ? 12 : over <= 0x300 ? 9 : over <= 0x400 ? 6 : 3;
        ++cases;
        if (c.movement() != want) bad("C1 carrying slows", "%d past the allowance: %d, the programs' %d", over, c.movement(), want);
    }
    back();
    summary("C1 carried past the Strength allowance slows by 0x200 / 0x300 / 0x400", cases, f1);
}

// ---- S1 / S2
int sell_want(const uint8_t* it)
{
    int v = u16(it + 0x3A) / 2;
    const int n = it[0x39], t = it[0x2E];
    if (n <= 1) return v;
    const bool dark_rule = d->facts.sell_secret;
    if (!dark_rule) return ((n * v) & 0xFFFF) / 20;
    if (secret_files() && t == 0x49) return v;
    if (t == d->facts.arrow || t == d->facts.quarrel) return ((n * v) & 0xFFFF) / 20;
    return (n * v) & 0xFFFF;
}
void sell()
{
    const int f0 = fails, f1 = fails;
    int cases = 0;
    const bool fix_was = rules::fixes();
    for (int k = 0; k < 600 && !recs.empty(); ++k) {
        rules::set_fixes(k % 2 == 1);
        party::Character& c = bare();
        ground->clear();
        rules::add_item(c, recs[rn(static_cast<int>(recs.size()))].data());
        uint8_t* it = c.items[0];
        it[0x34] = it[0x36] = 0;
        if (rn(3) == 0) it[0x39] = static_cast<uint8_t>(2 + rn(60));
        if (rn(2)) {
            const int v = rn(4) ? rn(2000) : rn(65536);
            it[0x3A] = static_cast<uint8_t>(v);
            it[0x3B] = static_cast<uint8_t>(v >> 8);
        }
        // a purse, sometimes near the load
        const int coin = dk_ui() ? 4 : krynn() ? 3 : 4;
        set_money(c, coin, rn(3) ? rn(200) : std::max(0, rules::max_load(c) - rn(60) - u16(it + 0x37)));
        rules::recalc(c, *names, d->facts);
        if (c.encumbrance() > rules::max_load(c)) continue;
        const int price = rules::sell_value(it, d->facts);
        const int want = sell_want(it);
        ++cases;
        if (!dk_ui() || d->facts.sell_secret)       // (Death Knights / the Dark Queen: their own code, not read)
            if (price != want && !(dkk() || hl()))
                bad("S1 Sell's price", "type %d count %d value %d: %d, the program's %d", it[0x2E], it[0x39], u16(it + 0x3A), price, want);
        int before[7], ctr[7];
        for (int m = 0; m < 7; ++m) before[m] = c.money(m), ctr[m] = ground->money[m];
        const long w0 = worth(c) + worth(ground->money);
        // what the seller has room for once the item is gone
        party::Character probe = c;
        rules::remove_item(probe, 0);
        rules::recalc(probe, *names, d->facts);
        int room = rules::max_load(probe) - probe.encumbrance();
        if (room < 0) room = 0;
        screen = Screen::Items;
        view_from = Screen::Shop;
        sell_item(0, C);
        items_yes_no(Ask::SellDeal, 'Y', C);
        note_until = 0;
        if (c.n_items != 0) {
            bad("S2 Sell takes the item", "still %d items", c.n_items);
            continue;
        }
        if (rules::fixes() && fam() != kChampF && fam() != kDarkF) {
            // (Game Fixes' coins made the highest: the worth counts)
            const long got = worth(c) + worth(ground->money) - w0;
            const long want_c = static_cast<long>(price / 5) * 1000 + (price % 5) * 200;
            if (got != want_c) bad("S2 Sell's coins (Game Fixes)", "worth +%ld copper, the price's %ld", got, want_c);
        } else if (fam() == kCurseF) {
            const int plat = price / 5, g = price % 5;
            const bool over = plat + g > room;
            const int p = over && plat > room ? room : plat;
            if (c.money(4) - before[4] != p || ground->money[4] - ctr[4] != plat - p || c.money(3) - before[3] != g)
                bad("S2 Sell's coins (Curse's family)", "price %d room %d: platinum +%d (counter +%d) gold +%d; the program's %d (%d) %d",
                    price, room, c.money(4) - before[4], ground->money[4] - ctr[4], c.money(3) - before[3], p, plat - p, g);
        } else if (fam() == kChampF) {
            const int p = price > room ? room : price;
            if (c.money(3) - before[3] != p || ground->money[3] - ctr[3] != price - p || c.money(4) != before[4])
                bad("S2 Sell's coins (Champions: steel)", "price %d room %d: steel +%d (counter +%d) platinum +%d; the program's %d (%d)",
                    price, room, c.money(3) - before[3], ground->money[3] - ctr[3], c.money(4) - before[4], p, price - p);
        } else if (fam() == kDarkF) {
            const int p = price > room ? room : price;
            if (c.money(4) - before[4] != p || ground->money[4] - ctr[4] != price - p)
                bad("S2 Sell's coins (platinum)", "price %d room %d: platinum +%d (counter +%d); the program's %d (%d)", price, room,
                    c.money(4) - before[4], ground->money[4] - ctr[4], p, price - p);
        } else {
            const long got = worth(c) + worth(ground->money) - w0;
            const long want_c = static_cast<long>(price / 5) * 1000 + (price % 5) * 200;
            if (got != want_c) bad("S2 Sell's coins (Secret)", "worth +%ld copper, the price's %ld", got, want_c);
        }
        rules::recalc(c, *names, d->facts);
        if (c.encumbrance() > rules::max_load(c) && fam() != kCurseF)
            bad("S2 nobody past the load", "%d of %d after selling for %d", c.encumbrance(), rules::max_load(c), price);
    }
    rules::set_fixes(fix_was);
    (void)f1;
    back();
    ground->clear();
    summary("S1 / S2 Sell: the price and the coins as the programs pay them", cases, f0);
}

// ---- I1
int id_price() { return dk_ui() ? 20 : secret_files() || krynn() ? 100 : 200; }
void identify()
{
    const int f0 = fails;
    int cases = 0;
    const bool fix_was = rules::fixes();
    for (int k = 0; k < 300 && !recs.empty(); ++k) {
        rules::set_fixes(k % 2 == 1);
        party::Character& c = bare();
        ground->clear();
        rules::add_item(c, recs[rn(static_cast<int>(recs.size()))].data());
        c.items[0][0x35] = dk_ui() ? 5 : 1;
        const int coin = dk_ui() ? 4 : krynn() ? 3 : 4;
        const int cost = id_price();
        // the purse in the game's coin (Champions: steel alone - its other
        // coins' worths aren't the engine's yet)
        const int unit = dk_ui() || krynn() ? cost : cost / 5;       // coins of that kind
        const int have = rn(3) == 0 ? rn(unit) : unit + rn(400);
        set_money(c, coin, have);
        if (rn(3) == 0) ground->money[coin] = rn(3) ? unit + rn(50) : rn(unit);
        if (!dk_ui() && !krynn() && rn(3) == 0) set_money(c, 1, rn(500));      // (silver: the worth rounds)
        rules::recalc(c, *names, d->facts);
        const long w0 = worth(c), g0 = worth(ground->money);
        const int own0 = c.money(coin), ctr0 = ground->money[coin];
        screen = Screen::Items;
        view_from = Screen::Shop;
        identify_item(0, C);
        items_yes_no(Ask::IdDeal, 'Y', C);
        note_until = 0;
        items_after_note = false;
        ++cases;
        const bool known = dk_ui() ? !(c.items[0][0x35] & 7) : c.items[0][0x35] == 0;
        if (dk_ui() || krynn()) {
            const bool can = own0 >= cost || (dk_ui() ? own0 + ctr0 >= cost : ctr0 >= cost);
            int want_own = own0, want_ctr = ctr0;
            if (own0 >= cost) want_own = own0 - cost;
            else if (can && dk_ui()) want_own = 0, want_ctr = ctr0 - (cost - own0);
            else if (can) want_ctr = ctr0 - cost;
            if (c.money(coin) != want_own || ground->money[coin] != want_ctr || known != can)
                bad("I1 Id's price", "%d own %d counter: %d / %d known %d; the program's %d / %d (%d)", own0, ctr0, c.money(coin),
                    ground->money[coin], known, want_own, want_ctr, can);
            continue;
        }
        // gold: the worth (Curse's family rounds it; Game Fixes: exact)
        const bool set = !rules::fixes() && (pool_files() || d->prof->game == games::Game::CurseOfTheAzureBonds || gateway());
        const long cw = static_cast<long>(cost) * 200;
        const long own_w = set ? (w0 + 100) / 200 * 200 : w0, ctr_w = set ? (g0 + 100) / 200 * 200 : g0;
        long want_own = w0, want_ctr = g0;
        bool can = true;
        if (rules::fixes()) {
            if (w0 >= cw) want_own = w0 - cw;
            else if (w0 + g0 >= cw) want_own = 0, want_ctr = g0 - (cw - w0);
            else can = false;
        } else if (own_w >= cw) {
            want_own = set ? (own_w - cw) / 200 * 200 : w0 - cw;
        } else if (ctr_w >= cw) {
            want_ctr = set ? (ctr_w - cw) / 200 * 200 : g0 - cw;
        } else {
            can = false;
        }
        if (worth(c) != want_own || worth(ground->money) != want_ctr || known != can)
            bad("I1 Id's price", "worths %ld / %ld -> %ld / %ld known %d; the program's %ld / %ld (%d)", w0, g0, worth(c),
                worth(ground->money), known, want_own, want_ctr, can);
    }
    rules::set_fixes(fix_was);
    back();
    ground->clear();
    summary("I1 Id: the game's price from the seller or the counter, the item known", cases, f0);
}

// ---- B1
void buy_check()
{
    const int f0 = fails;
    int cases = 0;
    const bool fix_was = rules::fixes();
    const uint16_t f_was = vm->get(0x7F6D);
    for (int k = 0; k < 400 && !recs.empty(); ++k) {
        rules::set_fixes(k % 2 == 1);
        party::Character& c = bare();
        ground->clear();
        const int f = 1 << rn(8);
        vm->set(0x7F6D, static_cast<uint16_t>(f));
        memcpy(ground->item[0], recs[rn(static_cast<int>(recs.size()))].data(), 63);
        uint8_t* g = ground->item[0];
        g[0x34] = 0;
        ground->n = 1;
        const int value = u16(g + 0x3A);
        long want = value ? value : 1;
        want = f == 0x10 ? want : f < 0x10 ? want >> (f == 1 ? 4 : f == 2 ? 3 : f == 4 ? 2 : 1) : want << (f == 0x20 ? 1 : f == 0x40 ? 2 : 3);
        if (dk_ui()) {
            if (g[0x39] > 1) want = (want * g[0x39]) & 0xFFFF;      // (one item: the shift's product as it is - its wrap not read)
            if (!want) want = 1;
        }
        const int coin = dk_ui() ? 4 : 3;
        const long per = dk_ui() || krynn() ? 1 : 5;          // coins of that kind for a price unit (gold: platinum)
        const int pcoin = dk_ui() || krynn() ? coin : 4;
        const long need = (want + per - 1) / per;
        const int have = rn(3) == 0 ? static_cast<int>(rn(static_cast<int>(std::min(need, 60000L)) + 1)) : static_cast<int>(std::min(need + rn(100), 60000L));
        set_money(c, pcoin, have);
        rules::recalc(c, *names, d->facts);
        const int n0 = c.n_items;
        const long w0 = worth(c);
        const int own0 = c.money(pcoin);
        const bool heavy = rules::too_heavy(c, g, *names, d->facts);
        const int shown = shop_price(g);
        buy(0, C);
        note_until = 0;
        ++cases;
        if (!dk_ui() && rules::price(g, f) != want) bad("B1 Buy's price", "value %d factor %d: %d, the rule's %ld", value, f, rules::price(g, f), want);
        if (dk_ui() && shown != want) bad("B1 Buy's price", "value %d x %d factor %d: %d, the rule's %ld", value, g[0x39], f, shown, want);
        const bool afford = dk_ui() || krynn() ? own0 >= want : w0 >= want * 200;
        const bool got = c.n_items == n0 + 1;
        if (got != (afford && !heavy)) {
            bad("B1 Buy: bought only when it can be paid and carried", "got %d, afford %d heavy %d (price %ld, %d coins)", got, afford, heavy, want, own0);
            continue;
        }
        if (!got) {
            if (worth(c) != w0) bad("B1 Buy: refused, nothing paid", "worth %ld -> %ld", w0, worth(c));
            continue;
        }
        if (dk_ui() || krynn()) {
            if (c.money(pcoin) != own0 - want) bad("B1 Buy: the price paid", "%d -> %d, price %ld", own0, c.money(pcoin), want);
        } else {
            const bool set = !rules::fixes() && (pool_files() || d->prof->game == games::Game::CurseOfTheAzureBonds || gateway());
            const long wantw = set ? ((w0 + 100) / 200 - want) * 200 : w0 - want * 200;
            if (worth(c) != wantw) bad("B1 Buy: the price paid", "worth %ld -> %ld, the program's %ld (price %ld)", w0, worth(c), wantw, want);
        }
        if (memcmp(c.items[n0], g, 0x34) || memcmp(c.items[n0] + 0x35, g + 0x35, 63 - 0x35))
            if (!items::fix_item(g)) bad("B1 Buy: the item as the shop had it", "type %d", g[0x2E]);
    }
    vm->set(0x7F6D, f_was);
    rules::set_fixes(fix_was);
    back();
    ground->clear();
    summary("B1 Buy: the price by the shop's factor, paid, the item added", cases, f0);
}

// ---- T1 / T2
void share()
{
    const int f0 = fails;
    int cases = 0;
    for (int k = 0; k < 800; ++k) {
        back();
        const int n = pt->count;
        int pool0[7];
        for (int m = 0; m < 7; ++m) pool0[m] = ground->money[m] = rn(3) ? rn(m >= 5 ? 20 : 3000) : 0;
        int mem0[party::kMaxParty][7];
        for (int i = 0; i < n; ++i) {
            party::Character& c = pt->m[i];
            for (int m = 0; m < 7; ++m) set_money(c, m, rn(4) == 0 ? rn(2000) : 0);
            if (rn(5) == 0) set_money(c, 4, std::max(0, rules::max_load(c) - rn(400)));
            rules::recalc(c, *names, d->facts);
            for (int m = 0; m < 7; ++m) mem0[i][m] = c.money(m);
        }
        int pcs = 0;
        for (int i = 0; i < n; ++i) pcs += !pt->m[i].npc();
        bool roomy = true;
        long tot_free = 0;
        for (int i = 0; i < n; ++i) {
            const party::Character& c = pt->m[i];
            if (c.npc()) continue;
            int sum = 0;
            for (int m = 0; m < 7; ++m) sum += pool0[m] / std::max(pcs, 1) + 1;
            if (c.encumbrance() + sum > rules::max_load(c)) roomy = false;
            tot_free += std::max(0, rules::max_load(c) - c.encumbrance());
        }
        rules::share(*pt, ground->money);
        ++cases;
        for (int m = 0; m < 7; ++m) {
            long sum = ground->money[m];
            for (int i = 0; i < n; ++i) sum += pt->m[i].money(m);
            long sum0 = pool0[m];
            for (int i = 0; i < n; ++i) sum0 += mem0[i][m];
            if (sum != sum0) bad("T1 Share keeps the coins", "kind %d: %ld -> %ld", m, sum0, sum);
        }
        for (int i = 0; i < n; ++i) {
            party::Character& c = pt->m[i];
            rules::recalc(c, *names, d->facts);
            int gain = 0;
            for (int m = 0; m < 7; ++m) gain += c.money(m) - mem0[i][m];
            if (c.npc() && gain) bad("T1 NPCs take no share", "member %d +%d", i, gain);
            if (gain > 0 && c.encumbrance() > rules::max_load(c)) bad("T1 nobody past the load", "member %d %d of %d", i, c.encumbrance(), rules::max_load(c));
        }
        if (roomy && pcs) {
            for (int m = 0; m < 7; ++m) {
                const int each = pool0[m] / pcs;
                int odd = 0;
                for (int i = 0; i < n; ++i) {
                    if (pt->m[i].npc()) continue;
                    const int gain = pt->m[i].money(m) - mem0[i][m];
                    if (gain != each && gain != each + 1) bad("T1 even shares", "kind %d: member %d +%d, each %d", m, i, gain, each);
                    odd += gain - each;
                }
                if (odd != pool0[m] % pcs || ground->money[m]) bad("T1 the odd coins one each", "kind %d: %d given, %d odd; %d left", m, odd, pool0[m] % pcs, ground->money[m]);
            }
        }
        bool left = false;
        for (int m = 0; m < 7; ++m) left |= ground->money[m] > 0;
        if (left)
            for (int i = 0; i < n; ++i)
                if (!pt->m[i].npc() && rules::max_load(pt->m[i]) - pt->m[i].encumbrance() > 0)
                    bad("T1 coins left only when nobody has room", "member %d has %d room", i, rules::max_load(pt->m[i]) - pt->m[i].encumbrance());
    }
    back();
    ground->clear();
    summary("T1 Share: even, odd coins one each, the load kept, nothing lost", cases, f0);
    // T2 Take
    const int f1 = fails;
    cases = 0;
    for (int k = 0; k < 300; ++k) {
        party::Character& c = bare();
        ground->clear();
        const int kind = rn(dk_ui() ? 3 : 7) + (dk_ui() ? 4 : 0);
        ground->money[kind] = 1 + rn(3000);
        if (rn(3) == 0) set_money(c, 4, std::max(0, rules::max_load(c) - rn(100)));
        rules::recalc(c, *names, d->facts);
        const int there = ground->money[kind], own = c.money(kind), room = rules::max_load(c) - c.encumbrance();
        const int ask = rn(2) ? there + rn(50) : 1 + rn(there);
        const long w_before = worth(c) + worth(ground->money);
        take_mode = 0;
        take_coin = kind;
        screen = Screen::Take;
        take_coins(ask, C);
        note_until = 0;
        ++cases;
        if (rules::fixes()) {
            // (Game Fixes: the coins made the highest - the worth counts)
            if (kind < 5 && worth(c) + worth(ground->money) != static_cast<long>(own) * 0 + w_before)
                bad("T2 Take keeps the coins (Game Fixes)", "worth %ld -> %ld", w_before, worth(c) + worth(ground->money));
            continue;
        }
        const int took = c.money(kind) - own;
        if (took + ground->money[kind] != there) bad("T2 Take keeps the coins", "%d there, took %d, %d left", there, took, ground->money[kind]);
        if (took > there) bad("T2 never more than lies there", "%d of %d", took, there);
        const int want = std::min(ask, there);
        if (!rules::fixes() && took != (want <= room ? want : 0))
            bad("T2 Take: all asked, or nothing when too heavy", "asked %d (%d there, room %d): took %d", ask, there, room, took);
    }
    back();
    ground->clear();
    screen = Screen::Game;
    summary("T2 Take: never more than there, nothing when too heavy", cases, f1);
}

// ---- G1
void appraise()
{
    const int f0 = fails;
    int cases = 0;
    for (int k = 0; k < 200; ++k) {
        party::Character& c = bare();
        ground->clear();
        const int jewel = rn(2);
        set_money(c, 5 + jewel, 1 + rn(3));
        if (rn(4) == 0) set_money(c, 4, std::max(0, rules::max_load(c) - rn(30)));
        rules::recalc(c, *names, d->facts);
        const int g0 = c.money(5 + jewel), n0 = c.n_items;
        const long w0 = worth(c) + worth(ground->money);
        screen = Screen::Appraise;
        appraised = 0;
        appraise_key(jewel ? 'J' : 'G', C);
        const int v = appraised;
        ++cases;
        if (c.money(5 + jewel) != g0 - 1 || v <= 0) {
            bad("G1 a gem / jewel appraised", "count %d -> %d, value %d", g0, c.money(5 + jewel), v);
            continue;
        }
        const bool keep = rn(2) && c.n_items < party::kMaxItems;
        appraise_key(keep ? 'K' : 'S', C);
        note_until = 0;
        if (keep && c.n_items == n0 + 1) {
            if (u16(c.items[n0] + 0x3A) != v) bad("G1 kept: an item of its value", "%d, valued %d", u16(c.items[n0] + 0x3A), v);
            continue;
        }
        const long got = worth(c) + worth(ground->money) - w0;
        const long want = dk_ui() ? v * 1000L : (v / 5) * 1000L + (rules::fixes() ? (v % 5) * 200 : 0);
        if (got != want && !krynn()) bad("G1 sold: its value", "worth +%ld copper, valued %d (%ld)", got, v, want);
    }
    back();
    ground->clear();
    screen = Screen::Game;
    summary("G1 Appraise: one less, its value paid or kept", cases, f0);
}

// ---- H1
void cheats()
{
    const int f0 = fails;
    int cases = 0;
    back();
    // Merchant: every good's price at a power-of-two factor no more than the value
    if (cheats_ready() || (CYD_CHEATS && (opts.cheats = true) && cheats_ready())) {
        cheat_open(1, C);
        if (cheat == Cheat::Merchant) {
            const int f = vm->get(0x7F6D) & 0xFF;
            ++cases;
            if (!f || (f & (f - 1)) || f > 0x10) bad("H1 the Merchant's factor", "%d", f);
            for (int i = 0; i < goods_count(); ++i) {
                const uint8_t* g = goods(i);
                const int v = u16(g + 0x3A) ? u16(g + 0x3A) : 1;
                ++cases;
                if (rules::price(g, f) > v) bad("H1 the Merchant's price never above the value", "%d of %d", rules::price(g, f), v);
            }
            cheat_end(C);
        } else {
            expect(P, "H1 the Merchant opens", screen == Screen::Game, "screen %d", (int)screen);
        }
        back();
        // Healer: a cure paid for nothing
        cheat_open(0, C);
        if (cheat == Cheat::Healer && temple) {
            party::Character& c = pt->m[0];
            c.rec[0x1A4] = 1;
            const long w0 = worth(c) + worth(ground->money);
            open_heal(C);
            cure_at = 5;                    // (Heal)
            shop_yes_no(Ask::PayCure, 'Y', C);
            note_until = 0;
            ++cases;
            if (worth(c) + worth(ground->money) != w0) bad("H1 the Healer is free", "worth %ld -> %ld", w0, worth(c) + worth(ground->money));
            if (hw) leave_heal(C);
            cheat_end(C);
        }
    }
    back();
    summary("H1 the Cheats tab: the Merchant's prices, the Healer free", cases, f0);
}

// ---- R1
void wear()
{
    const int f0 = fails;
    int cases = 0;
    for (const auto& g : recs) {
        party::Character& c = bare();
        c.rec[0x12B] = 0xFF;                  // (every class may)
        rules::add_item(c, g.data());
        c.items[0][0x36] = 0;
        // (the Berserker: cursed, it never comes off - its fury stays)
        if (d->facts.berserk_fx && items::passive(c.items[0]) == items::Passive::Effect && c.items[0][0x3D] == d->facts.berserk_fx) continue;
        rules::recalc(c, *names, d->facts);
        static party::Character before;
        before = c;
        screen = Screen::Items;
        view_from = Screen::Game;
        ready_item(0, C);
        note_until = 0;
        if (!c.items[0][0x34]) continue;
        ready_item(0, C);
        note_until = 0;
        rules::recalc(c, *names, d->facts);
        ++cases;
        for (int o = 0; o < party::kRecordSize; ++o)
            if (c.rec[o] != before.rec[o] && !(o >= 0x12D && o <= 0x13B) && !(o >= 0x1E && o < 0x1E + 84)) {
                char nm[48];
                names->name(items::Item{g.data()}, nm, sizeof nm);
                bad("R1 worn and put away: the record as before", "%s: byte %03X %d -> %d", nm, o, before.rec[o], c.rec[o]);
                break;
            }
        if (c.n_affects != before.n_affects) bad("R1 worn and put away: the effects as before", "%d -> %d", before.n_affects, c.n_affects);
    }
    back();
    screen = Screen::Game;
    summary("R1 items worn and put away leave the character as before", cases, f0);
}

// ---- R2
void rest_check()
{
    const int f0 = fails;
    int cases = 0;
    if (!load_magic()) {
        expect(P, "R2 the magic tables", false);
        return;
    }
    for (int days : {1, 2, 3}) {
        back();
        for (int i = 0; i < pt->count; ++i) {
            party::Character& c = pt->m[i];
            c.rec[0x195] = party::Okay;
            c.rec[0x1A4] = 1;
            c.n_affects = 0;
            for (int k = 0; k < magic::kListSize; ++k) c.rec[magic::kListAt + k] &= 0x7F;   // (nothing to learn)
        }
        const int hp0 = pt->m[0].hp(), max0 = pt->m[0].hp_max();
        rest = RestRun{};
        rest.left = days * 1440;
        screen = Screen::Rest;
        rest.running = true;
        magic::begin(rest.r);
        camp_heal_steps = 0;
        cheat = Cheat::Sanctuary;
        for (int t = 0; t < 200000 && rest.running && screen == Screen::Rest; ++t) {
            g_now += 400;
            rest_tick(g_now, C);
        }
        cheat = Cheat::None;
        ++cases;
        const int want = std::min(max0, hp0 + days);
        if (pt->m[0].hp() != want) bad("R2 a point of healing a day", "%d days: HP %d -> %d, %d", days, hp0, pt->m[0].hp(), want);
    }
    // An effect of N minutes: there before, gone after (the clock's 5-minute steps)
    for (int mins : {10, 60, 125}) {
        back();
        party::Character& c = pt->m[0];
        c.n_affects = 0;
        const uint8_t type = d->prof->cures.blinded ? d->prof->cures.blinded : 0x20;
        spells::add_affect(c, type, mins, 0, false);
        rest = RestRun{};
        rest.left = mins - 5;
        screen = Screen::Rest;
        rest.running = true;
        magic::begin(rest.r);
        cheat = Cheat::Sanctuary;
        for (int t = 0; t < 200000 && rest.running; ++t) {
            g_now += 400;
            rest_tick(g_now, C);
        }
        ++cases;
        if (!c.has_affect(type)) bad("R2 an effect lasts its minutes", "%d minutes: gone after %d", mins, mins - 5);
        rest = RestRun{};
        rest.left = 10;
        screen = Screen::Rest;
        rest.running = true;
        magic::begin(rest.r);
        for (int t = 0; t < 200000 && rest.running; ++t) {
            g_now += 400;
            rest_tick(g_now, C);
        }
        cheat = Cheat::None;
        ++cases;
        if (c.has_affect(type)) bad("R2 an effect ends after its minutes", "%d minutes: still there after %d", mins, mins + 5);
    }
    back();
    screen = Screen::Game;
    summary("R2 rest: a point a day, effects end on the clock", cases, f0);
}

void run()
{
    if (!monkey_boot() || !pt || !pt->count || !ground) {
        expect(P, "the game booted", false);
        return;
    }
    keep();
    collect();
    printf("  economy: %zu item records, family %d\n", recs.size(), (int)fam());
    if (getenv("ECODUMP")) {
        for (int t : {0x0C, 0x1C, 0x1E, 0x49}) {
            uint8_t it[63] = {};
            it[0x2E] = static_cast<uint8_t>(t);
            char nm[48];
            names->name(items::Item{it}, nm, sizeof nm, true);
            printf("  type %02X: %s (slot %d)\n", t, nm, names->type(t).slot);
        }
        uint8_t b[24] = {};
        if (program_ds(static_cast<uint16_t>(strtol(getenv("ECODUMP"), nullptr, 16)), b, sizeof b)) {
            printf("  DS %s:", getenv("ECODUMP"));
            for (uint8_t x : b) printf(" %02X", x);
            printf("\n");
        }
    }
    const char* only = getenv("ONLY");
    auto want = [&](const char* p) { return !only || strstr(only, p); };
    if (want("weight")) weight();
    if (want("armour")) armour();
    if (want("sell")) sell();
    if (want("id")) identify();
    if (want("buy")) buy_check();
    if (want("share") || want("take")) share();
    if (want("appraise")) appraise();
    if (want("cheats")) cheats();
    if (want("wear")) wear();
    if (want("rest")) rest_check();
}

} // namespace beco

void behave_economy() { beco::run(); }
