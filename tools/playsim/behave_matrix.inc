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
    f.driven = false;
    f.q_ac_shift = 0;
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
    // (Death Knights gives 0x06 to both entangle and Power Word Stun - its
    // spell row; effect 6's handler is the held ones' there, GAME.OVR
    // 0x1BCA9: taken as the stun)
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

// ---- Dice at their highest (the d20 20): damage that halves to something
int m_die_max(int n) { return n - 1; }

// A's weapon for a blow (a melee weapon type of the game's), its plus
uint8_t wpn[1][items::kRecordSize];
int melee_type()
{
    for (int t = 1; play::names && t < items::kTypes; ++t) {
        const items::TypeInfo& ti = play::names->type(t);
        if (ti.slot == items::kSlotWeapon && ti.range <= 1 && !(ti.flags & 0x81) && ti.sides > 0) return t;
    }
    return -1;
}
// B's hit points lost to one blow of A's (+plus weapon; -1 bare hands),
// B with the effect `type` (0 none), every die at its highest
int blow_loss(int type, int plus)
{
    put(A, 0, 10, 10);
    put(B, 1, 11, 10);
    F(A).rec[0x74] = 1;                 // (a human: AD&D's need-magic rule takes the race)
    auto* keep = F(A).items;
    const int keep_n = F(A).n_items;
    const int t = melee_type();
    if (plus >= 0 && t > 0) {
        memset(wpn, 0, sizeof wpn);
        wpn[0][0x2E] = static_cast<uint8_t>(t);
        wpn[0][0x32] = static_cast<uint8_t>(plus);
        wpn[0][0x34] = 1;
        F(A).items = wpn;
        F(A).n_items = 1;
    }
    if (type) add_aff(B, type, 0);
    create::g_die_hook = m_die_max;
    play::fg->cur = A;
    const int before = F(B).hp();
    combat::attack(play::fg->b, A, B, play::names, play::rng);
    dice_off();
    F(A).items = keep;
    F(A).n_items = keep_n;
    return before - F(B).hp();
}

// ---- T10 weapon immunities: what a +0 ... +3 weapon's blow does to one
// with each immunity the game has (coab's handlers: AffectResistWeapons -
// +0 nothing, +1 / +2 half, +3 all; AffectProtNonMagicWeapons - +0 nothing
// from a human; Pool's wight - magic or silver; Darkness's +1 / +2 / +3
// needed; the halvers below +3 / +4)
void weapons()
{
    if (melee_type() < 0 || !play::names) {
        printf("  (matrix: no melee weapon type)\n");
        return;
    }
    const combat::MonFx& m = play::fg->b.fx->mon;
    struct Imm {
        const char* name;
        uint8_t type;
        int what[4];        // per plus 0-3: 0 nothing, 1 half, 2 all
    } list[] = {
        {"resist weapons (stop normal)", m.stop_normal, {0, 1, 1, 2}},
        {"non-magic weapons no use (need magic)", m.need_magic, {0, 2, 2, 2}},
        {"silver or magic only", m.silver_only, {0, 2, 2, 2}},
        {"+1 needed", m.need_plus1, {0, 2, 2, 2}},
        {"+2 needed", m.need_plus2, {0, 0, 2, 2}},
        {"+3 needed", m.need_plus3, {0, 0, 0, 2}},
        {"half below +3", m.half_under3, {1, 1, 1, 2}},
        {"half below +4", m.half_under4, {1, 1, 1, 1}},
    };
    for (const Imm& im : list) {
        if (!im.type) continue;
        for (int plus = 0; plus <= 3; ++plus) {
            const int full = blow_loss(0, plus), got = blow_loss(im.type, plus);
            const int want = im.what[plus] == 0 ? 0 : im.what[plus] == 1 ? full / 2 : full;
            snprintf(w, sizeof w, "%s (%02X) x a +%d weapon's blow: %s", im.name, im.type, plus,
                     im.what[plus] == 0 ? "nothing" : im.what[plus] == 1 ? "half" : "all");
            expect(P, w, got == want && full > 0, "lost %d of %d", got, full);
        }
    }
}

// ---- T12 darts (Tom, 2026-11-06): AD&D 1e's rate of fire - 3 darts a
// round (the ITEMS file's 6 half attacks, every game's); one aimed throw
// rolls all three at its target; Strength (AD&D's table) to hit and
// damage for a hurled weapon - the originals give it to melee weapons
// (flags 4) only, so the dart gets it with the Game Fixes alone
int adnd_str_hit(int s, int e)
{
    if (s <= 3) return -3;
    if (s <= 5) return -2;
    if (s <= 7) return -1;
    if (s <= 16) return 0;
    if (s == 17 || (s == 18 && e <= 50)) return 1;
    if (s == 18 && e <= 99) return 2;
    return 3;                           // 18/00
}
int adnd_dex_missile(int d)
{
    // (the PHB's Dexterity table: reaction / missile adjustment)
    if (d <= 3) return -3;
    if (d == 4) return -2;
    if (d == 5) return -1;
    if (d <= 15) return 0;
    if (d <= 18) return d - 15;
    return 3;
}
int adnd_str_dmg(int s, int e)
{
    if (s <= 5) return -1;
    if (s <= 15) return 0;
    if (s <= 17) return 1;
    if (s == 18 && e == 0) return 2;
    if (s == 18 && e <= 75) return 3;
    if (s == 18 && e <= 90) return 4;
    if (s == 18 && e <= 99) return 5;
    return 6;                           // 18/00
}

