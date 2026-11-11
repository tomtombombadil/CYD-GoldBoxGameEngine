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
// First and Spells) and the switch off are the game's own, Spells ticked is
// the games' Magic On (qa_spells), the party's NPCs follow their own
// switches (kComputer as they start: the game's AI, as the switch off),
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
    bool forgot = false;                // (the memorized spell gone: cast, or begun)
};
// The first memorized place the game's computer looks at (AiCast::list_from)
int qa_place() { return play::fg && play::fg->t.ai ? play::fg->t.ai->list_from : 0; }
// qa_bow: member 0 holds a sword and carries a bow with readied arrows (the
// game's own types) - or QABOWITM=<a .ITM file>: those items (a player's)
bool qa_bow = false;
int qa_sword_plus = 0;                  // (the sword's plus: +3 out-rates a plain bow by the games' rule)
void qa_bow_items(party::Character& ch, combat::Fighter& f)
{
    if (const char* path = getenv("QABOWITM")) {
        FILE* fp = fopen(path, "rb");
        ch.n_items = 0;
        while (fp && ch.n_items < party::kMaxItems && fread(ch.items[ch.n_items], 1, items::kRecordSize, fp) == items::kRecordSize)
            ++ch.n_items;
        if (fp) fclose(fp);
        if (getenv("QADEBUG"))
            for (int k = 0; k < ch.n_items; ++k) {
                const items::TypeInfo& ti = names->type(ch.items[k][0x2E]);
                printf("    item %d type %d slot %d hands %d %dd%d flags %02X range %d attacks %d classes %02X plus %d ready %d rating %d\n", k,
                       ch.items[k][0x2E], ti.slot, ti.hands, ti.dice, ti.sides, ti.flags, ti.range, ti.attacks, ti.classes,
                       static_cast<int8_t>(ch.items[k][0x32]), ch.items[k][0x34], combat::weapon_rating(*names, ch.items[k], 0));
            }
        return;
    }
    int sword = -1, bow = -1, best = 0;
    for (int k = 1; k < 256; ++k) {
        const items::TypeInfo& ti = names->type(k);
        if (ti.slot != items::kSlotWeapon || !(ti.classes & f.rec[0x12B])) continue;
        if (!(ti.flags & 0x18) && ti.hands <= 1 && ti.dice * ti.sides > best) { sword = k; best = ti.dice * ti.sides; }
        if ((ti.flags & 0x01) && ti.range > 3 && bow < 0) bow = k;
    }
    // (none its class may use - Gateway's fighter: the bow's classes given to it)
    for (int k = 1; k < 256 && bow < 0; ++k) {
        const items::TypeInfo& ti = names->type(k);
        if (ti.slot == items::kSlotWeapon && (ti.flags & 0x01) && ti.range > 3) {
            bow = k;
            f.rec[0x12B] = static_cast<uint8_t>(f.rec[0x12B] | ti.classes);
        }
    }
    memset(ch.items, 0, sizeof ch.items[0] * 3);
    ch.items[0][0x2E] = static_cast<uint8_t>(sword);
    ch.items[0][0x34] = 1;
    ch.items[0][0x32] = static_cast<uint8_t>(qa_sword_plus);
    ch.items[1][0x2E] = static_cast<uint8_t>(bow);
    ch.items[2][0x2E] = static_cast<uint8_t>(fg->b.arrow);
    ch.items[2][0x34] = 1;
    ch.items[2][0x39] = 40;
    ch.n_items = 3;
    if (getenv("QADEBUG")) {
        printf("  qa bow: sword %d bow %d arrow %d (classes %02X)\n", sword, bow, fg->b.arrow, static_cast<int>(f.rec[0x12B]));
        for (int k = 1; k < 256; ++k) {
            const items::TypeInfo& ti = names->type(k);
            if (ti.slot == items::kSlotWeapon && (ti.flags & 0x81)) printf("    launcher %d flags %02X range %d classes %02X\n", k, ti.flags, ti.range, ti.classes);
        }
    }
}
QaOut qa_scene(int mask, int dist, bool dying, int undead, int cls = 0, bool thrown = false, bool npc = false, int spell = 0)
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
    f.can_cast = spell != 0;
    // (`spell`: the only one in memory)
    uint8_t keep_list[magic::kListSize];
    for (int k = 0; k < magic::kListSize; ++k) {
        keep_list[k] = f.rec[magic::kListAt + k];
        // (in the first place the game's AI looks at - round 20: most skip place 0)
        f.rec[magic::kListAt + k] = static_cast<uint8_t>(k == qa_place() ? spell : 0);
    }
    f.fleeing = false;
    f.moves = 12;
    party::Character& ch = pt->m[f.member];
    const uint8_t keep_ctl = f.rec[0xF7], keep_pctl = ch.rec[0xF7];
    if (npc) f.rec[0xF7] = ch.rec[0xF7] = 0x80 | 50;      // (an NPC: the party's record says so too)
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
    if (qa_bow) {
        qa_bow_items(ch, f);
        rules::recalc(ch, *names, d->facts);
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
    f.rec[0xF7] = keep_ctl;
    ch.rec[0xF7] = keep_pctl;
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
    o.forgot = spell && f.rec[magic::kListAt + qa_place()] != spell;
    for (int k = 0; k < magic::kListSize; ++k) f.rec[magic::kListAt + k] = keep_list[k];
    f.spell = 0;
    f.spell_n = 0;
    f.can_cast = false;
    b.f[B].rec[0xE9] = 0;
    b.f[1].rec[0x195] = party::Okay;
    fg->pages = fg->at = 0;
    fg->ptext_used = 0;
    fg->st = FSt::Turn;
    return o;
}

