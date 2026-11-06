// Our own features against what Tom specified (sweep `behave features`,
// -DBEHAVE_FEATURES, any game; CLAUDE.md "Camp Fix and Cheats", SPEC 11 /
// 12). Each a short check of the behaviour asked for, not of our code:
//   K1 Healer: the game's temple, a cure costs nothing, its picture shown
//      (when the scripts have one)
//   K2 Merchant: the goods at their own prices (factor 16 while open, the
//      game's factor back after), Id free, the shop picture shown
//   K3 Sanctuary: a rest never broken in on (period 1, chance 100) - and
//      the same rest outside it is (the check bites)
//   K4 Trainer: the training hall's party menu, every class (word 0x7EA8
//      low byte 0xFF while open, as before after), free, Modify for anyone
//   K5 Camp Fix: Pool's camp menu has Fix with the switch on, not off
//   K6 Keep Memorizing: spells chosen and not yet learnt stay chosen when
//      the camp breaks
//   K7 Companion views: the Party view's members are the party's (names'
//      hit points, health)
// (Show Rolls - every blow's roll on a page - and the Battle view's dots
// are checked in fights: fx_check.inc K8 / K9; Max Hit Points: behave
// levels ONLY=maxhp.)

namespace bft {

using namespace play;
const char* P = "features";

long worth_all(const party::Character& ch) { return static_cast<long>(price_worth(ch)) + price_worth(ground->money); }

void to_explore()
{
    for (int k = 0; k < 20 && d && !cheats_ready(); ++k) {
        play::back(C);
        g_now += 50;
        play::tick(g_now, C);
    }
}

void healer()
{
    to_explore();
    if (!cheat_open(0, C)) {
        expect(P, "K1 the Healer opens", false, "not at the exploring menu (screen %d)", (int)screen);
        return;
    }
    expect(P, "K1 the Healer: the temple's screen", cheat == Cheat::Healer && temple && screen == Screen::Shop, "screen %d",
           (int)screen);
    if (cheat_temple_pic.pic >= 0) expect(P, "K1 the Healer: the temple's picture", cheat_pic_on);
    party::Character& c = pt->m[0];
    c.rec[0x1A4] = 1;
    if (c.rec[0x78] < 2) c.rec[0x78] = 8;
    const long w0 = worth_all(c);
    pt->selected = 0;
    open_heal(C);
    if (hw) {
        cure_at = 5;                        // (Heal)
        shop_yes_no(Ask::PayCure, 'Y', C);
        note_until = 0;
        expect(P, "K1 the Healer: a cure free", worth_all(c) == w0, "worth %ld -> %ld", w0, worth_all(c));
        if (hw) leave_heal(C);
    } else {
        expect(P, "K1 the Healer: its cures", false, "no heal list");
    }
    cheat_end(C);
}

void merchant()
{
    to_explore();
    const uint16_t f0 = vm->get(0x7F6D);
    if (!cheat_open(1, C)) {
        expect(P, "K2 the Merchant opens", false, "not at the exploring menu");
        return;
    }
    if (cheat != Cheat::Merchant) {
        expect(P, "K2 the Merchant: goods found", false, "screen %d", (int)screen);
        return;
    }
    expect(P, "K2 the Merchant: the goods' own prices (factor 16)", (vm->get(0x7F6D) & 0xFF) == 0x10, "%d", vm->get(0x7F6D) & 0xFF);
    expect(P, "K2 the Merchant: goods", goods_count() > 0, "%d", goods_count());
    if (cheat_shop_pic.pic >= 0) expect(P, "K2 the Merchant: the shop's picture", cheat_pic_on);
    // Id free: an unidentified copy of the first good in member 0's pack
    party::Character& c = pt->m[0];
    pt->selected = 0;
    if (goods_count() > 0 && c.n_items < party::kMaxItems) {
        rules::add_item(c, goods(0));
        const int i = c.n_items - 1;
        c.items[i][0x35] = 1;               // (not identified)
        const long w0 = worth_all(c);
        screen = Screen::Items;
        identify_item(i, C);
        items_yes_no(Ask::IdDeal, 'Y', C);
        note_until = 0;
        expect(P, "K2 the Merchant: Id free", worth_all(c) == w0, "worth %ld -> %ld", w0, worth_all(c));
        expect(P, "K2 the Merchant: Id identifies", i < c.n_items && c.items[i][0x35] == 0);
        screen = Screen::Shop;
    }
    cheat_end(C);
    expect(P, "K2 the Merchant: the game's price factor back", vm->get(0x7F6D) == f0, "%d, was %d", vm->get(0x7F6D), f0);
}

bool rest_broken(bool sanctuary)
{
    for (int i = 0; i < pt->count; ++i) {
        pt->m[i].rec[0x195] = party::Okay;
        pt->m[i].rec[0x1A4] = 1;
    }
    rest = RestRun{};
    rest.left = 7 * 1440;
    screen = Screen::Rest;
    rest.running = true;
    magic::begin(rest.r);
    if (d->prof->magic.rest_period) vm->set(d->prof->magic.rest_period, 1);
    if (d->prof->magic.rest_chance) vm->set(d->prof->magic.rest_chance, 100);
    const Cheat was = cheat;
    if (!sanctuary) cheat = Cheat::None;
    bool broke = false;
    for (int t = 0; t < 200000 && rest.running && screen == Screen::Rest && !broke; ++t) {
        g_now += 400;
        rest_tick(g_now, C);
        if (rest.interrupted || (!rest.running && rest.left > 0)) broke = true;   // (Pool: its rest ends at once)
    }
    cheat = was;
    rest.running = false;
    rest.interrupted = false;
    return broke;
}

void sanctuary()
{
    to_explore();
    if (!d->prof->magic.rest_period || !d->prof->magic.rest_chance) {
        expect(P, "K3 Sanctuary: the game's rest encounter words", false, "none in the profile");
        return;
    }
    if (!cheat_open(2, C)) {
        expect(P, "K3 Sanctuary opens", false);
        return;
    }
    expect(P, "K3 Sanctuary: the camp", cheat == Cheat::Sanctuary && camping, "screen %d", (int)screen);
    if (!load_magic()) {
        expect(P, "K3 Sanctuary: the magic tables (a rest's)", false);
        return;
    }
    expect(P, "K3 Sanctuary: a week's rest never broken in on", !rest_broken(true));
    expect(P, "K3 (the check bites: the same rest outside Sanctuary is broken in on)", rest_broken(false));
    screen = Screen::Camp;
    leave_camp(C);
    cheat_end(C);
}

void trainer()
{
    to_explore();
    const uint16_t w0 = vm->get(0x7EA8);
    if (!cheat_open(3, C)) {
        expect(P, "K4 the Trainer opens", false);
        return;
    }
    expect(P, "K4 the Trainer: the hall's party menu", cheat == Cheat::Trainer && screen == Screen::PartyMenu, "screen %d",
           (int)screen);
    expect(P, "K4 the Trainer: every class", (vm->get(0x7EA8) & 0xFF) == 0xFF, "%04X", vm->get(0x7EA8));
    expect(P, "K4 the Trainer: free", train_free());
    // Modify for anyone: a member the game wouldn't let be modified
    pt->selected = 0;
    party::Character& c = pt->m[0];
    for (int i = 0; i < 4; ++i) c.rec[0x127 + i] = 0xFF;     // (experience: past any new character's)
    start_modify(C);
    expect(P, "K4 the Trainer: Modify for anyone", screen == Screen::Modify && modder != nullptr, "screen %d", (int)screen);
    if (screen == Screen::Modify) play::back(C);
    // Begin: back to the exploring menu, the word as it was
    screen = Screen::PartyMenu;
    const int b = pm_line('B');
    if (b >= 0) {
        int x, y;
        pm_line_point(b, &x, &y);
        play::tap(x, y, C);
        for (int k = 0; k < 20; ++k) {
            g_now += 50;
            play::tick(g_now, C);
        }
    } else {
        cheat_end(C);
    }
    expect(P, "K4 the Trainer: Begin goes back", cheat == Cheat::None && screen == Screen::Game, "cheat %d screen %d", (int)cheat,
           (int)screen);
    expect(P, "K4 the Trainer: the hall's word back", vm->get(0x7EA8) == w0, "%04X, was %04X", vm->get(0x7EA8), w0);
}

void camp_fix()
{
    if (!pool_files()) return;
    for (int on = 0; on < 2; ++on) {
        to_explore();
        opts.camp_fix = on == 1;
        open_camp(C);
        bool fix = false;
        for (int k = 0; k < menu.count; ++k)
            if (text::key(menu, k) == 'F') fix = true;
        expect(P, on ? "K5 Camp Fix: Pool's camp has Fix" : "K5 Camp Fix off: Pool's camp as the original's (no Fix)", fix == (on == 1),
               "menu %s", menu.s);
        leave_camp(C);
    }
    opts.camp_fix = true;
}

void keep_memorizing()
{
    to_explore();
    if (!load_magic()) return;
    open_camp(C);
    party::Character& c = pt->m[0];
    c.rec[magic::kListAt] = static_cast<uint8_t>(0x80 | (c.rec[magic::kListAt] ? c.rec[magic::kListAt] : 1));
    const uint8_t was = c.rec[magic::kListAt];
    leave_camp(C);
    expect(P, "K6 Keep Memorizing: a chosen spell stays chosen as the camp breaks", c.rec[magic::kListAt] == was, "%02X -> %02X",
           was, c.rec[magic::kListAt]);
    c.rec[magic::kListAt] &= 0x7F;
}

void companion()
{
    to_explore();
    expect(P, "K7 the Party view: the party's members", panel_members() == pt->count, "%d of %d", panel_members(), pt->count);
    for (int i = 0; i < pt->count && i < panel_members(); ++i) {
        PanelMember m;
        if (!panel_member(i, m)) {
            expect(P, "K7 the Party view: a member", false, "%d", i);
            continue;
        }
        char nm[20];
        pt->m[i].name(nm, sizeof nm);
        expect(P, "K7 the Party view: name, hit points, health",
               strncmp(nm, m.name, 15) == 0 && m.hp == pt->m[i].hp() && m.hp_max == pt->m[i].hp_max() && m.health == pt->m[i].health(),
               "%s %d/%d (%d) vs %s %d/%d", m.name, m.hp, m.hp_max, m.health, nm, pt->m[i].hp(), pt->m[i].hp_max());
    }
}

void run()
{
    if (!monkey_boot()) {
        expect(P, "the game booted", false);
        return;
    }
    opts.cheats = true;
    const char* only = getenv("ONLY");
    if (!only || strstr(only, "healer")) healer();
    if (!only || strstr(only, "merchant")) merchant();
    if (!only || strstr(only, "sanctuary")) sanctuary();
    if (!only || strstr(only, "trainer")) trainer();
    if (!only || strstr(only, "campfix")) camp_fix();
    if (!only || strstr(only, "memorize")) keep_memorizing();
    if (!only || strstr(only, "companion")) companion();
}

} // namespace bft

void behave_features() { bft::run(); }