// ---- Flasks of Oil (an Enhancement, Tom 2026-11-17: ordinary, not magic):
// a thrown flask's blow is fire, so a regenerating monster (the troll's
// death effect) it brings down stays down; switched off, as the originals - a
// plain blow: it gets up again (troll_up). A game without the effect has
// nothing for the switch to change
bool has_aff(int i, int type)
{
    const combat::Fighter& f = F(i);
    for (int k = 0; f.n_aff && k < *f.n_aff; ++k)
        if (f.aff[k][0] == type) return true;
    return false;
}
void oil()
{
    const combat::MonFx& m = play::fg->b.fx->mon;
    const int flask = play::names ? play::names->oil() : 0;
    if (!flask || !m.troll_death || !m.troll_up) {
        printf("  (matrix: Flasks of Oil - %s)\n", !flask ? "no flask of oil" : "no regenerating monster's effect");
        return;
    }
    static uint8_t pile[1][items::kRecordSize];
    auto* keep = F(A).items;
    const int keep_n = F(A).n_items;
    bool up[2];
    for (int on = 0; on < 2; ++on) {
        put(A, 0, 10, 10);
        put(B, 1, 12, 10, 1);
        add_aff(B, m.troll_death);
        memset(pile, 0, sizeof pile);
        pile[0][0x2E] = static_cast<uint8_t>(flask);
        pile[0][0x34] = 1;
        pile[0][0x39] = 1;
        F(A).items = pile;
        F(A).n_items = 1;
        F(A).attacks[0] = 1;
        F(A).attacks[1] = 0;
        rules::set_enh(on ? rules::kEnhAll : rules::kEnhAll & ~rules::kEnhOil);
        dice(20);
        play::fg->cur = A;
        combat::attack(play::fg->b, A, B, play::names, play::rng);
        dice_off();
        up[on] = has_aff(B, m.troll_up);
        F(A).items = keep;
        F(A).n_items = keep_n;
    }
    rules::set_enh(rules::kEnhAll);
    expect(P, "Flasks of Oil: a regenerating monster a thrown flask brings down stays down (on); gets up again (off)",
           !up[1] && up[0], "on: gets up %d; off: gets up %d", (int)up[1], (int)up[0]);
}

// ---- Held Mid-Step (a Game Fix, every game): a fighter held when it steps
// away from a foe (the foe's free attack) - the fix: no step, its turn over;
// off, the originals: the step is taken. (The hold is put on before the
// step, the foe not next to it - a blow on the held is a cruel one: the
// free attack's own hold is the same state when the step comes.)
void held_step()
{
    const combat::Facts& fx = *play::fg->b.fx;
    int held = 0;
    for (int k = 0; k < 4 && !held; ++k) held = fx.held[k];
    if (!held) {
        printf("  (matrix: Held Mid-Step - no held effect)\n");
        return;
    }
    int x_after[2];
    const uint64_t keep = rules::fixes_off();
    // (three goes - the first only settles the field: the squares an earlier
    // check left taken show as taken on its first step)
    for (int go = 0; go < 3; ++go) {
        const int on = go < 2 ? 1 : 0;
        combat::Battle& b = play::fg->b;
        for (int i = 0; i < b.n; ++i)
            if (i != A && i != B) b.f[i].size = 0;              // (the field to these two)
        for (int x = 7; x <= 13; ++x) b.ground[10][x] = b.ground[20][20];   // (no body left there by an earlier check)
        put(A, 0, 10, 10);
        put(B, 1, 12, 10);                  // (not next to it: any blow on the held is a cruel one - no step either way)
        add_aff(A, held);
        rules::set_fixes_off(on ? keep & ~(1ull << rules::kFixHeld) : keep | 1ull << rules::kFixHeld);
        dice(2);                            // (the free attack misses)
        play::fg->cur = A;
        if (getenv("HELDDBG")) {
            bool e; int bl;
            printf("  held dbg pre: moves %d up %d cost %d edge %d blk %d helpless %d\n", F(A).moves, (int)F(A).up(),
                   combat::step_cost(play::fg->b, play::fg->t, A, 6, &e, &bl), (int)e, bl, (int)combat::helpless(play::fg->b, F(A)));
        }
        play::try_step(C, 6);               // (west: away from the foe)
        dice_off();
        x_after[on] = F(A).x;
        if (getenv("HELDDBG")) printf("  held dbg: on %d x %d moves %d st %d note %d\n", on, F(A).x, F(A).moves, (int)play::fg->st, (int)(play::note_until != 0));
        play::fg->pages = 0;
    }
    rules::set_fixes_off(keep);
    expect(P, "Held Mid-Step: a held fighter stepping away stays (fix on), takes the step (off - the originals)",
           x_after[1] == 10 && x_after[0] == 9, "on x %d, off x %d (from 10)", x_after[1], x_after[0]);
}

