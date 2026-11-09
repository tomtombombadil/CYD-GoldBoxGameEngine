// The computer's bandaging and Quick Actions (behave rules4: ONLY= bandorder,
// quickacts; part of behave_round4.inc, namespace r4).
//
// bandorder - the games' own order (round 12, 2026-11-09; from the originals'
// code): the computer bandages a dying friend in its move-and-attack step,
// after the morale test, items, the pending spell, Turn Undead and spells,
// with moves left and a foe to go for - Curse GAME.OVR 0xC7EC +0x3D (the AI
// calls it at 0xBC69), Secret 0xDC94, Gateway 0xDEC1, Treasures 0xB575,
// Darkness 0xA71B, Death Knights 0xAEE7 +0x3C, Champions 0xD270 +0x39, the
// Dark Queen DQK.EXE 0x2E94D +0x3B; PoolRad's computer never bandages (its
// bandage routine, GAME.OVR 0xA806, has no caller but the menu's Bandage
// 0xA7CD and the dying checks 0xA11E / 0xA6D0). So a Quick cleric next to an
// undead foe turns it and leaves the dying friend; next to a living foe it
// bandages (PoolRad: it attacks).
//
// quickacts - Quick Actions (Tom, 2026-11-09; features.h CYD_QUICK_ACTIONS),
// checked against what Tom asked for: each column unticked stops its
// action (Melee - no blow at a foe next to it; Missile - no shot; Move - no
// step; Turn Undead - no turn; Guard - no guard, the turn ends; Items - no
// item; Spells - no spell), Bandage First ticked bandages at the turn's
// start in every game (before Turn Undead), the defaults (all but Bandage
// First) and the switch off are the game's own, party NPCs ignore the grid,
// and QUICK.TXT keeps the switches by name.

std::vector<std::pair<combat::TestAct, int>> qa_acts;     // fighter 0's actions: (act, target)
void qa_hook(const combat::Battle&, int i, combat::TestAct a, int t, int)
{
    if (i == 0) qa_acts.push_back({a, t});
}
bool qa_did(combat::TestAct a)
{
    for (auto& x : qa_acts)
        if (x.first == a) return true;
    return false;
}

