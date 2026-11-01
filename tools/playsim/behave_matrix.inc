// Behaviour checks: the STATUS x ACTION matrix (sweep's `behave matrix`,
// -DBEHAVE_MATRIX, any game: GAME, BOOTSLOT, MAREA / MBLOCK, FIGHTAT as the
// fuzzer's - tools/playsim/README.md). Each status a game has (its effect
// numbers from the game's profile facts) is put on a plain fighter on a
// cleared field, and each trigger is tried against it: its own turn, a
// guard's blow, a free attack, being attacked (the d20 that hits, the cruel
// blow), attacking, a saving throw, its moves, the computer's choice while
// fleeing, being charmed. The expected outcomes come from the rules - AD&D
// 1st edition as the games use them, coab's facts (combat_rules_facts.md
// 1.2-1.4, 2.5, 3.2-3.3, 4.5) and the original programs (Pool's ovr10, the
// cases noted) - never from the engine's code. A status a game hasn't got
// is passed over. Dice: the d20 set, the others lowest.

namespace mx {

const char* P = "matrix";
char w[200];

int m_d20 = 0;
int m_die(int n) { return n == 20 && m_d20 ? m_d20 - 1 : 0; }
void dice(int d20 = 0)
{
    m_d20 = d20;
    create::g_die_hook = m_die;
}
void dice_off() { create::g_die_hook = nullptr; }

combat::Fighter& F(int i) { return play::fg->b.f[i]; }

// A fight from the save's LOAD MONSTER, its field cleared to floor and its
// fighters off it
bool new_fight()
{
    dice_off();
    if (!monkey_boot()) return printf("  (matrix: boot)\n"), false;
    if (!start_fight()) return printf("  (matrix: no fight, screen %d)\n", (int)play::screen), false;
    if (!play::fg || play::fg->b.n <= play::fg->b.party_size) return printf("  (matrix: no monster)\n"), false;
    combat::Battle& b = play::fg->b;
    const int floor = b.ground[b.f[0].y][b.f[0].x];
    for (int y = 0; y < combat::kH; ++y)
        for (int x = 0; x < combat::kW; ++x) b.ground[y][x] = static_cast<uint8_t>(floor);
    for (int i = 0; i < b.n; ++i) b.f[i].size = 0;
    b.to_hit_party = b.to_hit_monsters = 0;
    b.n_clouds = 0;
    combat::occupancy(b);
    return true;
}

// A plain fighter: team, HP, Hit Dice 1, saves 15 (a d20 of 15 saves), no
// effects or items, THAC0-like 40 against AC 50 (a d20 of 10 hits), one
// attack of 1d6, moves 12, its delay 5, not attacked this round
void put(int i, int team, int x, int y, int hp = 60)
{
    combat::Fighter& f = F(i);
    f.size = 1;
    f.x = x;
    f.y = y;
    f.facing = team ? 6 : 2;
    f.rec[0x197] = static_cast<uint8_t>(team);
    f.rec[0x1A4] = f.rec[0x78] = static_cast<uint8_t>(hp);
    f.rec[0x195] = party::Okay;
    f.rec[0x196] = 1;
    for (int k = 0; k < 5; ++k) f.rec[0xDF + k] = 15;
    if (f.n_aff) *f.n_aff = 0;
    f.n_items = 0;
    f.rec[0xE5] = 1;
    f.rec[0xE9] = 0;
    f.rec[0x11A] = 0;
    f.rec[0x19A] = f.rec[0x19B] = 50;
    f.rec[0x199] = 40;
    f.rec[0x19E] = 1;
    f.rec[0x1A0] = 6;
    f.rec[0x1A2] = 0;
    f.rec[0x11] = 10;
    f.rec[0x1C] = 0;
    f.rec[0x14B] = 0;
    f.rec[0xF7] = team ? 0x80 | 50 : 0;
    f.rec[0x1A5] = 12;           // (its movement)
    f.attacks[0] = 1;
    f.attacks[1] = 0;
    f.delay = 5;
    f.moves = 24;
    f.received = 0;
    f.attacked = f.guarding = f.fleeing = f.quick = false;
    f.target = -1;
    f.flags2 = f.magic_res = 0;
    f.flee_last = 8;
    f.flee_back = 0;
    combat::occupancy(play::fg->b);
}

void add_aff(int i, int type, int data = 0)
{
    combat::Fighter& f = F(i);
    if (!type || !f.n_aff || *f.n_aff >= f.max_aff) return;
    if (combat::Monster* m = f.rec.monster()) {
        if (!m->room_for_aff(*f.n_aff + 1)) return;
        f.aff = m->aff;
    }
    uint8_t* a = f.aff[(*f.n_aff)++];
    memset(a, 0, party::kAffectSize);
    a[0] = static_cast<uint8_t>(type);
    a[3] = static_cast<uint8_t>(data);
}

// ---- The statuses
enum Kind { Held, Stun, Bless, Curse, Bestow, Haste, Slow, Invisible, Blinded, ProtEvil, Faerie, Mirror, Entangle, Enlarge,
            Fleeing, Dying, Unconscious, Plain };
struct Status {
    std::string name;
    Kind kind;
    uint8_t type;           // its effect (0: a state, not an effect)
};
std::vector<Status> statuses()
{
    const combat::Facts& fx = *play::fg->b.fx;
    std::vector<Status> v;
    static const char* held_names[4] = {"snake-charmed", "paralysed", "asleep", "held"};
    for (int k = 0; k < 4; ++k)
        if (fx.held[k]) {
            char t[32];
            snprintf(t, sizeof t, "%s (%02X)", held_names[k], fx.held[k]);
            v.push_back({t, Held, fx.held[k]});
        }
    auto one = [&](const char* n, Kind k, uint8_t t) { if (t) v.push_back({n, k, t}); };
    one("stunned", Stun, fx.sp.stun);
    one("blessed", Bless, fx.bless);
    one("cursed", Curse, fx.curse);
    one("bestowed curse", Bestow, fx.bestow);
    one("hasted", Haste, fx.haste);
    one("slowed", Slow, fx.slow);
    one("invisible", Invisible, fx.invisible);
    one("blinded", Blinded, fx.blinded);
    one("protected from evil", ProtEvil, fx.prot_evil);
    one("faerie fire", Faerie, fx.faerie);
    one("mirror images", Mirror, fx.mirror);
    // (Krynn's profiles give 0x06 to both entangle and stun - open: taken as the stun there)
    if (fx.entangle != fx.sp.stun) one("entangled", Entangle, fx.entangle);
    one("enlarged", Enlarge, fx.sp.enlarge);
    v.push_back({"fleeing", Fleeing, 0});
    v.push_back({"dying", Dying, 0});
    v.push_back({"unconscious", Unconscious, 0});
    v.push_back({"none", Plain, 0});
    return v;
}
void give(int i, const Status& s)
{
    combat::Fighter& f = F(i);
    switch (s.kind) {
    case Fleeing: f.fleeing = true; break;
    case Dying: combat::damage(play::fg->b, i, f.hp() + 3); break;         // (fallen as the game fells: 3 below)
    case Unconscious: combat::damage(play::fg->b, i, f.hp()); break;      // (at 0)
    case Plain: break;
    case Mirror: add_aff(i, s.type, play::fg->b.fx->mirror_plain ? 3 : 0x30); break;
    default: add_aff(i, s.type, 0); break;
    }
    combat::occupancy(play::fg->b);
}
bool cant_act(const Status& s) { return s.kind == Held || s.kind == Stun || s.kind == Dying || s.kind == Unconscious; }
bool down(const Status& s) { return s.kind == Dying || s.kind == Unconscious; }

// The actions the engine took (the fight checker's hook)
int n_blows = 0;
void count(const combat::Battle&, int, combat::TestAct a, int, int)
{
    if (a == combat::TestAct::Attack || a == combat::TestAct::FreeAttack) ++n_blows;
}

const int A = 0;        // the party's fighter
int B = 0;              // the monster's

// ---- T1: its own turn - picked after the round's start?
bool gets_turn(const Status& s)
{
    combat::Battle& b = play::fg->b;
    put(A, 0, 10, 10);
    put(B, 1, 14, 10);
    give(B, s);
    dice(10);
    combat::start_round(b, play::rng);
    bool picked = false;
    for (int k = 0; k < 4; ++k) {
        const int c = combat::next(b, play::rng);
        if (c < 0) break;
        if (c == B) picked = true;
        b.f[c].delay = 0;
    }
    dice_off();
    return picked;
}
// ---- T2: a guard with the status - A steps next to it
int guard_blows(const Status& s)
{
    put(A, 0, 10, 10);
    put(B, 1, 12, 10);
    F(B).guarding = true;             // (it chose to guard; the status came after)
    give(B, s);
    play::fg->cur = A;
    combat::step(play::fg->b, A, 2);
    n_blows = 0;
    dice(15);
    play::guard_attacks(C, A);
    dice_off();
    return n_blows;
}
// ---- T3: a free attack by an enemy with the status as A steps away
bool free_ok(const Status& s)
{
    put(A, 0, 10, 10);
    put(B, 1, 11, 10);
    give(B, s);
    return combat::free_attack_ok(play::fg->b, play::fg->t, B, A);
}
// ---- T4: the lowest d20 that hits B (21: none); slain: the cruel blow
int hit_roll(const Status& s, bool on_attacker, bool* slain = nullptr)
{
    for (int d20 = 1; d20 <= 20; ++d20) {
        put(A, 0, 10, 10);
        put(B, 1, 11, 10);
        give(on_attacker ? A : B, s);
        dice(d20);
        play::fg->cur = A;
        const combat::Attack at = combat::attack(play::fg->b, A, B, play::names, play::rng);
        dice_off();
        if (slain) *slain = at.slain;
        bool hit = false;
        for (int k = 0; k < at.n; ++k) hit |= at.hits[k].hit;
        if (hit) return d20;
    }
    return 21;
}
// ---- T5: the lowest d20 that saves (spells, type 4)
int save_roll(const Status& s)
{
    for (int d20 = 1; d20 <= 20; ++d20) {
        put(A, 0, 10, 10);
        put(B, 1, 14, 10);
        give(B, s);
        dice(d20);
        const bool ok = combat::saving_throw(F(B), 4, 0, play::rng);
        dice_off();
        if (ok) return d20;
    }
    return 21;
}
// ---- T6: its moves at the round's start (half squares)
int moves_at_start(const Status& s)
{
    put(A, 0, 10, 10);
    put(B, 1, 20, 10);
    give(B, s);
    dice(10);
    combat::start_round(play::fg->b, play::rng);
    dice_off();
    return F(B).moves;
}

void run()
{
    if (!new_fight()) {
        expect(P, "a fight for the matrix", false);
        return;
    }
    B = play::fg->b.party_size;
    combat::g_act_hook = count;
    const auto list = statuses();
    const combat::Facts& fx = *play::fg->b.fx;
    // The baselines
    const Status none{"none", Plain, 0};
    const int hit0 = hit_roll(none, false), save0 = save_roll(none), moves0 = moves_at_start(none);
    expect(P, "baseline: a d20 of 10 hits AC 50 with 40", hit0 == 10, "lowest hit %d", hit0);
    expect(P, "baseline: a d20 of 15 saves at 15", save0 == 15, "lowest save %d", save0);
    for (const Status& s : list) {
        // T1 own turn: the held, stunned and fallen lose their turns (1.3 / 1.4)
        {
            const bool t = gets_turn(s);
            snprintf(w, sizeof w, "%s x own turn: %s", s.name.c_str(), cant_act(s) ? "no turn" : "a turn");
            expect(P, w, t == !cant_act(s), "picked %d", (int)t);
        }
        // T2 guard: no blow from the held / stunned / fallen (2.5b; Tom, 2026-11-01)
        {
            const int n = guard_blows(s);
            snprintf(w, sizeof w, "%s x guard trigger: %s", s.name.c_str(), cant_act(s) ? "no blow" : "a blow");
            expect(P, w, (n > 0) == !cant_act(s), "blows %d, guarding %d", n, (int)F(B).guarding);
            snprintf(w, sizeof w, "%s x guard trigger: the guard is over", s.name.c_str());
            expect(P, w, !F(B).guarding);
        }
        // T3 free attack: none from the held / stunned (2.5a)
        if (!down(s)) {
            const bool ok = free_ok(s);
            snprintf(w, sizeof w, "%s x leaving its side: %s", s.name.c_str(), cant_act(s) ? "no free attack" : "a free attack");
            expect(P, w, ok == !cant_act(s), "free attack %d", (int)ok);
        }
        // T4 being attacked: the held slain by one cruel blow (3.2.3); the
        // target's AC by the status - blinded 4 worse, invisible -4 to hit
        // it (Secret's program: only with the Game Fixes), faerie fire 2
        // better (Curse's / Pool's own rule; Secret's 2 worse)
        if (!down(s)) {
            bool slain = false;
            const int h = hit_roll(s, false, &slain);
            int want = hit0;
            if (s.kind == Blinded) want -= 4;
            if (s.kind == Invisible && (!fx.secret || rules::fixes())) want += 4;
            if (s.kind == Faerie) want += fx.secret ? -2 : 2;
            if (s.kind == Held) {
                snprintf(w, sizeof w, "%s x attacked: slain by one cruel blow", s.name.c_str());
                expect(P, w, slain && h == 1, "slain %d at d20 %d, down %d", (int)slain, h, (int)!F(B).up());
            } else if (s.kind != Mirror) {
                snprintf(w, sizeof w, "%s x attacked: hit from a d20 of %d", s.name.c_str(), want);
                expect(P, w, h == want && !slain, "lowest hit %d, slain %d", h, (int)slain);
            }
        }
        // T4b attacking with the status: bless +1, curse -1, bestow curse -4,
        // blinded -4 (3.3; spell_facts)
        if (!cant_act(s)) {
            const int h = hit_roll(s, true);
            int want = hit0;
            if (s.kind == Bless) want -= 1;
            if (s.kind == Curse) want += 1;
            if (s.kind == Bestow || s.kind == Blinded) want += 4;
            snprintf(w, sizeof w, "%s x attacking: hits from a d20 of %d", s.name.c_str(), want);
            expect(P, w, h == want, "lowest hit %d", h);
        }
        // T5 saving throws: bestow curse and blindness -4 (spell_facts 1.8)
        if (!down(s)) {
            const int sv = save_roll(s);
            const int want = save0 + ((s.kind == Bestow || s.kind == Blinded) ? 4 : 0);
            snprintf(w, sizeof w, "%s x saving throw: from a d20 of %d", s.name.c_str(), want);
            expect(P, w, sv == want, "lowest save %d", sv);
            if (s.kind == Bestow || s.kind == Blinded) {
                // (the original: the total a byte - below 0 it wraps round to a
                // save made, so a 2 with -4 saves; AD&D's with the Game Fixes)
                rules::set_fixes(false);
                const int so = save_roll(s);
                rules::set_fixes(true);
                snprintf(w, sizeof w, "%s x saving throw, the original's: a d20 of 2 saves (the byte wraps)", s.name.c_str());
                expect(P, w, so == 2, "lowest save %d", so);
            }
        }
        // T6 moves: haste x2, slow / 2, entangled none (1.2; spell_facts)
        if (!cant_act(s)) {
            const int mv = moves_at_start(s);
            const int want = s.kind == Haste ? moves0 * 2 : s.kind == Slow ? moves0 / 2 : s.kind == Entangle ? 0 : moves0;
            snprintf(w, sizeof w, "%s x the round's moves: %d", s.name.c_str(), want);
            expect(P, w, mv == want, "moves %d", mv);
        }
    }
    // T7 fleeing (4.5 / 4.6; coab moralFailureEscape, Pool ovr10): it runs
    // while a way is open; cornered it fights where it stands (an enemy
    // next to it); with half a square left it guards
    {
        combat::Battle& b = play::fg->b;
        put(A, 0, 10, 10);
        put(B, 1, 11, 10);
        F(B).fleeing = true;
        F(B).delay = 5;
        dice(10);
        combat::Plan p = combat::think(b, play::fg->t, B, play::rng);
        expect(P, "fleeing x its turn, a way open: it runs (a step)", p.act == combat::Act::Step, "act %d", (int)p.act);
        // walls round it but A's side: the floor gone from its other squares
        for (int dd = 0; dd < 8; ++dd) {
            const int x = 11 + geo::dx(dd), y = 10 + geo::dy(dd);
            if (x != 10 || y != 10) b.ground[y][x] = 0xFF;
        }
        uint8_t keep[combat::kH][combat::kW];
        memcpy(keep, b.ground, sizeof keep);
        // (ground 0xFF: a tile of its own - a wall: the tile table's 0xFF cost)
        put(A, 0, 10, 10);
        put(B, 1, 11, 10);
        F(B).fleeing = true;
        int blocked = 0;
        for (int dd = 0; dd < 8; ++dd) {
            bool edge;
            if (combat::step_cost(b, play::fg->t, B, dd, &edge, nullptr) == 0xFF && !edge) ++blocked;
        }
        if (blocked == 8) {
            p = combat::think(b, play::fg->t, B, play::rng);
            expect(P, "fleeing x cornered: it strikes the enemy next to it", p.act == combat::Act::Attack && p.target == A,
                   "act %d target %d moves %d", (int)p.act, p.target, F(B).moves);
        } else {
            printf("  (matrix: cornering failed - %d of 8 blocked)\n", blocked);
        }
        put(A, 0, 10, 10);
        put(B, 1, 14, 10);
        F(B).fleeing = true;
        F(B).moves = 1;
        p = combat::think(b, play::fg->t, B, play::rng);
        expect(P, "fleeing x half a square left: it guards", p.act == combat::Act::Guard, "act %d", (int)p.act);
        dice_off();
        const int floor = b.ground[10][10];
        for (int y = 0; y < combat::kH; ++y)
            for (int x = 0; x < combat::kW; ++x) b.ground[y][x] = static_cast<uint8_t>(floor);
    }
    // T8 end of the fight: experience only for the beaten - not for one
    // standing or one that fled (6.1 / 6.2: those Okay or Running give none)
    {
        combat::Battle& b = play::fg->b;
        for (int st : {party::Okay, party::Running, party::Unconscious, party::Dead}) {
            put(A, 0, 10, 10);
            put(B, 1, 14, 10);
            F(B).rec[0x13C] = 100;
            F(B).rec[0x13D] = 0;
            F(B).rec[0x13E] = 0;
            F(B).rec[0x195] = static_cast<uint8_t>(st);
            if (st != party::Okay) F(B).rec[0x196] = 0;
            const int n_keep = b.n;
            b.n = B + 1;
            const combat::Outcome o = combat::finish(b, nullptr);
            b.n = n_keep;
            const bool want = st == party::Unconscious || st == party::Dead;
            snprintf(w, sizeof w, "status %d x end of the fight: %s", st, want ? "its experience" : "no experience");
            expect(P, w, (o.exp >= 100) == want, "exp %d", o.exp);
        }
    }
    // T9 charmed (Charm Person): on the caster's side, back when it ends
    if (fx.charm && play::load_magic()) {
        const combat::FightSpell* fs = nullptr;
        for (int i = 0; i < play::d->prof->fight.n_spells && !fs; ++i)
            if (play::d->prof->fight.spells[i].does == combat::SpellDoes::Charm) fs = &play::d->prof->fight.spells[i];
        if (fs) {
            put(A, 0, 10, 10);
            put(B, 1, 12, 10);
            F(B).rec[0x11A] = 1;            // (a person: man-sized humanoid)
            F(B).rec[0xDE] = 1;
            F(B).rec[0x10] = F(B).rec[0x11] = 10;
            F(B).flags2 |= 2;                 // (Secret's family: a person by its flags 2)
            dice(1);                        // (its save failed)
            combat::SpellLine lines[8];
            const int tg[1] = {B};
            combat::cast(play::fg->b, play::mrules->tables, A, fs->spell, *fs, tg, 1, play::rng, lines, 8, 5);
            dice_off();
            expect(P, "charmed x the caster's side", F(B).team() == F(A).team(), "team %d", F(B).team());
        }
    }
    combat::g_act_hook = fc::on ? fc::on_act : nullptr;
}

} // namespace mx

void behave_matrix() { mx::run(); }