// ---- Harm (a Game Fix, every game with the spell): its victim down to 1d4
// hit points - with fewer than the roll the fix leaves it be; off, the
// originals' sum wraps round and kills. The dice at their top (4), the
// victim at 2 hit points
void harm_fix()
{
    const combat::FightSpell* fs = nullptr;
    for (int i = 0; i < play::d->prof->fight.n_spells && !fs; ++i)
        if (play::d->prof->fight.spells[i].does == combat::SpellDoes::HarmTouch) fs = &play::d->prof->fight.spells[i];
    if (!fs) {
        printf("  (matrix: Harm - no such spell)\n");
        return;
    }
    int hp[2];
    const uint64_t keep = rules::fixes_off();
    for (int on = 0; on < 2; ++on) {
        put(A, 0, 10, 10);
        put(B, 1, 11, 10, 2);
        rules::set_fixes_off(on ? keep & ~(1ull << rules::kFixHarm) : keep | 1ull << rules::kFixHarm);
        create::g_die_hook = [](int n) { return n == 20 ? 0 : n - 1; };      // (saves failed, the 1d4 at 4)
        combat::SpellLine lines[8];
        const int tg[1] = {B};
        play::fg->b.f[A].rec[0x199] = 60;                                    // (the touch hits)
        combat::cast(play::fg->b, play::mrules->tables, A, fs->spell, *fs, tg, 1, play::rng, lines, 8, 9);
        dice_off();
        hp[on] = F(B).up() ? F(B).hp() : -99;
    }
    rules::set_fixes_off(keep);
    expect(P, "Harm: a victim with fewer hit points than the roll is left (fix on), killed - the sum wraps (off - the originals)",
           hp[1] == 2 && hp[0] != 2, "on %d, off %d (-99: down)", hp[1], hp[0]);
}

// ---- Cleric's Experience Bonus (a Game Fix): a cleric of Wisdom 15 - no
// bonus with the fix (AD&D: 16 and up), the game's +10% without where its
// own figure is lower
void cleric_xp()
{
    combat::Battle& b = play::fg->b;
    if (!b.fx || !b.fx->cleric_xp_wis || b.fx->cleric_xp_wis >= 16) {
        printf("  (matrix: Cleric's Experience Bonus - the game's own figure is AD&D's)\n");
        return;
    }
    uint32_t got[2];
    int share = 0;
    const uint64_t keep = rules::fixes_off();
    for (int on = 0; on < 2; ++on) {
        put(A, 0, 10, 10);
        F(A).rec[0x75] = 0;
        F(A).rec[0x15] = static_cast<uint8_t>(b.fx->cleric_xp_wis);
        for (int k = 0; k < 4; ++k) F(A).rec[0x127 + k] = 0;
        rules::set_fixes_off(on ? keep & ~(1ull << rules::kFixClericXp) : keep | 1ull << rules::kFixClericXp);
        share = combat::award(b, 1000, true);
        got[on] = F(A).rec[0x127] | F(A).rec[0x128] << 8;
    }
    rules::set_fixes_off(keep);
    expect(P, "Cleric's Experience Bonus: a cleric of the game's Wisdom figure gets none (fix on: 16 needed), +10% (off - the original)",
           got[1] == static_cast<uint32_t>(share) && got[0] == static_cast<uint32_t>(share + share / 10), "on %u, off %u (share %d)",
           got[1], got[0], share);
}