// A scene: party member 0 a Quick level-5 cleric (or `cls`) at (10, 10), a
// friend (member 1) dying or not beside it, the foe B at `dist` squares
// east (an undead of type `undead`, 0 none); member 0's switches `mask`
// (-1: the switch off); one ai_step. What it did: qa_acts; *bandaged
struct QaOut {
    bool bandaged = false, turned = false, attacked = false, missile = false, stepped = false, guarded = false;
    bool cast = false, item = false;
};
QaOut qa_scene(int mask, int dist, bool dying, int undead, int cls = 0, bool thrown = false, bool npc = false)
{
    QaOut o;
    if (!fight()) return o;
    combat::Battle& b = fg->b;
    mx::put(0, 0, 10, 10, 60);
    mx::put(1, 0, 9, 10, 60);
    mx::put(B, 1, 10 + dist, 10, 60);
    for (int k = B + 1; k < b.n; ++k) b.f[k].size = 0;
    for (int k = 2; k < b.party_size; ++k) b.f[k].size = 0;
    combat::occupancy(b);
    levels(0, {{cls, 5}});
    combat::Fighter& f = b.f[0];
    f.rec[0x15] = f.rec[0x14] = 16;
    f.quick = true;
    f.spell = 0;
    f.can_cast = false;
    f.fleeing = false;
    f.moves = 12;
    if (npc) f.rec[0xF7] = 0x80 | 50;
    party::Character& ch = pt->m[f.member];
    ch.n_items = 0;
    if (thrown) {
        // A thrown weapon only (no blow with it), its reach past the foe -
        // a type from the game's own ITEMS file
        int ty = -1;
        for (int pass = 0; pass < 2 && ty < 0; ++pass)
            for (int k = 1; k < 256 && ty < 0; ++k) {
                const items::TypeInfo& ti = names->type(k);
                if (ti.slot == items::kSlotWeapon && (ti.flags & 0x18) == 0x18 && !(ti.flags & 0x81) && ti.range > dist + 1 &&
                    (pass || (ti.classes & f.rec[0x12B])))
                    ty = k;
            }
        // (none the fighter's class may use - Gateway's fighter: the weapon's
        // classes given to it)
        if (ty >= 0 && !(names->type(ty).classes & f.rec[0x12B]))
            f.rec[0x12B] = static_cast<uint8_t>(f.rec[0x12B] | names->type(ty).classes);
        if (getenv("QADEBUG")) {
            printf("  qa thrown type %d (classes byte %02X)\n", ty, static_cast<int>(f.rec[0x12B]));
            for (int k = 1; k < 256; ++k) {
                const items::TypeInfo& ti = names->type(k);
                if (ti.slot == items::kSlotWeapon && (ti.flags & 0x10))
                    printf("    type %d flags %02X range %d classes %02X\n", k, ti.flags, ti.range, ti.classes);
            }
        }
        if (ty >= 0) {
            memset(ch.items[0], 0, items::kRecordSize);
            ch.items[0][0x2E] = static_cast<uint8_t>(ty);
            ch.items[0][0x34] = 1;
            ch.items[0][0x39] = 5;          // (a pile of them)
            ch.n_items = 1;
        }
    }
    f.items = ch.items;
    f.n_items = ch.n_items;
    b.f[1].rec[0x195] = dying ? party::Dying : party::Okay;
    b.f[B].rec[0xE9] = static_cast<uint8_t>(undead);
    b.undead_loaded = 2;
    fg->cur = 0;
    fg->ai_checked = false;
    fg->ai_diced = false;
    fg->flee_told = false;
    fg->use_item = -1;
    fg->pages = fg->at = 0;
    fg->ptext_used = 0;
    const uint8_t keep_mask = qa_mask[0];
    const bool keep_on = play::opts.quick_acts;
    play::opts.quick_acts = mask >= 0;
    qa_mask[0] = static_cast<uint8_t>(mask >= 0 ? mask : 0);
    qa_acts.clear();
    auto keep_hook = combat::g_act_hook;
    combat::g_act_hook = qa_hook;
    ai_step(C);
    combat::g_act_hook = keep_hook;
    if (getenv("QADEBUG")) {
        int ammo = -1;
        printf("  qa scene mask %02X dist %d: acts %zu, weapon %d, reach %d, n_items %d, attacks %d/%d, 12B %02X\n", mask, dist,
               qa_acts.size(), names ? combat::weapon(f, *names) : -2, combat::missile(b, f, &ammo), f.n_items, f.attacks[0],
               f.attacks[1], static_cast<int>(f.rec[0x12B]));
    }
    qa_mask[0] = keep_mask;
    play::opts.quick_acts = keep_on;
    o.bandaged = dying && b.f[1].rec[0x195] != party::Dying;
    o.turned = qa_did(combat::TestAct::TurnUndead);
    for (auto& x : qa_acts)
        if (x.first == combat::TestAct::Attack) {
            o.attacked = true;
            o.missile = o.missile || !combat::adjacent(b, 0, x.second);
        }
    o.stepped = qa_did(combat::TestAct::Step);
    o.guarded = qa_did(combat::TestAct::Guard);
    o.cast = qa_did(combat::TestAct::Cast);
    o.item = qa_did(combat::TestAct::UseItem);
    b.f[B].rec[0xE9] = 0;
    b.f[1].rec[0x195] = party::Okay;
    fg->pages = fg->at = 0;
    fg->ptext_used = 0;
    fg->st = FSt::Turn;
    return o;
}

