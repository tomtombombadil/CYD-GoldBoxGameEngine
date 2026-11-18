// The Enhancements' switches, each both ways (sweep `behave toggles`,
// -DBEHAVE_TOGGLES, any game; Tom 2026-11-17: "check each toggle and verify
// its effect in each game"). Each check asks what the player sees with the
// switch on - what Tom specified (CLAUDE.md, SPEC 11) - and with it off - the
// original's way. The engine-applied switches (Game Fixes, AD&D 1E Rules,
// Coin Exchange, Flasks of Oil) are the other suites' (tools/devkit/toggles.sh);
// Max Hit Points: behave levels ONLY=maxhp; Quick Actions: qa_verify.sh; Show
// Rolls' pages: fightfuzz FXCHECK (K8).
//   E1 Keep Memorizing  the spells chosen and not learnt: kept as the camp breaks / forgotten (the games)
//   E2 Save Slot Info   Save Which shows the slot's save / the game's plain menu
//   E3 More Characters  Add Character lists the saved games' members too / the game's files only
//   E4 Icon Portrait    View Character's icon box drawn / not (where the game has no portrait there)
//   E5 Icon Gallery     the icon editor opens on the gallery / on the original editor
//   E6 Game Font        the game's text in the engine's Clear letters / in the game's own
//   E7 Auto Join Items  a taken item onto its taker's pile of the same, a full pack too / refused
//   E8 Show Rolls       a fight keeps its rolls for the page / none kept
//   E9 Camp Fix         (behave features K5: PoolRad's camp has Fix / not)