bool has_aff_min(int i, int type, int* minutes)
{
    const combat::Fighter& f = F(i);
    for (int k = 0; f.n_aff && k < *f.n_aff; ++k)
        if (f.aff[k][0] == type) {
            *minutes = f.aff[k][1] | f.aff[k][2] << 8;
            return true;
        }
    return false;
}
// ---- Stirge and Squid (Gateway): a stirge's hit that latches - its hold
// keeps 0xCF of blood to drink with the fix; the original's none (it lets go)
void stirge()
{
    const combat::Facts::GateFx& gw = play::fg->b.fx->gw;
    if (!gw.ways || !gw.stirge || !gw.sucking) return (void)printf("  (matrix: Stirge - none in this game)\n");
    int mins[2] = {-1, -1};
    bool held[2];
    const uint64_t keep = rules::fixes_off();
    for (int on = 0; on < 2; ++on) {
        put(B, 1, 11, 10);
        put(A, 0, 10, 10);
        add_aff(B, gw.stirge);
        rules::set_fixes_off(on ? keep & ~(1ull << rules::kFixGHolds) : keep | 1ull << rules::kFixGHolds);
        create::g_die_hook = [](int n) { return n == 20 ? 19 : 0; };
        play::fg->cur = B;
        combat::attack(play::fg->b, B, A, play::names, play::rng);
        dice_off();
        held[on] = has_aff_min(B, gw.sucking, &mins[on]);
    }
    rules::set_fixes_off(keep);
    expect(P, "Stirge and Squid: a latched stirge holds blood to drink (fix on: 0xCF), none (off - the original)",
           held[1] && held[0] && mins[1] == 0xCF && mins[0] == 0, "on %d/%d, off %d/%d", (int)held[1], mins[1], (int)held[0], mins[0]);
}
// ---- Heal Cures Feeblemind (Death Knights): Heal on one feebleminded by its
// Feeblemind - cured with the fix, kept (the original looks for another
// effect) without
void heal_feeble()
{
    const combat::Facts& fx = *play::fg->b.fx;
    if (!fx.heal_feeble2) return (void)printf("  (matrix: Heal Cures Feeblemind - not this game)\n");
    const combat::FightSpell* fs = nullptr;
    for (int i = 0; i < play::d->prof->fight.n_spells && !fs; ++i)
        if (play::d->prof->fight.spells[i].does == combat::SpellDoes::FullHeal) fs = &play::d->prof->fight.spells[i];
    if (!fs) return (void)printf("  (matrix: Heal Cures Feeblemind - no Heal)\n");
    bool kept[2];
    const uint64_t keep = rules::fixes_off();
    for (int on = 0; on < 2; ++on) {
        put(A, 0, 10, 10);
        put(B, 0, 11, 10);
        add_aff(B, fx.heal_feeble2);
        rules::set_fixes_off(on ? keep & ~(1ull << rules::kFixKHeal) : keep | 1ull << rules::kFixKHeal);
        dice(20);
        combat::SpellLine lines[8];
        const int tg[1] = {B};
        combat::cast(play::fg->b, play::mrules->tables, A, fs->spell, *fs, tg, 1, play::rng, lines, 8, 12);
        dice_off();
        kept[on] = has_aff(B, fx.heal_feeble2);
    }
    rules::set_fixes_off(keep);
    expect(P, "Heal Cures Feeblemind: Heal takes it off (fix on), leaves it (off - the original)", !kept[1] && kept[0],
           "on kept %d, off kept %d", (int)kept[1], (int)kept[0]);
}

// ---- Monster Summoning (a Game Fix, Darkness): the reserve's monster comes
// in next to the caster. A big one (two squares, the second the caster's
// own) beside it: with the fix it needs both squares free - not answered;
// off, the original tests the one square - placed there
void summon_room()
{
    if (!play::ml || play::ml->reserve_from < 0) return (void)printf("  (matrix: Monster Summoning - no reserve)\n");
    const combat::FightSpell* fs = nullptr;
    for (int i = 0; i < play::d->prof->fight.n_spells && !fs; ++i)
        if (play::d->prof->fight.spells[i].does == combat::SpellDoes::Summon) fs = &play::d->prof->fight.spells[i];
    if (!fs) return (void)printf("  (matrix: Monster Summoning - no such spell)\n");
    combat::Battle& b = play::fg->b;
    combat::Monster& m = play::ml->m[play::ml->reserve_from];
    const uint8_t keep_size = m.get(0xDE);
    const uint8_t plain = (play::vm->get(0x22) & 0xFF) ? 0x17 : 0x45;
    static uint8_t keep_ground[5][7];
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 7; ++x) keep_ground[y][x] = b.ground[8 + y][7 + x];
    int got[2][2];
    const uint64_t keep = rules::fixes_off();
    for (int on = 0; on < 2; ++on) {
        const int n0 = b.n;
        for (int i = 0; i < b.n; ++i)
            if (i != A) b.f[i].size = 0;
        put(A, 0, 10, 10);
        for (int y = 8; y <= 12; ++y)
            for (int x = 7; x <= 13; ++x) b.ground[y][x] = 0;
        b.ground[9][10] = b.ground[10][10] = plain;           // (north: one plain square, then the caster's)
        for (int k = play::ml->reserve_from; k < play::ml->n; ++k) play::ml->m[k].set(0x195, 9);
        m.set(0xDE, 2);
        combat::occupancy(b);
        rules::set_fixes_off(on ? keep & ~(1ull << rules::kFixSummon) : keep | 1ull << rules::kFixSummon);
        play::summon(A, *fs);
        got[on][0] = b.n > n0 ? b.f[b.n - 1].x : -1;
        got[on][1] = b.n > n0 ? b.f[b.n - 1].y : -1;
        while (b.n > n0) b.f[--b.n].size = 0;
        play::fg->pages = 0;
    }
    rules::set_fixes_off(keep);
    m.set(0xDE, keep_size);
    for (int y = 0; y < 5; ++y)
        for (int x = 0; x < 7; ++x) b.ground[8 + y][7 + x] = keep_ground[y][x];
    combat::occupancy(b);
    expect(P, "Monster Summoning: a two-square monster not put half on the caster (fix on); on its one free square (off - the original)",
           got[1][0] < 0 && got[0][0] == 10 && got[0][1] == 9, "on at %d,%d; off at %d,%d (-1: not answered)", got[1][0], got[1][1],
           got[0][0], got[0][1]);
}

