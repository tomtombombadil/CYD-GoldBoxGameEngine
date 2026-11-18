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
                for (int k = 0; k < 8; ++k)
                    if (a.rec[0xEA + k] != b.rec[0xEA + k]) {
                        ++below;
                        if (a.rec[0xEA + k] != 0 || b.rec[0xEA + k] < 200) ++bad;
                    }
            }
    if (!below) {
        printf("  (toggles: X1 Thief Skills - no race / Dexterity takes a skill below 0%% in this game's tables)\n");
        return;
    }
    expect(P, "X1 Thief Skills: a skill below 0% is 0% (fix on), wraps round past 200% (off - the original)", bad == 0,
           "%d skills below 0, %d wrong", below, bad);
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
}

} // namespace btg

void behave_toggles() { btg::run(); }