void ai_bandage_order()
{
    const int g = game();
    if (!fight()) return;
    if (fg->b.party_size < 2) {
        expect(P, "bandaging (two party members)", false);
        return;
    }
    const bool turns = fg->t.turn_known;
    const QaOut u = turns ? qa_scene(-1, 1, true, 1) : QaOut{};      // (a skeleton next to it)
    const QaOut n = qa_scene(-1, 1, true, 0);                        // (a living foe next to it)
    if (g == 1) {
        expect(P, "PoolRad: the computer never bandages (GAME.OVR 0xA806's callers: the menu 0xA7CD, the dying checks) - "
                  "a Quick cleric next to a foe attacks, next to an undead turns it",
               !n.bandaged && n.attacked && u.turned && !u.bandaged, "living foe: bandaged %d attacked %d; undead: turned %d bandaged %d",
               n.bandaged, n.attacked, u.turned, u.bandaged);
        return;
    }
    expect(P, "the computer bandages in its move step, before the blow (Curse 0xC7EC, Secret 0xDC94, Gateway 0xDEC1, "
              "Treasures 0xB575, Darkness 0xA71B, Death Knights 0xAEE7, Champions 0xD270, the Dark Queen 0x2E94D)",
           n.bandaged && !n.attacked, "game %d: bandaged %d attacked %d", g, n.bandaged, n.attacked);
    if (turns)
        expect(P, "... after Turn Undead: a Quick cleric next to an undead turns it, the dying friend left",
               u.turned && !u.bandaged, "game %d: turned %d bandaged %d", g, u.turned, u.bandaged);
}

// QUICK.TXT: the switches by name - a round trip through the engine's own
// file functions (names with spaces and odd characters, more than 8 names,
// a missing file, a name twice, case and trailing spaces)
void quick_file()
{
    char path[96];
    qa_path(path, sizeof path, "QUICK.TXT");
    plat::remove(path);
    const uint8_t keep = qa_mask[0];
    // (a missing file: the defaults)
    qa_load();
    bool ok_missing = true;
    for (int i = 0; i < pt->count; ++i) ok_missing = ok_missing && (!qa_member(i) || qa_mask[i] == quickacts::kDefault);
    // Twelve names through the text functions
    std::string text;
    const char* names12[12] = {"ALIAS",       "DRAGONBAIT", "AKABAR BEL AKASH", "o'Brien-Smith", "X",         "   PADDED   ",
                               "Tab\tIn",     "NINE",       "TEN TEN TEN",      "ELEVEN!?#",     "TWELFTH ONE", "zz"};
    std::vector<char> out(4096);
    for (int k = 0; k < 12; ++k) {
        const size_t n = quickacts::set(text.data(), text.size(), names12[k], static_cast<uint8_t>(k * 21 + 1), out.data(), out.size());
        text.assign(out.data(), n);
    }
    // (one set again: replaced, not added)
    const size_t n2 = quickacts::set(text.data(), text.size(), "dragonbait  ", 0x5A, out.data(), out.size());
    text.assign(out.data(), n2);
    int bad = 0, lines = 0;
    for (char ch : text) lines += ch == '\n';
    for (int k = 0; k < 12; ++k) {
        const uint8_t want = k == 1 ? 0x5A : static_cast<uint8_t>(k * 21 + 1);
        if (quickacts::find(text.data(), text.size(), names12[k]) != want) ++bad;
    }
    if (quickacts::find(text.data(), text.size(), "NOBODY") != quickacts::kDefault) ++bad;
    if (quickacts::find(text.data(), text.size(), "alias") != 1) ++bad;          // (case aside)
    expect(P, "Quick Actions' file: 12 names (spaces, a tab, punctuation, case, trailing spaces) set and found, one set "
              "twice is one line, an unknown name the defaults",
           bad == 0 && lines == 13, "%d wrong, %d lines (want 13: the comment + 12)", bad, lines);
    // The engine's own write / read: member 0's switches into the card's file and back
    qa_load();
    if (pt->count && qa_member(0)) {
        qa_mask[0] = 0x3C;
        qa_write(0);
        qa_mask[0] = 0;
        qa_load();
        size_t n;
        char* t = qa_read(&n);
        char nm[24];
        pt->m[0].name(nm, sizeof nm);
        const bool found = t && quickacts::find(t, n, nm) == 0x3C;
        free(t);
        expect(P, "Quick Actions' file: written by name on the card and read back at a fight's start (QUICK.TXT)",
               ok_missing && qa_mask[0] == 0x3C && found, "missing-file defaults %d, read back %02X, in the file %d", ok_missing,
               qa_mask[0], found);
    }
    plat::remove(path);
    qa_mask[0] = keep;
}