// Bounced Bolt Length (Game Fixes, every game): on the game's own field
// tables, a bolt from (10,10) aimed at (13,10), range 7, a wall 6 squares out
// (a ground value of the game's whose move cost is 0xFF), nobody else there.
// The original (curse_finish_facts.md 2.5): it bounces with 8 more near the
// caster and hits the caster coming back; with the fix exactly its range -
// 14, 15, the turn at the wall, 15 .. 11 - nobody hit
void bolt_len()
{
    combat::Battle& b = play::fg->b;
    if (!b.tables) return (void)printf("  (matrix: Bounced Bolt - no tables)\n");
    const combat::Tables& t = *b.tables;
    static uint8_t keep_ground[combat::kH][combat::kW];
    memcpy(keep_ground, b.ground, sizeof keep_ground);
    const bool keep_in = b.indoors;
    int floor_v = -1, wall_v = -1;
    for (int v = 1; v < 256 && (floor_v < 0 || wall_v < 0); ++v) {
        b.ground[10][16] = static_cast<uint8_t>(v);
        const int c = combat::tile(b, t, 16, 10)[0];
        if (c == 1 && floor_v < 0) floor_v = v;
        if (c == 0xFF && wall_v < 0) wall_v = v;
    }
    if (floor_v < 0 || wall_v < 0) {
        memcpy(b.ground, keep_ground, sizeof keep_ground);
        return (void)printf("  (matrix: Bounced Bolt - no floor / wall values)\n");
    }
    for (int i = 0; i < b.n; ++i)
        if (i != A) b.f[i].size = 0;
    put(A, 0, 10, 10);
    for (int y = 0; y < combat::kH; ++y)
        for (int x = 0; x < combat::kW; ++x) b.ground[y][x] = static_cast<uint8_t>(floor_v);
    b.ground[10][16] = static_cast<uint8_t>(wall_v);
    b.indoors = true;
    combat::occupancy(b);
    int nh[2], end[2], ns[2];
    const uint64_t keep = rules::fixes_off();
    for (int on = 0; on < 2; ++on) {
        rules::set_fixes_off(on ? keep & ~(1ull << rules::kFixBolt) : keep | 1ull << rules::kFixBolt);
        int hits[16], n = 0;
        combat::BoltSeg sg[16];
        nh[on] = combat::bolt_path(b, t, A, 13, 10, 7, true, hits, 16, sg, &n, 16);
        ns[on] = n;
        end[on] = n ? sg[n - 1].x1 : -1;
    }
    rules::set_fixes_off(keep);
    memcpy(b.ground, keep_ground, sizeof keep_ground);
    b.indoors = keep_in;
    combat::occupancy(b);
    expect(P, "Bounced Bolt Length: range 7 off a wall 6 out - ends at 11, nobody hit (fix on); the caster hit (off - the original)",
           nh[1] == 0 && ns[1] == 2 && end[1] == 11 && nh[0] == 1, "on %d hit, %d legs, ends %d; off %d hit, %d legs, ends %d",
           nh[1], ns[1], end[1], nh[0], ns[0], end[0]);
}