// Spells (2026-11-09, Tom: "I could have sworn that Quick casted spells"):
// every game's AI spell step casts for one of the player's own characters
// only while the fight's Magic flag is on - its control byte over 0x7F
// (NPCs, monsters) or the flag (Curse GAME.OVR 0xC0C8 [0x75F4], PoolRad
// 0xB187 [0x6E30], Secret 0xD58F [0x8E14], Gateway 0xD6FD [0x8901],
// Champions 0xCBBC [0x7198], Darkness 0x9FEA [0xAEDA], Treasures 0xAE2C
// [0xA5CB], Death Knights 0xA830 [0xC4BE], DQK.EXE 0x2E15B [0xB1CB]); Alt-M
// in the fight menu or during Quick play toggles it, each fight's start
// clears it (DOSBox, Curse's bar fight: Magic Off - no spell of the party's
// used in the whole fight; poked on - PHILIPPE "Casts a Spell", the party's
// Magic Missiles and Hold Person gone from memory). So: the column ticked -
// a Quick magic-user with Magic Missile memorized casts it (on some of 40
// turns: the AI's d7 rounds against the spell's priority) and it leaves
// memory; unticked, the start (kDefault) and Quick Actions off (the game's
// start: Magic Off) - never; a party NPC (control 0x80 on) casts as it
// starts (kComputer: the original's AI, spells whatever the flag) and with
// the switch off, never with its Spells unticked (Tom, 2026-11-09: NPCs'
// switches too - our comfort)
void qa_spells()
{
    using namespace quickacts;
    // the game's missile spell: its fight table's (level + 1) / 2 missiles
    int mm = 0;
    for (int k = 0; k < d->prof->fight.n_spells && !mm; ++k)
        if (d->prof->fight.spells[k].does == combat::SpellDoes::Damage && d->prof->fight.spells[k].per == 2)
            mm = d->prof->fight.spells[k].spell;
    if (!mm) {
        expect(P, "Quick Actions, Spells: the game's Magic Missile in its fight table", false);
        return;
    }
    const int D = kDefault;
    auto tries = [&](int mask, bool npc, int* forgot) {
        int cast = 0;
        *forgot = 0;
        for (int k = 0; k < 40; ++k) {
            const QaOut o = qa_scene(mask, 3, false, 0, 5, false, npc, mm);
            cast += o.cast || o.forgot;
            *forgot += o.forgot;
        }
        return cast;
    };
    int f_on, f_def, f_off, f_npc;
    const int on = tries(D | kSpells, false, &f_on), def = tries(D, false, &f_def), off = tries(-1, false, &f_off);
    const int npc = tries(kComputer, true, &f_npc), npc_off = tries(-1, true, &f_npc);
    const int npc_no = tries(kComputer & ~kSpells, true, &f_npc);
    expect(P, "Quick Actions, Spells (the games' Magic On): ticked - a Quick magic-user casts its memorized Magic Missile "
              "and it leaves memory; unticked (the start, as each fight's Magic Off) and Quick Actions off - never; a party "
              "NPC casts as it starts (kComputer) and with the switch off (every game's spell step: control over 0x7F), "
              "never with its Spells unticked",
           on > 0 && f_on == on && def == 0 && off == 0 && npc > 0 && npc_off > 0 && npc_no == 0,
           "spell %02X of 40 turns: ticked cast %d (left memory %d), unticked %d, switch off %d, NPC start %d, NPC switch off "
           "%d, NPC Spells unticked %d", mm, on, f_on, def, off, npc, npc_off, npc_no);
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
    for (int i = 0; i < pt->count; ++i) ok_missing = ok_missing && (!qa_member(i) || qa_mask[i] == qa_start(i));
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
    // An NPC (Tom, 2026-11-09): not in the file - kComputer (spells ticked);
    // its line written - the line decides, whatever kind it is later
    if (pt->count > 1) {
        plat::remove(path);
        party::Character& ch = pt->m[1];
        const uint8_t keep_ctl = ch.rec[0xF7], keep1 = qa_mask[1];
        ch.rec[0xF7] = 0x80 | 50;
        qa_load();
        const bool start = qa_npc(1) && qa_mask[1] == quickacts::kComputer && qa_mask[0] == quickacts::kDefault &&
                           quickacts::find("", 0, "NOBODY", quickacts::kComputer) == quickacts::kComputer;
        qa_mask[1] = 0x11;
        qa_write(1);
        ch.rec[0xF7] = static_cast<uint8_t>(keep_ctl & 0x7F);
        qa_load();
        const bool kept = !qa_npc(1) && qa_mask[1] == 0x11;
        // (the tab's model: the NPC's row marked, its toggle writes)
        ch.rec[0xF7] = 0x80 | 50;
        const int rows = quick_begin();
        const bool row_npc = rows == pt->count && quick_npc(1) && !quick_npc(0) && quick_get(1) == 0x11;
        const bool tog = quick_toggle(1, 2) && quick_get(1) == (0x11 ^ quickacts::kSpells);
        qa_load();
        const bool back = qa_mask[1] == (0x11 ^ quickacts::kSpells);
        ch.rec[0xF7] = keep_ctl;
        qa_mask[1] = keep1;
        expect(P, "Quick Actions' file, an NPC: not in the file - kComputer (the game's AI, Spells ticked); its line once "
                  "written decides; the tab's rows are the whole party, the NPC's marked; a toggle written at once",
               start && kept && row_npc && tog && back, "start %d, kept %d, rows %d npc-row %d, toggle %d, read back %d", start,
               kept, rows, row_npc, tog, back);
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
    // Missile with a bow (Tom, 2026-11-11: his PoolRad fighters charged): a
    // sword held, a bow and readied arrows carried, the foe 3 squares off -
    // the bow readied and shot; next to the foe the sword
    qa_bow = true;
    const QaOut w_on = qa_scene(D, 3, false, 0, 2), w_off = qa_scene(D & ~kMissile, 3, false, 0, 2);
    const QaOut w_next = qa_scene(D, 1, false, 0, 2);
    // A +3 sword out-rates a plain bow by the games' rule (8 + 24 + 3 > 2 x
    // the bow's): the switch off - the game's own - keeps the sword and
    // steps; Missile ticked - the bow (Tom, 2026-11-11)
    qa_sword_plus = 3;
    const QaOut r_on = qa_scene(D, 3, false, 0, 2), r_off = qa_scene(-1, 3, false, 0, 2);
    qa_sword_plus = 0;
    qa_bow = false;
    expect(P, "Quick Actions, Missile with a bow and a +3 sword: ticked the bow (shot); the switch off the game's rating (no "
              "shot: it steps)",
           r_on.missile && !r_off.missile && r_off.stepped, "ticked shot %d; off shot %d stepped %d", r_on.missile, r_off.missile,
           r_off.stepped);
    expect(P, "Quick Actions, Missile with a bow: a sword held, a bow and readied arrows carried, the foe 3 squares off - "
              "ticked it readies the bow and shoots; unticked no shot; the foe next to it: a blow",
           w_on.missile && !w_off.missile && w_next.attacked && !w_next.missile, "ticked shot %d attacked %d stepped %d; "
           "unticked shot %d; next to it attacked %d shot %d", w_on.missile, w_on.attacked, w_on.stepped, w_off.missile,
           w_next.attacked, w_next.missile);
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
    // Items / Spells unticked: none
    const QaOut i_off = qa_scene(D & ~kItems & ~kSpells, 1, false, 0);
    expect(P, "Quick Actions, Items / Spells unticked: no item used, no spell cast", !i_off.item && !i_off.cast,
           "item %d cast %d", i_off.item, i_off.cast);
    qa_spells();
    // The switch off: the grid not followed; everything unticked: nothing
    const QaOut o_off = qa_scene(-1, 1, false, 0, 2), o_none = qa_scene(0, 1, false, 0, 2);
    expect(P, "Quick Actions: the switch off - the grid not followed; everything unticked - nothing done",
           o_off.attacked && !o_none.attacked && !o_none.stepped && !o_none.guarded,
           "off attacked %d; none attacked %d stepped %d guarded %d", o_off.attacked, o_none.attacked, o_none.stepped,
           o_none.guarded);
    // The party's NPCs (Tom, 2026-11-09): their switches start at kComputer -
    // the game's AI, the same actions as the switch off; Melee unticked: no
    // blow (it guards); Move unticked: no step; all unticked: nothing
    const QaOut n_def = qa_scene(kComputer, 1, false, 0, 2, false, true), n_off = qa_scene(-1, 1, false, 0, 2, false, true);
    const QaOut n_mel = qa_scene(kComputer & ~kMelee, 1, false, 0, 2, false, true);
    const QaOut n_mov = qa_scene(kComputer & ~kMove, 4, false, 0, 2, false, true);
    const QaOut n_mov_on = qa_scene(kComputer, 4, false, 0, 2, false, true);
    const QaOut n_none = qa_scene(0, 1, false, 0, 2, false, true);
    expect(P, "Quick Actions, a party NPC: as it starts (kComputer) the same as the switch off (the game's AI); Melee "
              "unticked - no blow, it guards; Move unticked - no step; all unticked - nothing done",
           n_def.attacked == n_off.attacked && n_def.stepped == n_off.stepped && n_def.guarded == n_off.guarded &&
               n_def.attacked && !n_mel.attacked && n_mel.guarded && n_mov_on.stepped && !n_mov.stepped && !n_none.attacked &&
               !n_none.stepped && !n_none.guarded,
           "start attacked %d stepped %d guarded %d, off %d %d %d; no melee attacked %d guarded %d; move stepped %d, no move "
           "%d; none %d %d %d", n_def.attacked, n_def.stepped, n_def.guarded, n_off.attacked, n_off.stepped, n_off.guarded,
           n_mel.attacked, n_mel.guarded, n_mov_on.stepped, n_mov.stepped, n_none.attacked, n_none.stepped, n_none.guarded);
    quick_file();
}

// The Quick tab's model (quickpage; the tab itself is the engine's Game
// menu, ui/viewer.cpp - drawn by LovyanGFX, its pictures from QEMU:
// tools/devkit/qemu_shots.sh), in a fight and outside one, through the
// play:: functions the tab uses: quick_ready; quick_begin's rows are the
// whole party in party order (names as the party's, NPCs marked); a toggle
// flips that one switch and writes QUICK.TXT at once (read back by name);
// a second toggle puts it back; the game screen, the fight and its colours
// are untouched (the tab draws nothing on the game canvas); typing: not ready
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
        if (!*first) snprintf(first, sizeof first, "%s", w);
    };
    if (!quick_ready()) {
        expect(P, (std::string("the Quick tab is ready - ") + where).c_str(), false, "not ready");
        return;
    }
    const int n = quick_begin();
    if (n != pt->count || n > party::kMaxParty) fail("rows: not the whole party");
    for (int r = 0; r < n; ++r) {
        char a[24], b2[24];
        pt->m[r].name(b2, sizeof b2);
        if (!quick_name(r, a, sizeof a) || strcmp(a, b2)) fail("a row's name isn't its party member's");
        if (quick_npc(r) != pt->m[r].npc()) fail("a row's NPC mark");
        if (quick_get(r) != qa_start(r)) fail("a row's switches aren't its start (no file)");
    }
    char nm[24];
    if (quick_name(n, nm, sizeof nm) || quick_toggle(n, 0) || quick_toggle(0, quickacts::kCols)) fail("a row / column past the end");
    for (int r = 0; r < n; ++r)
        for (int k = 0; k < quickacts::kCols; ++k) {
            const uint8_t m0 = quick_get(r);
            if (!quick_toggle(r, k) || quick_get(r) != (m0 ^ (1 << k))) fail("a toggle didn't flip its one switch");
            size_t len;
            char* t = qa_read(&len);
            quick_name(r, nm, sizeof nm);
            if (!t || quickacts::find(t, len, nm, qa_start(r)) != quick_get(r)) fail("the file not written at once");
            free(t);
            if (k == 3 || k == 6) continue;                 // (left flipped: read back below)
            quick_toggle(r, k);
        }
    // (the file read back: what the toggles left)
    uint8_t want[party::kMaxParty];
    for (int r = 0; r < n; ++r) want[r] = quick_get(r);
    qa_load();
    for (int r = 0; r < n; ++r)
        if (qa_mask[r] != want[r] || want[r] != (qa_start(r) ^ quickacts::kItems ^ quickacts::kTurn)) fail("read back");
    if (memcmp(before.data(), C.px, before.size()) != 0) fail("the game screen changed");
    if (fight_palette_on != pal) fail("the fight's colours changed");
    if (fg && (static_cast<int>(fg->st) != st || fg->cur != cur || fg->b.round != round)) fail("the fight's state changed");
    {
        const Input keep = input_mode;
        input_mode = Input::Text;
        if (quick_ready() || quick_begin()) fail("ready while the game asks for typing");
        input_mode = keep;
    }
    plat::remove(path);
    qa_load();
    expect(P, (std::string("the Quick tab's model - ") + where +
               ": the whole party's rows (names, NPCs marked), each toggle written at once and read back, the game "
               "screen and fight untouched, not while typing").c_str(),
           bad == 0, "%d wrong: %s", bad, first);
}

void quick_page_check()
{
    if (!monkey_boot()) return;
    quick_page_one("exploring");
    if (!fight()) return;
    quick_page_one("fight");
}