void quick_actions()
{
    using namespace quickacts;
    const int g = game();
    if (!fight()) return;
    if (fg->b.party_size < 2) {
        expect(P, "Quick Actions (two party members)", false);
        return;
    }
    const bool turns = fg->t.turn_known;
    const int D = kDefault;
    // Melee: a fighter (class 2) next to a living foe
    const QaOut m_on = qa_scene(D, 1, false, 0, 2), m_off = qa_scene(D & ~kMelee, 1, false, 0, 2);
    const QaOut m_off_ng = qa_scene(D & ~kMelee & ~kGuard, 1, false, 0, 2);
    expect(P, "Quick Actions, Melee: ticked a blow at the foe next to it; unticked no blow (it guards), Guard unticked too: "
              "the turn just ends",
           m_on.attacked && !m_on.missile && !m_off.attacked && m_off.guarded && !m_off_ng.attacked && !m_off_ng.guarded &&
               !m_off_ng.stepped,
           "ticked attacked %d; unticked attacked %d guarded %d; + Guard unticked attacked %d guarded %d stepped %d", m_on.attacked,
           m_off.attacked, m_off.guarded, m_off_ng.attacked, m_off_ng.guarded, m_off_ng.stepped);
    // Missile: a thrown weapon, the foe 3 squares off
    const QaOut s_on = qa_scene(D, 3, false, 0, 2, true), s_off = qa_scene(D & ~kMissile, 3, false, 0, 2, true);
    const QaOut s_off_nm = qa_scene(D & ~kMissile & ~kMove, 3, false, 0, 2, true);
    expect(P, "Quick Actions, Missile: ticked a shot at the foe 3 squares off; unticked no shot (it steps toward it), Move "
              "unticked too: it holds its square (guards)",
           s_on.attacked && s_on.missile && !s_off.attacked && s_off.stepped && !s_off_nm.attacked && !s_off_nm.stepped &&
               s_off_nm.guarded,
           "ticked shot %d; unticked shot %d stepped %d; + Move unticked shot %d stepped %d guarded %d", s_on.missile, s_off.attacked,
           s_off.stepped, s_off_nm.attacked, s_off_nm.stepped, s_off_nm.guarded);
    // Move: a fighter, the foe 4 squares off, nothing to throw
    const QaOut v_on = qa_scene(D, 4, false, 0, 2), v_off = qa_scene(D & ~kMove, 4, false, 0, 2);
    const QaOut v_mel = qa_scene(D & ~kMelee, 4, false, 0, 2);
    expect(P, "Quick Actions, Move: ticked it steps toward the foe; unticked it holds its square (guards); Melee unticked "
              "with nothing to throw: no step either",
           v_on.stepped && !v_off.stepped && v_off.guarded && !v_mel.stepped, "ticked stepped %d; unticked stepped %d guarded %d; "
           "no melee stepped %d", v_on.stepped, v_off.stepped, v_off.guarded, v_mel.stepped);
    // Turn Undead: a cleric next to a skeleton
    if (turns) {
        const QaOut t_on = qa_scene(D, 1, false, 1), t_off = qa_scene(D & ~kTurn, 1, false, 1);
        expect(P, "Quick Actions, Turn Undead: ticked a Quick cleric turns the undead next to it; unticked it doesn't (it "
                  "fights)",
               t_on.turned && !t_off.turned && t_off.attacked, "ticked turned %d; unticked turned %d attacked %d", t_on.turned,
               t_off.turned, t_off.attacked);
    }
    // Bandage First: before Turn Undead, before the blow - every game
    const QaOut b_l = qa_scene(D | kBandage, 1, true, 0), b_u = turns ? qa_scene(D | kBandage, 1, true, 1) : QaOut{};
    const QaOut b_def = qa_scene(D, 1, true, 0), b_off = qa_scene(-1, 1, true, 0);
    expect(P, "Quick Actions, Bandage First ticked: the dying friend bandaged at the turn's start - before the blow, before "
              "Turn Undead (every game, PoolRad too)",
           b_l.bandaged && !b_l.attacked && (!turns || (b_u.bandaged && !b_u.turned)), "game %d: living foe bandaged %d attacked %d; "
           "undead bandaged %d turned %d", g, b_l.bandaged, b_l.attacked, b_u.bandaged, b_u.turned);
    expect(P, "Quick Actions, Bandage First unticked (the start): the game's own order - as the switch off",
           b_def.bandaged == b_off.bandaged && b_def.attacked == b_off.attacked, "default bandaged %d attacked %d; off bandaged %d "
           "attacked %d", b_def.bandaged, b_def.attacked, b_off.bandaged, b_off.attacked);
    // Items / Spells unticked: none (a Quick character of the player's own
    // never casts in the games' Quick - the column has nothing to stop there)
    const QaOut i_off = qa_scene(D & ~kItems & ~kSpells, 1, false, 0);
    expect(P, "Quick Actions, Items / Spells unticked: no item used, no spell cast", !i_off.item && !i_off.cast,
           "item %d cast %d", i_off.item, i_off.cast);
    // The switch off, and party NPCs: the grid not followed
    const QaOut o_off = qa_scene(-1, 1, false, 0, 2), o_npc = qa_scene(0, 1, false, 0, 2, false, true);
    const QaOut o_none = qa_scene(0, 1, false, 0, 2);
    expect(P, "Quick Actions: the switch off - the grid not followed; a party NPC acts as its game makes it; everything "
              "unticked - nothing done",
           o_off.attacked && o_npc.attacked && !o_none.attacked && !o_none.stepped && !o_none.guarded,
           "off attacked %d; NPC attacked %d; none attacked %d stepped %d guarded %d", o_off.attacked, o_npc.attacked,
           o_none.attacked, o_none.stepped, o_none.guarded);
    quick_file();
}