void darts()
{
    const int dart = play::d->prof->items.dart;
    if (!dart || !play::names || play::names->type(dart).range <= 1) {
        printf("  (matrix: no darts)\n");
        return;
    }
    combat::Battle& b = play::fg->b;
    put(A, 0, 10, 10);
    put(B, 1, 13, 10, 200);
    static uint8_t pile[1][items::kRecordSize];
    memset(pile, 0, sizeof pile);
    pile[0][0x2E] = static_cast<uint8_t>(dart);
    pile[0][0x34] = 1;
    pile[0][0x39] = 10;
    auto* keep = F(A).items;
    const int keep_n = F(A).n_items;
    F(A).items = pile;
    F(A).n_items = 1;
    const int keep_round = b.round;
    for (int r = 0; r < 2; ++r) {
        b.round = r;
        bool ranged = false;
        const int n = combat::slot1_attacks(b, F(A), &ranged);
        snprintf(w, sizeof w, "darts readied x round %d: 3 throws", r + 1);
        expect(P, w, n == 3 && ranged, "attacks %d ranged %d", n, (int)ranged);
    }
    b.round = keep_round;
    F(A).attacks[0] = 3;
    F(A).attacks[1] = 0;
    dice(2);                            // (every d20 a 2: all miss, the target stays)
    play::fg->cur = A;
    const combat::Attack at = combat::attack(b, A, B, play::names, play::rng);
    dice_off();
    expect(P, "darts x one aimed throw: three blows rolled", at.n == 3 && at.shots == 3, "blows %d shots %d", at.n, at.shots);
    F(A).items = keep;
    F(A).n_items = keep_n;
    // Strength with darts readied: the member's own record, 18/00
    if (F(A).member < 0 || !play::pt) return;
    party::Character& ch = play::pt->m[F(A).member];
    static party::Character save;
    save = ch;
    ch.n_items = 1;
    memset(ch.items[0], 0, sizeof ch.items[0]);
    memcpy(ch.items[0], pile[0], items::kRecordSize);
    // (Thrown Weapons - the AD&D 1E Rules, Tom 2026-11-17: Dexterity's missile
    // bonus to hit, no Strength - DMG p.64; off: the original's - neither)
    static const int kStr[3][2] = {{18, 100}, {17, 0}, {12, 0}};
    static const int kDex[3] = {18, 16, 4};
    for (int s = 0; s < 3; ++s) {
        ch.rec[0x10] = ch.rec[0x11] = static_cast<uint8_t>(kStr[s][0]);
        ch.rec[0x1C] = ch.rec[0x1D] = static_cast<uint8_t>(kStr[s][1]);
        ch.rec[0x16] = ch.rec[0x17] = static_cast<uint8_t>(kDex[s]);
        ch.rec[0x125] = 1;
        int hit[2], dmg[2];
        for (int on = 0; on < 2; ++on) {
            rules::set_adnd(on ? 0xFF : 0);
            rules::recalc(ch, *play::names, play::d->facts);
            hit[on] = static_cast<int8_t>(ch.rec[0x199]);
            dmg[on] = static_cast<int8_t>(ch.rec[0x1A2]);
        }
        rules::set_adnd(0xFF);
        // (a game whose darts already count Dexterity - ITEMS flags 2: the rule adds none to hit)
        const bool has_dex = play::names->type(dart).flags & 0x02;
        if (s == 0) printf("  (matrix: darts type %d flags %02X)\n", dart, play::names->type(dart).flags);
        const int wh = has_dex ? 0 : adnd_dex_missile(kDex[s]), wd = 0;
        snprintf(w, sizeof w, "darts x Dex %d, Str %d/%02d: %+d to hit, no Strength damage with Thrown Weapons (DMG p.64), "
                 "none without", kDex[s], kStr[s][0], kStr[s][1], wh);
        expect(P, w, hit[1] - hit[0] == wh && dmg[1] - dmg[0] == wd, "hit %d -> %d, damage %d -> %d", hit[0], hit[1], dmg[0],
               dmg[1]);
    }
    // A dagger (melee and thrown): Strength to hit and damage in the record
    // (melee); thrown at one 3 squares off, Dexterity's to hit in place of
    // Strength's and no Strength damage (Thrown Weapons, DMG p.64), the same
    // as melee without the rule
    int dagger = 0;                              // (the first type both melee and thrown: Curse's hand axe 2)
    for (int t = 1; t < items::kTypes && !dagger; ++t)
        if (play::names->type(t).range > 1 && (play::names->type(t).flags & 0x14) == 0x14)
            dagger = t;
    if (dagger) {
        memset(ch.items[0], 0, sizeof ch.items[0]);
        ch.items[0][0x2E] = static_cast<uint8_t>(dagger);
        ch.items[0][0x34] = 1;
        ch.items[0][0x39] = 1;
        ch.rec[0x10] = ch.rec[0x11] = 18;
        ch.rec[0x1C] = ch.rec[0x1D] = 100;       // (Strength 18/00: +3 to hit, +6 damage)
        ch.rec[0x16] = ch.rec[0x17] = 16;        // (Dexterity 16: +1 missiles)
        ch.rec[0x125] = 1;
        F(A).items = ch.items;
        F(A).n_items = 1;
        int mods[2][2], dmods[2][2];             // [rule][next to / thrown]: to hit, damage
        for (int on = 0; on < 2; ++on) {
            rules::set_adnd(on ? 0xFF : 0);
            rules::recalc(ch, *play::names, play::d->facts);
            for (int far = 0; far < 2; ++far) {
                put(B, 1, far ? 13 : 11, 10, 200);
                combat::occupancy(b);
                static combat::RollLog log;
                log = combat::RollLog{};
                combat::g_rolls = &log;
                F(A).attacks[0] = 1;
                F(A).attacks[1] = 0;
                dice(20);                                // (a 20: the blow lands, its damage rolled)
                play::fg->cur = A;
                combat::attack(b, A, B, play::names, play::rng);
                dice_off();
                combat::g_rolls = nullptr;
                mods[on][far] = dmods[on][far] = -999;
                for (int k = 0; k < log.n; ++k)
                    if (log.r[k].kind == combat::RollKind::Hit && mods[on][far] == -999) mods[on][far] = log.r[k].mod;
                    else if (log.r[k].kind == combat::RollKind::Damage && dmods[on][far] == -999) dmods[on][far] = log.r[k].mod;
            }
        }
        rules::set_adnd(0xFF);
        const int want = adnd_dex_missile(16) - adnd_str_hit(18, 100);
        snprintf(w, sizeof w, "a dagger thrown x Dex 16, Str 18/00: %+d to hit against melee with Thrown Weapons, 0 without", want);
        expect(P, w, mods[0][0] != -999 && mods[1][1] - mods[1][0] == want && mods[0][1] == mods[0][0],
               "rule off: next to %d thrown %d; on: next to %d thrown %d", mods[0][0], mods[0][1], mods[1][0], mods[1][1]);
        const int wantd = -adnd_str_dmg(18, 100);
        snprintf(w, sizeof w, "a dagger thrown x Str 18/00: %+d damage against melee with Thrown Weapons (DMG p.64), 0 without",
                 wantd);
        expect(P, w, dmods[0][0] != -999 && dmods[1][0] != -999 && dmods[1][1] - dmods[1][0] == wantd &&
                         dmods[0][1] == dmods[0][0],
               "rule off: next to %d thrown %d; on: next to %d thrown %d", dmods[0][0], dmods[0][1], dmods[1][0], dmods[1][1]);
        F(A).items = keep;
        F(A).n_items = keep_n;
    } else {
        printf("  (matrix: no dagger that throws)\n");
    }
    ch = save;
    rules::recalc(ch, *play::names, play::d->facts);
}

