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
    for (int s = 0; s < 3; ++s) {
        static const int kStr[3][2] = {{18, 100}, {17, 0}, {12, 0}};
        ch.rec[0x10] = ch.rec[0x11] = static_cast<uint8_t>(kStr[s][0]);
        ch.rec[0x1C] = ch.rec[0x1D] = static_cast<uint8_t>(kStr[s][1]);
        ch.rec[0x125] = 1;
        int hit[2], dmg[2];
        for (int fix = 0; fix < 2; ++fix) {
            rules::set_fixes(fix == 1);
            rules::recalc(ch, *play::names, play::d->facts);
            hit[fix] = static_cast<int8_t>(ch.rec[0x199]);
            dmg[fix] = static_cast<int8_t>(ch.rec[0x1A2]);
        }
        rules::set_fixes(true);
        const int wh = adnd_str_hit(kStr[s][0], kStr[s][1]), wd = adnd_str_dmg(kStr[s][0], kStr[s][1]);
        snprintf(w, sizeof w, "darts x Strength %d/%02d: +%d to hit, +%d damage with the Game Fixes, none without",
                 kStr[s][0], kStr[s][1], wh, wd);
        expect(P, w, hit[1] - hit[0] == wh && dmg[1] - dmg[0] == wd, "hit %d -> %d, damage %d -> %d", hit[0], hit[1], dmg[0],
               dmg[1]);
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
        const bool works = !fx.items.displace_unused || rules::fixes();
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
        rules::set_fixes(fix == 1);
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
    rules::set_fixes(true);
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
    images();
    morale_tests();
    prot_evil();
    combat::g_act_hook = fc::on ? fc::on_act : nullptr;
}

} // namespace mx

void behave_matrix() { mx::run(); }