// QASHOT=dir: the canvas as dir/qa<game>_<tag>.ppm (scratch only - real game screens)
void qa_shot(const char* tag)
{
    const char* dir = getenv("QASHOT");
    if (!dir) return;
    char path[300];
    snprintf(path, sizeof path, "%s/qa%d_%s.ppm", dir, game(), tag);
    FILE* f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6 320 200 255\n");
    for (int y = 0; y < 200; ++y)
        for (int x = 0; x < 320; ++x) {
            const pic::Rgb& c = pic::kEga[C.get(x, y) & 15];
            fputc(c.r, f), fputc(c.g, f), fputc(c.b, f);
        }
    fclose(f);
}

// The Quick Actions page itself (quickpage), in a fight and outside one:
// it opens over the game screen and the game waits (no tick moves the
// fight, the movement / fight keys are off); the cursor keys move its
// cursor (Up / Down through "?", the characters and Exit, round; Left /
// Right through the columns, round) and Select = a tap at nav_point (a box
// ticks / unticks - the file written; the "?" opens the help page, any key
// closes it; Exit closes the page); a tap lights its target as it touches
// (the cursor moves there) and acts on release; a heading moves the cursor
// to its column without ticking; closing puts the game screen back byte for
// byte (and a fight's colours)
void quick_page_one(const char* where)
{
    std::vector<uint8_t> before(static_cast<size_t>(C.stride()) * C.h);
    memcpy(before.data(), C.px, before.size());
    const int pal = fight_palette_on;
    const int st = fg ? static_cast<int>(fg->st) : -1, cur = fg ? fg->cur : -1, round = fg ? fg->b.round : -1;
    char path[96];
    qa_path(path, sizeof path, "QUICK.TXT");
    plat::remove(path);
    int bad = 0;
    char first[200] = "";
    auto fail = [&](const char* w) {
        ++bad;
        if (!*first) snprintf(first, sizeof first, "%s (row %d col %d help %d)", w, qp.row, qp.col, qp.help);
    };
    if (!quick_open(C)) {
        expect(P, (std::string("the Quick Actions page opens - ") + where).c_str(), false, "not ready");
        return;
    }
    qa_shot(where);
    if (!quick_page() || walking() || fight_pad() || qp.row != 0 || qp.col != 0 || qp.n < 1) fail("open: page up, keys off, cursor 0,0");
    if (fight_colours() != 0) fail("a fight's colours off while it's up");
    for (int k = 0; k < 5; ++k) tick(g_now += 1000, C);
    if (fg && (static_cast<int>(fg->st) != st || fg->cur != cur || fg->b.round != round)) fail("the fight moved on while it was up");
    if (act(Act::Forward, C) && !quick_page()) fail("a movement key closed it");
    auto select = [&]() {
        int x, y, y0, y1;
        if (!nav_point(&x, &y)) return fail("no nav_point");
        tap_highlight(x, y, C, &y0, &y1);
        tap(x, y, C);
        tap_highlight_end(C);
    };
    // Down, Right, Select: member 1's Missile
    nav(Nav::Down, C);
    nav(Nav::Right, C);
    const int n = qp.n;
    if (qp.row != (n > 1 ? 1 : n) || qp.col != (n > 1 ? 1 : 0)) fail("Down / Right");
    if (n > 1) {
        const uint8_t m0 = qa_mask[qp.member[1]];
        select();
        if (qa_mask[qp.member[1]] != (m0 ^ quickacts::kMissile)) fail("Select didn't tick the box under the cursor");
        size_t len;
        char* t = qa_read(&len);
        char nm[24];
        pt->m[qp.member[1]].name(nm, sizeof nm);
        if (!t || quickacts::find(t, len, nm) != qa_mask[qp.member[1]]) fail("the file not written");
        free(t);
        // Left round to the last column
        nav(Nav::Left, C);
        nav(Nav::Left, C);
        if (qp.col != quickacts::kCols - 1) fail("Left round");
    }
    // A tap on row 0's Itm box: lit (cursor there) as it touches, ticked on release
    {
        const int x = kQaGridX + 3 * kQaColW + 12, y = qa_row_y(0) + 3;
        int y0, y1;
        const uint8_t m0 = qa_mask[qp.member[0]];
        if (!tap_highlight(x, y, C, &y0, &y1) || qp.row != 0 || qp.col != 3) fail("the tap's highlight");
        if (qa_mask[qp.member[0]] != m0) fail("ticked before the release");
        tap(x, y, C);
        tap_highlight_end(C);
        if (qa_mask[qp.member[0]] != (m0 ^ quickacts::kItems)) fail("a tap on a box didn't tick it");
    }
    // A tap on Mov's heading: the cursor's column, nothing ticked
    {
        const uint8_t m0 = qa_mask[qp.member[0]];
        const int x = kQaGridX + 5 * kQaColW + 4, y = 3 * 8 + 3;
        int y0, y1;
        tap_highlight(x, y, C, &y0, &y1);
        tap(x, y, C);
        tap_highlight_end(C);
        if (qp.col != 5 || qa_mask[qp.member[0]] != m0) fail("a heading's tap");
    }
    // Up to the "?" (from row 0), Select: the help page; a key closes it
    while (qp.row > -1) nav(Nav::Up, C);
    select();
    if (!qp.help) fail("the ? didn't open the help");
    qa_shot((std::string(where) + "_help").c_str());
    nav(Nav::Down, C);
    if (qp.help || !quick_page()) fail("a key didn't close the help");
    // Up from the "?": Exit (round); Select closes the page
    nav(Nav::Up, C);
    if (qp.row != qp.n) fail("Up from the ? isn't Exit");
    select();
    if (quick_page()) fail("Exit didn't close it");
    if (memcmp(before.data(), C.px, before.size()) != 0) fail("the game screen didn't come back as it was");
    if (fight_palette_on != pal) fail("the fight's colours didn't come back");
    if (fg && (static_cast<int>(fg->st) != st || fg->cur != cur)) fail("the fight's state changed");
    qa_shot((std::string(where) + "_after").c_str());
    plat::remove(path);
    qa_load();
    expect(P, (std::string("the Quick Actions page - ") + where + ": cursor keys, Select, taps, the ?, Exit, the screen back").c_str(),
           bad == 0, "%d wrong: %s", bad, first);
}

void quick_page_check()
{
    if (!monkey_boot()) return;
    quick_page_one("exploring");
    if (!fight()) return;
    quick_page_one("fight");
}