namespace btg {

using namespace play;
const char* P = "toggles";

bool boot()
{
    if (!monkey_boot()) return expect(P, "the game booted", false), false;
    for (int k = 0; k < 20 && d && !cheats_ready(); ++k) {
        play::back(C);
        g_now += 50;
        play::tick(g_now, C);
    }
    return true;
}

uint32_t canvas_sum(const pic::Canvas& c)
{
    uint32_t h = 2166136261u;
    for (int y = 0; y < 200; ++y)
        for (int x = 0; x < 320; ++x) h = (h ^ c.get(x, y)) * 16777619u;
    return h;
}

void keep_memorizing()
{
    if (!boot() || !load_magic()) return;
    party::Character& c = pt->m[0];
    bool kept[2];
    for (int on = 0; on < 2; ++on) {
        opts.keep_mem = on == 1;
        open_camp(C);
        c.rec[magic::kListAt] = static_cast<uint8_t>(0x80 | 1);
        const uint8_t was = c.rec[magic::kListAt];
        leave_camp(C);
        kept[on] = c.rec[magic::kListAt] == was;
        c.rec[magic::kListAt] = 0;
    }
    opts.keep_mem = true;
    expect(P, "E1 Keep Memorizing: a chosen spell kept as the camp breaks (on), forgotten (off - the games)", kept[1] && !kept[0],
           "on kept %d, off kept %d", (int)kept[1], (int)kept[0]);
}

void save_info()
{
    if (!boot()) return;
    bool shown[2];
    for (int on = 0; on < 2; ++on) {
        opts.save_info = on == 1;
        ask_save(C);
        shown[on] = peek_wanted();
        back_from_save(C);
    }
    opts.save_info = true;
    if (hl()) {
        expect(P, "E2 Save Slot Info: the Dark Queen's Save shows no slots' saves either way (its own save menu)", true);
        return;
    }
    expect(P, "E2 Save Slot Info: Save Which shows the slot's save (on), the game's plain menu (off)", shown[1] && !shown[0],
           "on %d off %d", (int)shown[1], (int)shown[0]);
}

void more_characters()
{
    if (!boot()) return;
    // (the party out of the way for the count: in_party leaves its members out)
    const int keep = pt->count;
    pt->count = 0;
    int n[2];
    for (int on = 0; on < 2; ++on) {
        opts.more_guys = on == 1;
        find_guys('O');
        n[on] = d->guys;
    }
    pt->count = keep;
    opts.more_guys = true;
    // The saved games' members on the card: CHRDAT<slot><n> with the game's extension
    int members = 0;
    plat::File dir = plat::open(d->save_dir, "r");
    if (dir && dir.isDirectory())
        for (plat::File f = dir.openNextFile(); f; f = dir.openNextFile()) {
            const char* nm = f.name();
            const char* sl = strrchr(nm, '/');
            if (sl) nm = sl + 1;
            if (strlen(nm) == 12 && strncasecmp(nm, "CHRDAT", 6) == 0 && strcasecmp(nm + 8, ext_member()) == 0 &&
                static_cast<long>(f.size()) == static_cast<long>(rec_file_size()))
                ++members;
        }
    if (hl()) {
        expect(P, "E3 More Characters: the Dark Queen lists its .QCH files either way (no members' files)", n[0] == n[1],
               "on %d off %d", n[1], n[0]);
        return;
    }
    expect(P, "E3 More Characters: Add Character lists the saved games' members too (on), the game's own files (off)",
           members > 0 && n[1] > n[0] && n[1] - n[0] <= members, "on %d off %d, members' files %d (NPCs and names twice not listed)",
           n[1], n[0], members);
}

void portrait()
{
    if (!boot()) return;
    if (!pt->count) return;
    const uint16_t st = vm->get(0x7F12);
    vm->set(0x7F12, 0);                     // (not the original's portrait place)
    view_from = Screen::Game;
    uint32_t sum[2];
    for (int on = 0; on < 2; ++on) {
        opts.portrait = on == 1;
        C.fill(0, 0, 320, 200, 0);
        view_box(C, pt->m[0]);
        sum[on] = canvas_sum(C);
    }
    vm->set(0x7F12, st);
    opts.portrait = true;
    C.fill(0, 0, 320, 200, 0);
    const uint32_t blank = canvas_sum(C);
    expect(P, "E4 Icon Portrait: View Character's icon box drawn (on), nothing there (off - the game's View)",
           sum[1] != blank && sum[0] == blank, "on %s, off %s", sum[1] != blank ? "drawn" : "blank", sum[0] != blank ? "drawn" : "blank");
}

void gallery()
{
    bool g[2];
    for (int on = 0; on < 2; ++on) {
        if (!boot()) return;
        opts.gallery = on == 1;
        open_camp(C);
        start_icon(C, Screen::Alter);
        g[on] = icon_gallery();
    }
    opts.gallery = true;
    if (hl()) {
        expect(P, "E5 Icon Gallery: the Dark Queen picks icons from its own list either way (no editor to open on)", !g[0] && !g[1]);
        return;
    }
    expect(P, "E5 Icon Gallery: the icon editor opens on the gallery (on), on the original editor (off)", g[1] && !g[0],
           "on %d off %d", (int)g[1], (int)g[0]);
}

void game_font()
{
    if (!boot()) return;
    uint32_t sum[2];
    const uint8_t keep = opts.font;
    for (int on = 0; on < 2; ++on) {
        Options o = opts;
        o.font = on ? 1 : 0;
        set_options(o);
        C.fill(0, 0, 320, 200, 0);
        text_line(C, "Abcdefgh", 8, 0, 0, 1);
        sum[on] = canvas_sum(C);
    }
    Options o = opts;
    o.font = keep;
    set_options(o);
    expect(P, "E6 Game Font: Clear's letters differ from the game's own (Original = the game's)", sum[0] != sum[1]);
}

void auto_join()
{
    if (!boot()) return;
    // A pile one of the party carries that Join would add to
    int who = -1, at = -1;
    for (int m = 0; m < pt->count && who < 0; ++m)
        for (int i = 0; i < pt->m[m].n_items && who < 0; ++i) {
            static party::Character t;
            t = pt->m[m];
            if (rules::pile_onto(t, pt->m[m].items[i], secret_files())) {
                who = m;
                at = i;
            }
        }
    // (none: a pile of a type Join piles - arrows, darts... - given to member 0)
    for (int t = 1; who < 0 && t < items::kTypes && pt->count && pt->m[0].n_items < party::kMaxItems; ++t) {
        uint8_t it[items::kRecordSize] = {};
        it[0x2E] = static_cast<uint8_t>(t);
        it[0x39] = 5;
        static party::Character tc;
        tc = pt->m[0];
        rules::add_item(tc, it);
        if (rules::pile_onto(tc, it, secret_files())) {
            rules::add_item(pt->m[0], it);
            who = 0;
            at = pt->m[0].n_items - 1;
        }
    }
    if (who < 0) {
        printf("  (toggles: E7 - no item type Join piles)\n");
        return;
    }
    bool taken[2];
    static party::Character keep;
    keep = pt->m[who];
    for (int on = 0; on < 2; ++on) {
        party::Character& ch = pt->m[who];
        ch = keep;
        ch.rec[0x10] = ch.rec[0x11] = 18;       // (strength: not too heavy)
        ch.rec[0x1C] = ch.rec[0x1D] = 100;
        uint8_t item[items::kRecordSize];
        memcpy(item, ch.items[at], sizeof item);
        // the pack full: copies of another thing (a type no pile shares)
        while (ch.n_items < party::kMaxItems) {
            uint8_t* f = ch.items[ch.n_items++];
            memcpy(f, item, items::kRecordSize);
            f[0x2E] = static_cast<uint8_t>(item[0x2E] ^ 0x40);
            f[0x34] = 0;                        // (not readied)
            f[0x39] = 1;
        }
        rules::recalc(ch, *names, d->facts);
        ground->n = 1;
        memcpy(ground->item[0], item, sizeof item);
        opts.auto_join = on == 1;
        pt->selected = who;
        screen = Screen::Loot;
        take_loot(0, C);
        note_until = 0;
        taken[on] = ground->n == 0;
    }
    pt->m[who] = keep;
    ground->n = 0;
    opts.auto_join = true;
    expect(P, "E7 Auto Join Items: a full pack takes an item onto its pile of the same (on), refuses it (off - the games)",
           taken[1] && !taken[0], "on taken %d, off taken %d", (int)taken[1], (int)taken[0]);
}

void show_rolls()
{
    bool kept[2];
    for (int on = 0; on < 2; ++on) {
        if (!boot()) return;
        opts.rolls = on == 1;
        if (!start_fight()) {
            expect(P, "E8 Show Rolls: a fight", false, "none started");
            return;
        }
        kept[on] = combat::g_rolls != nullptr;
    }
    opts.rolls = true;
    expect(P, "E8 Show Rolls: a fight keeps its rolls for the page (on), none (off)", kept[1] && !kept[0], "on %d off %d",
           (int)kept[1], (int)kept[0]);
}

// (a survey, not a check: the oil the game's shops sell - its item type - and
// the type the engine takes for a flask of oil, `profile items.flask`)
void oil_survey()
{
    if (!boot() || !cheat_open(1, C) || cheat != Cheat::Merchant) {
        printf("  (toggles: oil - no Merchant)\n");
        return;
    }
    int found = 0;
    for (int i = 0; i < goods_count(); ++i) {
        items::Item it{goods(i)};
        char nm[64];
        names->name(it, nm, sizeof nm, true);
        if (strcasestr(nm, "oil") || strcasestr(nm, "flask")) {
            printf("  (toggles: oil - the shops' \"%s\" is type %d; the engine's oil type %d)\n", nm, goods(i)[0x2E], names->oil());
            ++found;
        }
    }
    if (!found) printf("  (toggles: oil - no oil in the shops; the engine's oil type %d)\n", names->oil());
    cheat_end(C);
}

// X1 Thief Skills (a Game Fix, every game): a thief whose Dexterity / race
// takes a skill below 0% - 0% with the fix; the original's byte wraps it
// round (250%+). Every race byte and Dexterity 3-18 at 1st-3rd level; a game
// whose tables never go below 0 has nothing for the fix to change
void thief_fix()
{
    if (!blv::m && !boot()) return;
    if (!blv::m && !play::load_rules(blv::m)) return (void)printf("  (toggles: X1 - no rule tables)\n");
    int below = 0, bad = 0;
    for (int race = 0; race <= 8; ++race)
        for (int L = 1; L <= 3; ++L)
            for (int dex = 3; dex <= 18; ++dex) {
                party::Character a;
                blv::fresh(a, classes::Thief);
                a.rec[0x74] = static_cast<uint8_t>(race);
                a.rec[0x109 + classes::Thief] = static_cast<uint8_t>(L);
                blv::set_stat(a, 3, dex);
                party::Character b = a;
                const uint64_t keep = rules::fixes_off();
                rules::set_fixes_off(keep & ~(1ull << rules::kFixThief));
                classes::thief_skills(a, blv::m->tables);
                rules::set_fixes_off(keep | 1ull << rules::kFixThief);
                classes::thief_skills(b, blv::m->tables);
                rules::set_fixes_off(keep);
                // (below 0: the original's byte wrapped round past 200 - with no bonus a skill never is)
                for (int k = 0; k < 8; ++k)
                    if (b.rec[0xEA + k] >= 200) {
                        ++below;
                        if (a.rec[0xEA + k] != 0) ++bad;
                    }
            }
    if (!below) {
        printf("  (toggles: X1 Thief Skills - no race / Dexterity takes a skill below 0%% in this game's tables)\n");
        return;
    }
    expect(P, "X1 Thief Skills: a skill below 0% is 0% (fix on), wraps round past 200% (off - the original)", bad == 0,
           "%d skills below 0, %d wrong", below, bad);
}

// X3 NPCs' Coin Share (a Game Fix, PoolRad): an NPC's share of a treasure's
// coins - the original keeps it in a byte (over 255: the low byte), the fix
// all of it. 6000 gold, six in the party, one an NPC with one share: 1000
// taken with the fix, 1000 & 0xFF = 232 without
void npc_share()
{
    if (!pool_files()) return;
    if (!boot() || pt->count < 2) return;
    if (!start_fight()) return (void)expect(P, "X3 NPCs' Coin Share: a fight (its words)", false);
    int left[2];
    static party::Character keep;
    keep = pt->m[0];
    const uint64_t k = rules::fixes_off();
    for (int on = 0; on < 2; ++on) {
        pt->m[0].rec[0xF7] = 0x80 | 50;
        pt->m[0].rec[0xF8] = 1;
        for (int i = 0; i < pt->count; ++i) pt->m[i].rec[0x195] = party::Okay;
        ground->clear();
        ground->money[3] = 1000 * pt->count;
        rules::set_fixes_off(on ? k & ~(1ull << rules::kFixPShare) : k | 1ull << rules::kFixPShare);
        npc_share_page(C);
        left[on] = ground->money[3];
    }
    rules::set_fixes_off(k);
    pt->m[0] = keep;
    ground->clear();
    const int all = 1000 * pt->count;
    expect(P, "X3 NPCs' Coin Share: an NPC takes its whole share (fix on), the share's low byte (off - the original)",
           left[1] == all - 1000 && left[0] == all - (1000 & 0xFF), "on left %d, off left %d of %d", left[1], left[0], all);
}

// X4 Loaded Game's Area (a Game Fix, PoolRad): a game saved in a block keeps
// that block's own words (0x4C00-0x4C1F) when loaded - the fix; the original
// clears them unless the program was in that block before the load
void loaded_area()
{
    if (!pool_files()) return;
    int got[2];
    for (int on = 0; on < 2; ++on) {
        if (!boot()) return;
        const uint64_t k = rules::fixes_off();
        rules::set_fixes_off(on ? k & ~(1ull << rules::kFixPLoad) : k | 1ull << rules::kFixPLoad);
        vm->set(0x4C1E, 77);
        if (!save_game_now('J')) {
            expect(P, "X4 Loaded Game's Area: a save", false);
            rules::set_fixes_off(k);
            return;
        }
        vm->set(0x4C1E, 0);
        d->last_block = static_cast<uint8_t>(d->gs.script + 1);    // (the program in another block before the load)
        load_game('J');
        begin_adventuring();
        pump(2000);
        got[on] = vm ? vm->get(0x4C1E) : -1;
        rules::set_fixes_off(k);
    }
    expect(P, "X4 Loaded Game's Area: a load keeps the saved block's own words (fix on), clears them (off - the original)",
           got[1] == 77 && got[0] == 0, "on %d, off %d", got[1], got[0]);
}

// ---- Rule-table probes: the routine the switch sits in run with it on and
// off over a sweep of cases. Each asks (a) that it changes something in this
// game - its games mask in rules.cpp says it should - and (b) the direction
// the switch's help gives. Where the engine's other suites already test the
// exact figures (behave levels / pool / secret, the host tests), these are
// the remaining games' "it takes effect here" check.
template <typename F> void fix_both(uint64_t bit, F&& f)
{
    const uint64_t k = rules::fixes_off();
    rules::set_fixes_off(k | bit); f(false);
    rules::set_fixes_off(k & ~bit); f(true);
    rules::set_fixes_off(k);
}
template <typename F> void adnd_both(uint16_t bit, F&& f)
{
    const uint16_t k = rules::adnd_bits();
    rules::set_adnd(k & ~bit); f(false);
    rules::set_adnd(k | bit); f(true);
    rules::set_adnd(k);
}
bool rules_ready()
{
    if (blv::m) return true;
    if (!boot()) return false;
    if (!play::load_rules(blv::m) || !blv::m) return printf("  (toggles: no rule tables)\n"), false;
    return true;
}
bool lists(uint64_t fixbit, uint16_t adndbit = 0)
{
    const int g = static_cast<int>(d->prof->game);
    for (int k = 0; k < rules::kFixCount; ++k)
        if ((1ull << k) == fixbit) return rules::fix_info(k).games >> g & 1;
    for (int k = 0; k < rules::kAdndCount; ++k)
        if (rules::adnd_info(k).bit == adndbit) return rules::adnd_info(k).games >> g & 1;
    return false;
}
int die_low(int) { return 0; }
// A level of class k (the game's own class mask bit), experience enough
void train1(party::Character& a, int k)
{
    Making* m = blv::m;
    blv::put32(a.rec, 0x127, 0x3FFFFFFF);
    create::train_classes(a, m->tables, m->facts, m->dice, m->tables.u8(static_cast<uint16_t>(m->tables.lay.class_masks + k)), true);
}
// A half-elf of class id `id` (multi-classes 8-16), each of its classes 1st
void multi_char(party::Character& a, int id, int bits, int con)
{
    blv::fresh(a, 0);
    a.rec[0x74] = 4;
    if (blv::m->tables.lay.krynn) a.ext[party::kExtRace] = 6;
    a.rec[0x75] = static_cast<uint8_t>(id);
    for (int k = 0; k < 8; ++k) a.rec[0x109 + k] = static_cast<uint8_t>(bits >> k & 1);
    blv::set_stat(a, 4, con);
    classes::class_bonuses(a, blv::m->tables);
    a.rec[0x78] = a.rec[0x1A4] = 10;
}

// X5 Hit Points a Level: a Constitution 3 fighter / thief trained a level at a
// time, the dice at their lowest - with the fix every level gives 1 or more
// (and the penalty is a minus); off, as the original
void hp_level()
{
    const uint64_t bit = 1ull << rules::kFixHpLevel;
    if (!rules_ready() || !lists(bit)) return;
    Making* m = blv::m;
    int gain[2][6] = {}, below = 0;
    const uint16_t ad = rules::adnd_bits();
    rules::set_adnd(ad & ~rules::kAdndMultiHp);         // (the game's own multi-class sum - the fix is in it)
    for (int on = 0; on < 2; ++on) {
        fix_both(bit, [&](bool o) {
            if (o != (on == 1)) return;
            party::Character a;
            multi_char(a, create::FighterThief, 1 << classes::Fighter | 1 << classes::Thief, 3);
            create::g_die_hook = die_low;
            for (int L = 0; L < 6; ++L) {
                const int was = a.hp_max();
                train1(a, L % 2 ? classes::Thief : classes::Fighter);
                gain[on][L] = a.hp_max() - was;
            }
            create::g_die_hook = nullptr;
        });
    }
    rules::set_adnd(ad);
    bool differ = false, ok = true;
    for (int L = 0; L < 6; ++L) {
        if (gain[0][L] != gain[1][L]) differ = true;
        if (gain[1][L] < 1) ok = false;
        if (gain[0][L] < 1 || gain[0][L] > 100) ++below;
    }
    expect(P, "X5 Hit Points a Level: Con 3 fighter / thief, lowest dice - every level 1+ (fix on), the original's otherwise",
           differ && ok, "on %d %d %d %d %d %d, off %d %d %d %d %d %d", gain[1][0], gain[1][1], gain[1][2], gain[1][3], gain[1][4],
           gain[1][5], gain[0][0], gain[0][1], gain[0][2], gain[0][3], gain[0][4], gain[0][5]);
}

// X6 Multi-Class Hit Points (1E rule): a fighter / magic-user / thief trained
// in all three, every die at its top - on, each class's dice added and
// divided by the classes; off, the game's own sum. Changes something
void multi_hp()
{
    if (!rules_ready() || !lists(0, rules::kAdndMultiHp)) return;
    Making* m = blv::m;
    int tot[2] = {};
    adnd_both(rules::kAdndMultiHp, [&](bool on) {
        party::Character a;
        multi_char(a, create::FighterMUThief, 1 << classes::Fighter | 1 << classes::MagicUser | 1 << classes::Thief, 16);
        create::g_die_hook = [](int n) { return n - 1; };
        for (int L = 0; L < 6; ++L) train1(a, L % 3 == 0 ? classes::Fighter : L % 3 == 1 ? classes::MagicUser : classes::Thief);
        create::g_die_hook = nullptr;
        tot[on] = a.hp_max();
    });
    expect(P, "X6 Multi-Class Hit Points: a F/MU/T trained 6 times, top dice - the 1E sum differs from the game's own",
           tot[0] != tot[1], "on %d, off %d", tot[1], tot[0]);
}

// X8 Race Adjustments: a new character of each race, the dice fixed - with
// the fix the races' adjustments (dwarves Con +1 ...), off as the original
void race_adj()
{
    const uint64_t bit = 1ull << rules::kFixRaceAdj;
    if (!rules_ready() || !lists(bit)) return;
    Making* m = blv::m;
    int races[12];
    const int nr = create::races(races, 12, secret_files());
    int changed = 0;
    for (int r = 0; r < nr; ++r) {
        int st[2][6] = {};
        fix_both(bit, [&](bool on) {
            party::Character a{};
            create::g_die_hook = [](int n) { return n / 2; };
            create::begin(a, m->tables, m->facts, m->dice, races[r], 0, classes::Fighter, 0);
            create::roll(a, m->tables, m->facts, m->dice);
            create::g_die_hook = nullptr;
            for (int i = 0; i < 6; ++i) st[on][i] = a.rec[0x10 + 2 * i];
        });
        for (int i = 0; i < 6; ++i)
            if (st[0][i] != st[1][i]) { ++changed; break; }
    }
    expect(P, "X8 Race Adjustments: the races' stat adjustments at creation (fix on), none (off - the original)", changed > 0,
           "%d of %d races changed", changed, nr);
}

// X11 Thief Skills by Race: a thief of each race - its own race's row with the
// fix, the row one on (the original) without
void thief_race()
{
    const uint64_t bit = 1ull << rules::kFixThiefRace;
    if (!rules_ready() || !lists(bit)) return;
    Making* m = blv::m;
    int races[12];
    const int nr = create::races(races, 12, secret_files());
    int changed = 0;
    for (int r = 0; r < nr; ++r) {
        uint8_t sk[2][8] = {};
        party::Character a{};
        create::g_die_hook = [](int n) { return n / 2; };
        create::begin(a, m->tables, m->facts, m->dice, races[r], 0, classes::Thief, 0);
        create::g_die_hook = nullptr;
        fix_both(bit, [&](bool on) {
            classes::thief_skills(a, m->tables);
            memcpy(sk[on], a.rec + 0xEA, 8);
        });
        if (memcmp(sk[0], sk[1], 8)) ++changed;
    }
    expect(P, "X11 Thief Skills by Race: the race's own row (fix on) differs from the row one on (off - the original)",
           changed > 0, "%d races changed", changed);
}

// X12 Paladins' Hit Points (Krynn): a paladin of Constitution 18 trained -
// the fighters' bonus with the fix, none without
void pal_hp()
{
    const uint64_t bit = 1ull << rules::kFixPalHp;
    if (!rules_ready() || !lists(bit)) return;
    Making* m = blv::m;
    int hp[2] = {};
    fix_both(bit, [&](bool on) {
        party::Character a;
        blv::fresh(a, classes::Paladin);
        blv::set_stat(a, 4, 18);
        a.rec[0x78] = a.rec[0x1A4] = 10;
        create::g_die_hook = die_low;
        blv::variant(a, 0);
        for (int L = 0; L < 4; ++L) train1(a, classes::Paladin);
        create::g_die_hook = nullptr;
        hp[on] = a.hp_max();
    });
    expect(P, "X12 Paladins' Hit Points: a Con 18 paladin gains more (fix on - the fighters' bonus) than off (the original)",
           hp[1] > hp[0], "on %d, off %d", hp[1], hp[0]);
}

// X13 Saves Past 18th (the Dark Queen): a 21st-level fighter's saves - the
// last row filled with the fix; off, the original's empty rows (0: every
// save made)
void saves18()
{
    const uint64_t bit = 1ull << rules::kFixQSaves18;
    if (!rules_ready() || !lists(bit)) return;
    Making* m = blv::m;
    int sv[2] = {};
    fix_both(bit, [&](bool on) {
        party::Character a;
        blv::fresh(a, classes::Fighter);
        a.rec[0x109 + classes::Fighter] = 21;
        classes::class_bonuses(a, m->tables);
        sv[on] = a.rec[0xDF];
    });
    expect(P, "X13 Saves Past 18th: a 21st-level fighter's save is a real row's (fix on), 0 - always made (off - the original)",
           sv[1] > 0 && sv[0] == 0, "on %d, off %d", sv[1], sv[0]);
}

// X14 Wisdom 13 (the Dark Queen): multi-class clerics made with the lowest
// dice - Wisdom 13 at least with the fix (the rule fires), off the original
void wis13()
{
    const uint64_t bit = 1ull << rules::kFixQWis13;
    if (!rules_ready() || !lists(bit)) return;
    Making* m = blv::m;
    int races[12];
    const int nr = create::races(races, 12, secret_files());
    int w[2] = {99, 99};
    fix_both(bit, [&](bool on) {
        for (int r = 0; r < nr; ++r) {
            party::Character a{};
            create::g_die_hook = die_low;
            create::begin(a, m->tables, m->facts, m->dice, races[r], 0, create::ClericFighter, 0);
            create::roll(a, m->tables, m->facts, m->dice);
            create::g_die_hook = nullptr;
            if (a.rec[0x14] && a.rec[0x14] < w[on]) w[on] = a.rec[0x14];
        }
    });
    expect(P, "X14 Wisdom 13: a multi-class cleric's Wisdom 13 or more (fix on), lower (off - the original)",
           w[1] >= 13 && w[0] < 13, "least on %d, off %d", w[1], w[0]);
}

// X2 Raising the Dead (1E rule): Raise Dead in camp on a dead member - no
// Constitution lost with the rule, a point without
void raise_dead()
{
    if (!boot() || !load_magic() || pt->count < 2) return;
    const auto& mg = d->prof->magic;
    const spells::CampSpell* cs = nullptr;
    for (int k = 0; k < mg.n_camp && !cs; ++k)
        if (mg.camp[k].does == spells::Does::Raise) cs = &mg.camp[k];
    if (!cs) return (void)printf("  (toggles: X2 - no camp Raise Dead)\n");
    int con[2] = {};
    static spells::Line lines[24];
    static party::Character keep;
    keep = pt->m[1];
    adnd_both(rules::kAdndRaise, [&](bool on) {
        pt->m[1] = keep;
        party::Character& c = pt->m[1];
        c.rec[0x74] = 7;
        c.ext[party::kExtRace] = 0;
        c.rec[0x18] = c.rec[0x19] = 15;
        c.rec[0x195] = party::Dead;
        c.rec[0x78] = 0;
        pt->m[0].rec[0x109 + classes::Cleric] = 15;
        spells::cast(*pt, 0, 1, *cs, mrules->tables, d->prof->cures, d->prof->magic.facts, rng, lines, 24, 15, 0);
        con[on] = c.health() == party::Dead ? -1 : c.rec[0x19];
    });
    pt->m[1] = keep;
    expect(P, "X2 Raising the Dead: raised with Constitution kept (1E on), one point less (off - the original)",
           con[1] == 15 && con[0] == 14, "on Con %d, off Con %d (-1: not raised)", con[1], con[0]);
}

// X18 Raising the Dead at the temple (1E rule; the Cheats' Healer = the
// game's own temple, free): a dead human magic-user 4, Constitution 16, max
// 28 of 20 rolled. On: Constitution 16, max 28. Off, each original's temple:
// Constitution in use 15; the generic temples (Curse / Champions / PoolRad /
// Gateway - GAME.OVR 0x51AA / 0x5880 / 0x4B22 / 0x607E) also cut the maximum
// by (max - rolled) / (levels) = 8 / 4 -> 26; the others' hit points follow
// their own working out (Darkness-type: Constitution 15's)
void temple_raise()
{
    if (!boot() || !pt->count) return;
    int con[2] = {}, mx[2] = {}, dead[2] = {};
    static party::Character keep;
    keep = pt->m[0];
    adnd_both(rules::kAdndRaise, [&](bool on) {
        for (int k = 0; k < 20 && d && !cheats_ready(); ++k) {
            play::back(C);
            g_now += 50;
            play::tick(g_now, C);
        }
        if (!cheat_open(0, C) || cheat != Cheat::Healer) return (void)(dead[on] = 2);
        pt->m[0] = keep;
        party::Character& c = pt->m[0];
        c.rec[0x74] = 7;
        c.ext[party::kExtRace] = 0;
        for (int k = 0; k < 8; ++k) c.rec[0x109 + k] = c.rec[0x111 + k] = 0;
        c.rec[0x109 + classes::MagicUser] = 4;
        c.rec[0x18] = c.rec[0x19] = 16;
        c.rec[0x12C] = 20;
        c.rec[0x78] = 28;
        c.rec[0x1A4] = 0;
        c.rec[0x195] = party::Dead;
        pt->selected = 0;
        open_heal(C);
        if (hw) {
            heal_pick(pool_temple() ? 6 : 7, C);
            if (ask == Ask::PayCure) shop_yes_no(Ask::PayCure, 'Y', C);
            note_until = 0;
            if (hw) leave_heal(C);
        }
        cheat_end(C);
        con[on] = c.rec[0x19];
        mx[on] = c.rec[0x78];
        dead[on] = c.health() == party::Dead;
    });
    pt->m[0] = keep;
    const bool generic = d->prof->cures.temple_raise != 0;
    expect(P, "X18 Raising the Dead at the temple: Constitution kept, max kept (1E on); one point less, the original's hit "
              "point cut (off)",
           !dead[0] && !dead[1] && con[1] == 16 && mx[1] == 28 && con[0] == 15 && (!generic || mx[0] == 26),
           "on Con %d max %d, off Con %d max %d (%s temple; dead %d %d)", con[1], mx[1], con[0], mx[0],
           generic ? "generic" : "own", dead[1], dead[0]);
}

// X19 Magic Items (1E rule; PoolRad): a member's Flame Tongue (the game's own
// facts: its type and name word, +2 and 0x3D as the original's) - switched
// on mid-game it becomes AD&D's +1 sword with the flame tongue effect, the
// name's +1 word; off, the original's bytes again
void flame_tongue()
{
    if (!boot() || !pt->count || !lists(0, rules::kAdndItems)) return;
    const items::FlameFix& ff = d->prof->items.flame;
    if (!ff.type) return (void)printf("  (toggles: X19 - no Flame Tongue facts)\n");
    party::Character& c = pt->m[0];
    const int keep_n = c.n_items;
    uint8_t sw[items::kRecordSize] = {};
    sw[0x2E] = ff.type; sw[0x2F] = ff.word; sw[0x30] = ff.plus_word; sw[0x31] = ff.type; sw[0x32] = ff.plus;
    sw[0x3D] = ff.spell;
    const uint64_t keep_fix = rules::fixes_off();
    const uint16_t keep_adnd = rules::adnd_bits();
    const uint8_t keep_enh = static_cast<uint8_t>((rules::enh(rules::kEnhCoins) ? rules::kEnhCoins : 0) | (rules::enh(rules::kEnhOil) ? rules::kEnhOil : 0));
    const Options keep_o = opts;
    Options o = opts;
    int plus[2], fx[2], word[2];
    for (int on = 0; on < 2; ++on) {
        o.adnd = static_cast<uint16_t>(on ? o.adnd | rules::kAdndItems : o.adnd & ~rules::kAdndItems);
        set_options(o);
        if (on == 0 && !rules::add_item(c, sw)) return (void)printf("  (toggles: X19 - no room)\n");
        if (on == 0) set_options(o);
        const uint8_t* it = c.items[c.n_items - 1];
        plus[on] = it[0x32];
        fx[on] = it[0x3D];
        word[on] = it[0x30];
    }
    c.n_items = keep_n;
    set_options(keep_o);
    rules::set_fixes_off(keep_fix);
    rules::set_adnd(keep_adnd);
    rules::set_enh(keep_enh);
    expect(P, "X19 Magic Items: a Flame Tongue is AD&D's +1 with its flame effect (1E on), the original's +2 (off)",
           plus[1] == 1 && fx[1] == ff.effect && word[1] == ff.plus1_word && plus[0] == ff.plus && fx[0] == ff.spell,
           "on +%d 0x3D %02X word %02X; off +%d 0x3D %02X word %02X", plus[1], fx[1], word[1], plus[0], fx[0], word[0]);
}

// M6-M9 Modify Character's steps (Tom, 2026-11-18: the Dark Queen's knight
// lost his 18(8x), Constitution 17 -> 18 took 134 hit points to 96 and they
// went no higher), every game, a human fighter type (the Krynn games' knight
// too) of the 10th level, through create::modify_stat / modify_hp as the
// Modify screen calls them - the originals' rules (Pool create_facts 5,
// Curse GAME.OVR 0x22F40 / 0x23160 / 0x231FC, Champions 0x2332B, Death
// Knights 0x17F24, the Dark Queen 0x3E30B / 0x3E8EE / 0x3F0E0):
//   M6 Strength 18(80) up: 18(81) (the race's top allowing), never plain 18
//   M7 Strength 17 up: 18 - no (xx) - then 18(01)
//   M8 Constitution up with the hit points above both bounds: they stay
//      (only raised to the least); Constitution down: lowered to the most
//   M9 the hit points up: a point more, at most the most; down: no less than
//      the least
void modify_steps()
{
    if (!rules_ready()) return;
    Making* m = blv::m;
    const auto& t = m->tables;
    const bool kr = t.lay.krynn;
    int types[4] = {classes::Fighter, classes::Paladin, classes::Ranger, classes::Monk};
    int nt = kr ? 4 : 3;
    int done = 0, bad6 = 0, bad7 = 0, bad8 = 0, bad9 = 0;
    char why[160] = "";
    for (int ti = 0; ti < nt; ++ti) {
        const int k = types[ti];
        if (k >= t.classes()) continue;
        party::Character a;
        blv::fresh(a, k == classes::Monk ? classes::Monk : k);
        a.rec[0x109 + k] = 10;
        if (kr && k == classes::Monk) a.rec[0x75] = 7;
        // Strength: 18(80) up
        blv::set_stat(a, 0, 18);
        a.rec[0x1C] = a.rec[0x1D] = 80;
        create::modify_stat(a, t, m->facts, 0, 1);
        const int s1 = a.stat(0), e1 = a.str00();
        if (!(s1 == 18 && (e1 == 81 || e1 == 80))) {
            ++bad6;
            snprintf(why, sizeof why, "class %d: 18(80) up -> %d(%d)", k, s1, e1);
        }
        // 17 up: 18, then 18(01)
        blv::set_stat(a, 0, 17);
        a.rec[0x1C] = a.rec[0x1D] = 0;
        create::modify_stat(a, t, m->facts, 0, 1);
        const int s2 = a.stat(0), e2 = a.str00();
        create::modify_stat(a, t, m->facts, 0, 1);
        const int s3 = a.stat(0), e3 = a.str00();
        if (s2 == 18 && !(e2 == 0 && s3 == 18 && e3 == 1)) {
            ++bad7;
            snprintf(why, sizeof why, "class %d: 17 up -> %d(%d), then %d(%d)", k, s2, e2, s3, e3);
        }
        // Constitution 17, the hit points far above both bounds
        blv::set_stat(a, 4, 17);
        a.rec[0x78] = a.rec[0x1A4] = 250;
        const int lo = create::hp_least(a, t, m->facts);
        create::modify_stat(a, t, m->facts, 4, 1);
        const int after_up = a.rec[0x78];
        create::modify_stat(a, t, m->facts, 4, -1);
        const int hi = create::hp_most(a, t, m->facts), after_down = a.rec[0x78];
        if (after_up != 250 || after_down != (hi < 250 ? hi : 250) || lo > hi) {
            ++bad8;
            snprintf(why, sizeof why, "class %d: Con up %d (250), down %d (most %d), least %d", k, after_up, after_down, hi, lo);
        }
        // the hit points' own steps
        a.rec[0x78] = static_cast<uint8_t>(hi);
        create::modify_hp(a, t, m->facts, 1);
        const int up = a.rec[0x78];
        a.rec[0x78] = static_cast<uint8_t>(lo);
        create::modify_hp(a, t, m->facts, -1);
        const int dn = a.rec[0x78];
        a.rec[0x78] = static_cast<uint8_t>(lo);
        create::modify_hp(a, t, m->facts, 1);
        const int up1 = a.rec[0x78];
        if (up != hi || dn != lo || (lo < hi && up1 != lo + 1) || a.rec[0x1A4] != a.rec[0x78]) {
            ++bad9;
            snprintf(why, sizeof why, "class %d: at most %d up -> %d, at least %d down -> %d, up -> %d", k, hi, up, lo, dn, up1);
        }
        printf("  (toggles: Modify class %d lv 10 Con 17: least %d most %d)\n", k, lo, hi);
        ++done;
    }
    expect(P, "M6 Modify: a fighter type's 18(80) goes up to 18(81), not plain 18", done && !bad6, "%d types; %s", done, why);
    expect(P, "M7 Modify: Strength 17 up gives 18, then 18(01)", done && !bad7, "%d types; %s", done, why);
    expect(P, "M8 Modify: Constitution up keeps hit points above the bounds; down lowers them to the most", done && !bad8,
           "%d types; %s", done, why);
    expect(P, "M9 Modify: the hit points up to the most, down to the least, a point a step", done && !bad9, "%d types; %s",
           done, why);
}

// X15 Missing Sounds (Gateway): the sounds its player plays as the miss -
// their own fitting ones with the fix, the miss (9) without
void gw_sounds()
{
    const uint64_t bit = 1ull << rules::kFixGSounds;
    if (!boot() || !lists(bit)) return;
    const sound::Clips& cl = d->prof->clips;
    int n = 0, bad = 0;
    for (int v = 0; v < 16; ++v) {
        if (!(cl.as_nine >> v & 1)) continue;
        int got[2];
        fix_both(bit, [&](bool on) { got[on] = clip_for(v); });
        ++n;
        if (got[0] != 9 || got[1] == 9 || got[1] != cl.fixed_as[v]) ++bad;
    }
    expect(P, "X15 Missing Sounds: the sounds played as the miss get their own (fix on), the miss (off - the original)",
           n > 0 && bad == 0, "%d sounds, %d wrong", n, bad);
}

// X16 Bozak Stun (Champions): the bozak's stun on a member as a fight ends -
// gone with the fix, kept (the original's list lacks it) without
void bozak()
{
    const uint64_t bit = 1ull << rules::kFixBozak;
    if (!boot() || !lists(bit) || !pt->count) return;
    bool kept[2];
    fix_both(bit, [&](bool on) {
        party::Character& c = pt->m[0];
        rules::remove_affects(c, 0x6A);
        if (c.n_affects < party::kMaxAffects) {
            memset(c.affects[c.n_affects], 0, party::kAffectSize);
            c.affects[c.n_affects][0] = 0x6A;
            c.affects[c.n_affects][1] = 10;
            ++c.n_affects;
        }
        fight_cleanup_party(combat::Won);
        kept[on] = rules::remove_affects(c, 0x6A) > 0;
    });
    expect(P, "X16 Bozak Stun: ends with the fight (fix on), stays (off - the original)", !kept[1] && kept[0], "on kept %d, off kept %d",
           (int)kept[1], (int)kept[0]);
}

// X17 New Game's Vault (Darkness, Treasures, Gateway): a new game's vault -
// emptied with the fix, the last game's left in it without
void new_vault()
{
    const uint64_t bit = 1ull << rules::kFixVault;
    if (!boot() || !lists(bit)) return;
    const auto& v = d->prof->vault;
    if (!v.slot_file) return (void)printf("  (toggles: X17 - no vault file)\n");
    char path[200];
    save_path(v.file, path, sizeof path);
    long sz[2];
    fix_both(bit, [&](bool on) {
        plat::File f = plat::open(path, "w");
        uint8_t junk[64];
        memset(junk, 0x11, sizeof junk);
        if (f) { f.write(junk, sizeof junk); f.close(); }
        vault_new_game();
        plat::File g = plat::open(path, "r");
        sz[on] = g ? static_cast<long>(g.size()) : -1;
        uint8_t first = 0;
        if (g && g.read(&first, 1) == 1 && first == 0 && sz[on] < 64) sz[on] = 0;
        if (g) g.close();
    });
    expect(P, "X17 New Game's Vault: emptied (fix on), the last game's left (off - the original)", sz[1] == 0 && sz[0] == 64,
           "on %ld, off %ld bytes", sz[1], sz[0]);
}

void run()
{
    const char* only = getenv("ONLY");
    auto want = [&](const char* s) { return !only || strstr(only, s); };
    if (want("memorize")) keep_memorizing();
    if (want("saveinfo")) save_info();
    if (want("guys")) more_characters();
    if (want("portrait")) portrait();
    if (want("gallery")) gallery();
    if (want("font")) game_font();
    if (want("join")) auto_join();
    if (want("rolls")) show_rolls();
    if (want("oil")) oil_survey();
    if (want("thief")) thief_fix();
    if (want("share")) npc_share();
    if (want("loadarea")) loaded_area();
    if (want("hplevel")) hp_level();
    if (want("multihp")) multi_hp();
    if (want("raceadj")) race_adj();
    if (want("thiefrace")) thief_race();
    if (want("palhp")) pal_hp();
    if (want("saves18")) saves18();
    if (want("wis13")) wis13();
    if (want("raise")) raise_dead();
    if (want("temple")) temple_raise();
    if (want("flame")) flame_tongue();
    if (want("modify")) modify_steps();
    if (want("gwsounds")) gw_sounds();
    if (want("bozak")) bozak();
    if (want("vault")) new_vault();
}

} // namespace btg

void behave_toggles() { btg::run(); }