// ---- T11 Mirror Image and Displacement against blows (dice highest: the
// image die takes it when the game's images take blows). Mirror: Curse's /
// Pool's images only take spells (coab's MirrorImage: spell_id > 0); Secret's
// and Gateway's take blows too (their handlers in the blow's damage phase).
// Displacement (coab's AffectDisplace): the first attack on it misses
void images()
{
    combat::Battle& b = play::fg->b;
    const combat::Facts& fx = *b.fx;
    if (fx.mirror) {
        put(A, 0, 10, 10);
        put(B, 1, 11, 10);
        add_aff(B, fx.mirror, fx.mirror_plain ? 3 : 0x30);
        create::g_die_hook = m_die_max;
        play::fg->cur = A;
        const int before = F(B).hp();
        combat::attack(b, A, B, play::names, play::rng);
        dice_off();
        const bool blows = fx.secret || fx.gw.mirror;
        int k = -1;
        for (int j = 0; F(B).n_aff && j < *F(B).n_aff && k < 0; ++j)
            if (F(B).aff[j][0] == fx.mirror) k = j;
        const int left = k < 0 ? 0 : fx.mirror_plain ? F(B).aff[k][3] : F(B).aff[k][3] >> 4;
        snprintf(w, sizeof w, "mirror images x a blow: %s", blows ? "an image lost, no harm" : "it lands, the images kept");
        expect(P, w, blows ? (F(B).hp() == before && left == 2) : (F(B).hp() < before && left == 3), "hp %d of %d, images %d",
               F(B).hp(), before, left);
    }
    if (fx.items.displace) {
        const bool works = !fx.items.displace_unused || rules::all_fixes();
        put(A, 0, 10, 10);
        put(B, 1, 11, 10);
        add_aff(B, fx.items.displace, 0);
        bool hit1 = false, hit2 = false;
        for (int k = 0; k < 2; ++k) {
            F(A).attacks[0] = 1;
            F(A).attacks[1] = 0;
            dice(20);
            play::fg->cur = A;
            const combat::Attack at = combat::attack(b, A, B, play::names, play::rng);
            dice_off();
            bool h = false;
            for (int j = 0; j < at.n; ++j) h |= at.hits[j].hit;
            (k ? hit2 : hit1) = h;
        }
        snprintf(w, sizeof w, "displacement x two blows (d20 20): %s", works ? "the first misses, the second hits" : "both hit");
        expect(P, w, works ? (!hit1 && hit2) : (hit1 && hit2), "first %d second %d", (int)hit1, (int)hit2);
    }
}

// ---- T13 Protection from Evil against an evil attacker's blow: the
// original's -2 to hit is lost (only the saves count); AD&D's with the
// Game Fixes (SPEC 12)
void prot_evil()
{
    const combat::Facts& fx = *play::fg->b.fx;
    if (!fx.prot_evil) return;
    for (int fix = 0; fix < 2; ++fix) {
        rules::set_all(fix == 1);
        int lowest = 21;
        for (int d20 = 1; d20 <= 20 && lowest == 21; ++d20) {
            put(A, 0, 10, 10);
            put(B, 1, 11, 10);
            F(A).rec[0x11B] = 2;        // (the attacker lawful evil)
            add_aff(B, fx.prot_evil, 0);
            dice(d20);
            play::fg->cur = A;
            const combat::Attack at = combat::attack(play::fg->b, A, B, play::names, play::rng);
            dice_off();
            for (int k = 0; k < at.n; ++k)
                if (at.hits[k].hit) lowest = d20;
        }
        snprintf(w, sizeof w, "protected from evil x an evil attacker's blow, Game Fixes %s: hit from a d20 of %d", fix ? "on" : "off",
                 fix ? 12 : 10);
        expect(P, w, lowest == (fix ? 12 : 10), "lowest hit %d", lowest);
    }
    rules::set_all(true);
}

