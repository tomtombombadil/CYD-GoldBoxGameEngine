// Rules round 20 (rules4 ONLY=aicast): the computer's spell choice, from each
// original's code (the Project's claude/rules_round20_facts.md).
//  1. the by-priority games' per-target use test (Darkness, Treasures,
//     Death Knights, the Dark Queen) against the original routine RUN on the
//     same states (oracle_aicast_<game>.txt - tools/devkit/oracles/aicast.py,
//     Unicorn), and the spell rows it reads against the engine's entries;
//  2. their chooser (Darkness GAME.OVR 0x9DAD / 0x9FA9 ...): the first by
//     priority with a use, then a d100 over 4 x the fighting levels and a d5
//     up to its priority;
//  3. the d7 games' judge (Curse GAME.OVR 0xBDEC ...): a heal spell with no
//     one to heal judged as one at the enemy (then aborted - no target), an
//     area's friends asked at EVERY candidate (a failed save: no use),
//     PoolRad's any friend, byte 8 = 1 not asked; the list's places (slot 0
//     never looked at but in PoolRad / Gateway).

namespace aicast {

struct Row { int level, aim, effect, when, priority, foe; };

void use_check()
{
    const int g = game();
    const char* name = g == 4 ? "darkness" : g == 9 ? "treasures" : g == 6 ? "knights" : g == 7 ? "queen" : nullptr;
    if (!name) return;
    char path[200];
    snprintf(path, sizeof path, "%s/oracle_aicast_%s.txt", getenv("ORACLE_DIR") ? getenv("ORACLE_DIR") : "/tmp/claude-0/bt", name);
    std::ifstream in(path);
    if (!in) {
        expect(P, "aicast: the oracle file (tools/devkit/oracles/aicast.py)", false, "%s missing", path);
        return;
    }
    if (!fight()) return;
    combat::Battle& b = fg->b;
    const classes::Tables& st = m->tables;
    const int count = st.lay.spell_count;
    const int C2 = B + 1 < b.n ? B + 1 : 1;
    std::string line;
    int n = 0, bad = 0, rows = 0, rbad = 0;
    char first[200] = "", rfirst[160] = "";
    while (std::getline(in, line)) {
        if (!line.compare(0, 4, "row ")) {
            Row r{};
            int s = 0;
            if (sscanf(line.c_str(), "row %d %d %d %d %d %d %d", &s, &r.level, &r.aim, &r.effect, &r.when, &r.priority, &r.foe) != 7 ||
                s >= count)
                continue;
            const spells::Entry e = spells::entry(st, s);
            ++rows;
            if ((e.level != r.level || e.aim != r.aim || e.affect != r.effect || (e.when != 0) != (r.when != 0) ||
                 e.priority != r.priority || (e.foe == 0) != (r.foe == 0)) && !rbad++)
                snprintf(rfirst, sizeof rfirst, "spell %d: engine %d %d %d %d %d %d, original %d %d %d %d %d %d", s, e.level, e.aim,
                         e.affect, e.when, e.priority, e.foe, r.level, r.aim, r.effect, r.when, r.priority, r.foe);
            continue;
        }
        if (line.compare(0, 4, "use ")) continue;
        int s, side, hurt, person, flags, flags2, size, hd, mr, near, res;
        char te[64], ce[64];
        if (sscanf(line.c_str(), "use %d %d %d %d %d %d %d %d %d %d %63s %63s %d", &s, &side, &hurt, &person, &flags, &flags2,
                   &size, &hd, &mr, &near, te, ce, &res) != 13 || s >= count)
            continue;
        // the caster (B, side 1), the target (A), one of the target's foes next to it or far off
        mx::put(B, 1, 3, 3);
        mx::put(0, side, 10, 10);
        mx::put(C2, side ? 0 : 1, near ? 11 : 16, 10);
        for (int k = 0; k < b.n; ++k)
            if (k != B && k != 0 && k != C2) b.f[k].size = 0;
        combat::occupancy(b);
        combat::Fighter& o = b.f[0];
        o.rec[0x78] = 20;
        o.rec[0x1A4] = hurt ? 10 : 20;
        o.rec[0x11A] = person ? 7 : 0;
        o.rec[0x14B] = static_cast<uint8_t>(flags);
        o.flags2 = static_cast<uint8_t>(flags2);
        o.rec[0xDE] = static_cast<uint8_t>(size);
        o.rec[0xE5] = static_cast<uint8_t>(hd);
        o.magic_res = static_cast<uint8_t>(mr);
        auto effs = [&](int who, const char* list) {
            if (list[0] == '-') return;
            for (const char* p = list; *p;) {
                mx::add_aff(who, static_cast<int>(strtol(p, nullptr, 16)));
                const char* c = strchr(p, ',');
                if (!c) break;
                p = c + 1;
            }
        };
        effs(0, te);
        effs(B, ce);
        const int got = combat::ai_use(b, fg->t, st, B, s, 0, true);
        const int want = res < 0 ? -1 : res > 0 ? 1 : 0, have = got < 0 ? -1 : got > 0 ? 1 : 0;
        ++n;
        if (want != have && !bad++)
            snprintf(first, sizeof first, "spell 0x%X (side %d hurt %d person %d flags %d/%d size %d hd %d mr %d near %d %s %s): %d, the original %d",
                     s, side, hurt, person, flags, flags2, size, hd, mr, near, te, ce, got, res);
    }
    mx::put(C2, 1, 16, 16);
    expect(P, "aicast: the use test against the original routine run (oracle_aicast)", n > 1000 && bad == 0,
           "%s: %d of %d states differ (%s)", name, bad, n, first);
    expect(P, "aicast: the spell rows the use test reads (level, aim, effect, castable, priority, side)", rows > 50 && rbad == 0,
           "%s: %d of %d rows differ (%s)", name, rbad, rows, rfirst);
}

// A spell of this game: aim 0 (itself), an effect, castable, no use rule of
// its own, a fight spell the engine casts; priority p (-1: any) - 0: none
int self_spell(int p, int not_this = 0)
{
    const classes::Tables& st = m->tables;
    const combat::AiCast* ai = fg->t.ai;
    for (int s = 1; s <= st.lay.spell_count; ++s) {
        const spells::Entry e = spells::entry(st, s);
        if ((e.aim & 0x0F) || !e.affect || !e.when || e.foe != 0 || s == not_this || (p >= 0 && e.priority != p)) continue;
        const combat::FightSpell* fs = combat::fight_spell(d->prof->fight.spells, d->prof->fight.n_spells, s);
        if (!fs || fs->does == combat::SpellDoes::NotYet || fs->does == combat::SpellDoes::Teleport) continue;
        bool ruled = false;
        for (int k = 0; ai && k < ai->n_use; ++k)
            for (uint8_t x : ai->use[k].spells) ruled = ruled || (x && x == s);
        if (!ruled && e.priority >= 1 && e.priority <= 5) return s;
    }
    return 0;
}

int face100 = 0, face5 = 0;
int die_set(int sides) { return sides == 100 ? face100 - 1 : sides == 5 ? face5 - 1 : 0; }

void chooser_check()
{
    const combat::AiCast* ai = fg->t.ai;
    if (!ai || ai->kind != 1 || !fight()) return;
    combat::Battle& b = fg->b;
    const classes::Tables& st = m->tables;
    // two self spells of different priorities
    int lo = 0, hi = 0;
    for (int p = 1; p <= 5 && !hi; ++p)
        if (int s = self_spell(p))
            for (int q = p + 1; q <= 5 && !hi; ++q)
                if (int s2 = self_spell(q, s)) lo = s, hi = s2;
    if (!hi) {
        printf("  (aicast: no two self spells here)\n");
        return;
    }
    const int plo = spells::entry(st, lo).priority, phi = spells::entry(st, hi).priority;
    auto setup = [&](int fighter_level) {
        mx::put(B, 1, 3, 3);
        mx::put(0, 0, 10, 10);
        for (int k = 1; k < b.n; ++k)
            if (k != B) b.f[k].size = 0;
        combat::occupancy(b);
        combat::Fighter& f = b.f[B];
        for (int k = 0; k < 84; ++k) f.rec[0x1E + k] = 0;
        // (the low one first, the high one after it: the sort puts it first)
        f.rec[0x1E + ai->list_from] = static_cast<uint8_t>(lo);
        f.rec[0x1E + ai->list_from + 1] = static_cast<uint8_t>(hi);
        levels(B, {{2, fighter_level}, {5, 5}});
    };
    int bad = 0, n = 0;
    char first[160] = "";
    auto run = [&](const char* what, int d100, int d5, int want) {
        face100 = d100;
        face5 = d5;
        create::g_die_hook = die_set;
        int tg[combat::kMaxFighters], nt = 0;
        const int got = combat::choose_spell(b, fg->t, st, B, d->prof->fight.spells, d->prof->fight.n_spells, rng, tg, &nt);
        create::g_die_hook = nullptr;
        ++n;
        if (got != want && !bad++)
            snprintf(first, sizeof first, "%s: got %d, want %d (spells %d/%d priorities %d/%d)", what, got, want, lo, hi, plo, phi);
    };
    setup(0);
    run("the higher priority first", 100, 1, hi);
    setup(0);
    mx::add_aff(B, spells::entry(st, hi).affect);
    run("its effect on already: the next", 100, 1, lo);
    setup(0);
    run("the d5 over its priority: none (no other tried)", 100, phi + 1 <= 5 ? phi + 1 : 5, phi + 1 <= 5 ? 0 : hi);
    // a fighter's level 10: 40 - a d100 of 40 none, 41 casts
    setup(10);
    run("a d100 not over 4 x the fighting level: none", 40, 1, 0);
    setup(10);
    run("a d100 over it: cast", 41, 1, hi);
    expect(P, "aicast: the by-priority chooser (first with a use, d100 over 4 x the fighting levels, d5 up to the priority)",
           bad == 0, "game %d: %d of %d wrong (%s)", game(), bad, n, first);
}

// The d7 games' judge
void judge_check()
{
    const combat::AiCast* ai = fg->t.ai;
    if (!ai || ai->kind != 0 || !fight()) return;
    combat::Battle& b = fg->b;
    const classes::Tables& st = m->tables;
    int bad = 0, n = 0;
    char first[200] = "";
    auto check = [&](const char* what, bool got, bool want) {
        ++n;
        if (got != want && !bad++) snprintf(first, sizeof first, "%s: %d, want %d", what, got, want);
    };
    auto clear = [&]() {
        for (int k = 0; k < b.n; ++k) b.f[k].size = 0;
        combat::occupancy(b);
    };
    // (a) the heal spell, nobody hurt, an enemy next to it: judged a use (as
    // one at the enemy), then no target - aborted
    {
        const int heal = ai->heal[0];
        clear();
        mx::put(B, 1, 10, 10);
        mx::put(0, 0, 11, 10);
        const spells::Entry e = spells::entry(st, heal);
        create::g_die_hook = [](int) { return 0; };
        const bool use = combat::ai_judge(b, fg->t, st, B, heal, e.priority, rng);
        int tg[combat::kMaxFighters];
        const combat::FightSpell* fs = combat::fight_spell(d->prof->fight.spells, d->prof->fight.n_spells, heal);
        const int nt = fs ? combat::ai_targets(b, fg->t, st, B, *fs, heal, 1, rng, tg) : -1;
        create::g_die_hook = nullptr;
        check("a heal spell with no one hurt and an enemy in reach: judged a use", use, true);
        check("... and no target (aborted)", nt == 0, true);
        // a hurt friend next to it: a use, at the friend
        clear();
        mx::put(B, 1, 10, 10);
        mx::put(B + 1 < b.n ? B + 1 : 1, 1, 9, 10, 60);
        b.f[B + 1 < b.n ? B + 1 : 1].rec[0x1A4] = 5;
        check("a heal spell with a hurt friend next to it, no enemy: a use", combat::ai_judge(b, fg->t, st, B, heal, e.priority, rng), true);
    }
    // (b) an area spell (row byte 15): enemies 0 and 1, a friend next to
    // enemy 1 only - its failed save stops the spell whichever enemy, its
    // made save lets it go; PoolRad: any friend there stops it; byte 8 = 1:
    // friends not asked
    int area = 0;
    for (int s = 1; s <= st.lay.spell_count && !area; ++s) {
        const spells::Entry e = spells::entry(st, s);
        if (e.area && e.foe && e.priority >= 1 && e.on_save != 1) area = s;
    }
    if (area && b.party_size >= 2 && B + 1 < b.n) {
        const spells::Entry e = spells::entry(st, area);
        auto place = [&]() {
            clear();
            mx::put(B, 1, 5, 10);
            mx::put(0, 0, 7, 10);
            mx::put(1, 0, 7, 14);
            mx::put(B + 1, 1, 8, 14);
        };
        place();
        create::g_die_hook = [](int) { return 0; };     // (saves fail)
        check("an area spell: a friend near ANY candidate failing its save - no use",
              combat::ai_judge(b, fg->t, st, B, area, e.priority, rng), false);
        place();
        create::g_die_hook = [](int s) { return s - 1; };   // (saves made)
        check("... the friend's save made - a use (PoolRad: no use)", combat::ai_judge(b, fg->t, st, B, area, e.priority, rng),
              !ai->pool_area);
        create::g_die_hook = nullptr;
        place();
        b.f[B + 1].size = 0;
        combat::occupancy(b);
        check("... no friend near: a use", combat::ai_judge(b, fg->t, st, B, area, e.priority, rng), true);
    } else {
        printf("  (aicast: no area spell here)\n");
    }
    // (c) the list's places: one spell in place 0 only
    {
        int s = 0;
        for (int k = 1; k <= st.lay.spell_count && !s; ++k) {
            const spells::Entry e = spells::entry(st, k);
            const combat::FightSpell* fs = combat::fight_spell(d->prof->fight.spells, d->prof->fight.n_spells, k);
            if (e.foe == 0 && !combat::ai_heal_spell(ai, k) && e.priority >= 1 && fs && fs->does != combat::SpellDoes::NotYet &&
                fs->does != combat::SpellDoes::Teleport && (e.aim & 0x0F) == 0)
                s = k;
        }
        if (s) {
            clear();
            mx::put(B, 1, 10, 10);
            mx::put(0, 0, 14, 10);
            combat::Fighter& f = b.f[B];
            for (int k = 0; k < 84; ++k) f.rec[0x1E + k] = 0;
            f.rec[0x1E] = static_cast<uint8_t>(s);
            create::g_die_hook = [](int sides) { return sides == 7 ? 6 : 0; };     // (7 rounds: down to priority 1)
            int tg[combat::kMaxFighters], nt = 0;
            const int got = combat::choose_spell(b, fg->t, st, B, d->prof->fight.spells, d->prof->fight.n_spells, rng, tg, &nt);
            create::g_die_hook = nullptr;
            check("a spell in the list's place 0 alone (looked at only by PoolRad / Gateway)", got == s, ai->list_from == 0);
            for (int k = 0; k < 84; ++k) f.rec[0x1E + k] = 0;
        }
    }
    expect(P, "aicast: the d7 games' judge (heal fall-through and abort, every candidate's area, PoolRad's friends, place 0)",
           bad == 0 && n >= 3, "game %d: %d of %d wrong (%s)", game(), bad, n, first);
}

void run()
{
    use_check();
    chooser_check();
    judge_check();
}

} // namespace aicast