// ---- T12 morale (coab's FleeCheck sub_3637F; Darkness ovr 0xACAC, Secret
// 0xE22E, Dark Queen 0x2EF27 the same): its morale (control & 0x7F) x 2
// (over 102: 0), Bless +5 / Curse -5 (not below 0), against the hit points
// it has lost (%); then the enemy side's health against the script's base
// (0 or our side: on); then it flees when no enemy moves faster (moves:
// movement x 2, haste x 2, slow / 2, halved), else surrenders with
// Intelligence over 5
void morale_tests()
{
    combat::Battle& b = play::fg->b;
    const combat::Facts& fx = *b.fx;
    auto setup = [&](int ctl, int hp, int mv_b, int intel) {
        put(A, 0, 10, 10);
        put(B, 1, 14, 10, 60);
        F(B).rec[0xF7] = static_cast<uint8_t>(0x80 | ctl);
        F(B).rec[0x1A4] = static_cast<uint8_t>(hp);
        F(B).rec[0x1A5] = static_cast<uint8_t>(mv_b);
        F(B).rec[0x13] = static_cast<uint8_t>(intel);
        b.enemy_health = 10;
        b.morale_base = 0;
    };
    auto test = [&](const char* name, combat::Morale want) {
        const combat::Morale got = combat::morale(b, play::fg->t, B);
        expect(P, name, got == want, "morale %d", (int)got);
    };
    setup(25, 50, 12, 10);
    test("morale x hurt a little (17% lost, morale 50): it fights", combat::Morale::Fight);
    setup(25, 10, 12, 10);
    test("morale x badly hurt, as fast as its enemies: it flees", combat::Morale::Flee);
    setup(25, 10, 6, 10);
    test("morale x badly hurt, enemies faster, Intelligence 10: it surrenders", combat::Morale::Surrender);
    setup(25, 10, 6, 3);
    test("morale x badly hurt, enemies faster, Intelligence 3: it fights", combat::Morale::Fight);
    if (fx.haste) {
        setup(25, 10, 12, 10);
        add_aff(A, fx.haste, 0);
        test("morale x badly hurt, a hasted enemy (twice its moves): it surrenders", combat::Morale::Surrender);
    }
    if (fx.slow) {
        setup(25, 10, 12, 10);
        add_aff(B, fx.slow, 0);
        test("morale x badly hurt and slowed: it surrenders", combat::Morale::Surrender);
    }
    if (fx.curse) {
        setup(2, 60, 12, 10);           // (morale 4, unhurt: Curse brings it to 0 - a failed test)
        add_aff(B, fx.curse, 0);
        test("morale x 4 and cursed (0), unhurt: it flees", combat::Morale::Flee);
    }
    if (fx.bless) {
        setup(25, 30, 12, 10);          // (50% lost; morale 50 + 5)
        add_aff(B, fx.bless, 0);
        test("morale x 50% lost, blessed (55): it fights", combat::Morale::Fight);
    }
    setup(25, 10, 12, 10);
    b.enemy_health = 100;
    b.morale_base = 0;
    test("morale x badly hurt, its side unhurt (100 of 100): it fights", combat::Morale::Fight);
    put(B, 0, 14, 10);
    F(B).rec[0x1A4] = 5;
    test("morale x a party member: no test", combat::Morale::Fight);
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
            if (s.kind == Invisible && (!fx.secret || rules::all_fixes())) want += 4;
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
                rules::set_all(false);
                const int so = save_roll(s);
                rules::set_all(true);
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
    // T8b a flight's runner: back Okay AND able to fight (in_combat 1) - coab
    // CleanupPlayersStateAfterCombat (6.1); without it the next fight placed
    // none of them and counted as won (Death Knights' Soth, 2026-11-24)
    if (F(A).member >= 0) {
        uint8_t* r = play::pt->m[F(A).member].rec;
        for (combat::Result res : {combat::Fled, combat::Won}) {
            r[0x195] = party::Running;
            r[0x196] = 0;
            play::fight_cleanup_party(res);
            snprintf(w, sizeof w, "ran x the fight %s: Okay and in the next fight", res == combat::Fled ? "fled" : "won");
            expect(P, w, r[0x195] == party::Okay && r[0x196] == 1, "status %d, in_combat %d", r[0x195], r[0x196]);
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
    // T14 targeted by a damage spell (its save failed - a d20 of 1 - its
    // dice highest): every status standing takes the damage (AD&D: none
    // of them wards off a spell's harm; mirror images apart - they take
    // single-target spells, T11's)
    if (play::load_magic()) {
        // (one that harms a plain target so: not a touch spell - those roll to hit)
        auto harm = [&](const combat::FightSpell* fs, const Status& s) {
            put(A, 0, 10, 10);
            put(B, 1, 12, 10);
            give(B, s);
            create::g_die_hook = [](int n) { return n == 20 ? 0 : n - 1; };
            combat::SpellLine lines[8];
            const int tg[1] = {B};
            const int before = F(B).hp();
            combat::cast(play::fg->b, play::mrules->tables, A, fs->spell, *fs, tg, 1, play::rng, lines, 8, 9);
            dice_off();
            return before - F(B).hp();
        };
        const Status plain{"none", Plain, 0};
        const combat::FightSpell* fs = nullptr;
        for (int i = 0; i < play::d->prof->fight.n_spells && !fs; ++i) {
            const combat::FightSpell& c = play::d->prof->fight.spells[i];
            if (c.does == combat::SpellDoes::Damage && c.sides && harm(&c, plain) > 0) fs = &c;
        }
        for (const Status& s : list) {
            if (!fs || down(s) || s.kind == Mirror) continue;
            const int lost = harm(fs, s);
            snprintf(w, sizeof w, "%s x a damage spell (%02X, its save failed): harmed", s.name.c_str(), fs->spell);
            expect(P, w, lost > 0, "lost %d", lost);
        }
    }
    weapons();
    darts();
    oil();
    held_step();
    harm_fix();
    cleric_xp();
    stirge();
    heal_feeble();
    summon_room();
    bolt_len();
    images();
    morale_tests();
    prot_evil();
    combat::g_act_hook = fc::on ? fc::on_act : nullptr;
}

} // namespace mx

void behave_matrix() { mx::run(); }
