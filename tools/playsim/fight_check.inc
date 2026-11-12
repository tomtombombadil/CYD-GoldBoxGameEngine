// Fight invariants (Claude's side; built only with -DCYD_TEST_HOOKS, included
// by sweep.cpp): every fighter action the engine takes is told here
// (combat::g_act_hook - attack, free attack, guard, cast, step, bandage,
// turn undead, specials, items, the turn's start, harm) and checked against
// the rules - the AD&D 1st edition rules as the Gold Box games use them and
// the facts docs (combat_rules_facts.md 1.3 / 1.4 / 2.5 / 3.2 / 4.1 / 4.5),
// never the engine's own code. The state is checked after every tick too
// (fc::tick from pump / the fuzzer). A broken rule prints a line with the
// fuzzer's seed and fight (FUZZSEED / FUZZONE replay it) and is counted;
// fc::report() prints the totals. The board builds have none of this.
//
// The rules (R1 ... - the report names them):
//  R1  a fighter that is down (unconscious, dying, dead, stoned, gone), fled
//      or not on the field never acts - no turn, blow, free attack, guard
//      blow, spell, step, bandage, item, special or Turn Undead
//      (facts 1.3: those not in combat get delay 0; 4.1)
//  R2  a held one - asleep, held, paralysed, snake-charmed, helpless
//      (Facts::held) - or a stunned one (Power Word Stun) never acts
//      (facts 1.4: held types checked at the turn's start, their turn
//      lost; 2.5a / b: a held enemy gets no free attack, a held guard no
//      guard blow)
//  R3  a blow at a held target slays it: it is down after the blow
//      ("slays helpless ... with one cruel blow", facts 3.2.3)
//  R4  a fighter on its own turn attacks only with attacks left (facts
//      3.2.4; a free attack with none gets one - 2.5a; a guard with none
//      strikes no blow - v0.164.4 / the original's)
//  R5  one fleeing (turned undead, panic, fear) runs on its own turn: it
//      strikes only once its run is over (moves 0 - cornered, or its moves
//      spent) and uses no special attack (breath, gaze). (The original -
//      Pool ovr10 PlayerQuickFight, coab sub_3504B / sub_35DB1 /
//      moralFailureEscape - lets it use items, cast and turn undead before
//      it runs, and fight where it stands when it can run no further.)
//  R6  hit points: never above the maximum, never below 0; a fighter that
//      stands has hit points; standing <=> status Okay / Animated (4.1)
//  R7  the turn's start: delay 1-20 (1.2 / 1.3); the first turn of a round
//      at most d6 + the best Dex reaction (+5): 11
//  R8  fire / cold / electricity immune (MonFx no_fire, no_cold, no_elec)
//      lose no hit points to that kind of harm; R8w: one only magic weapons
//      hurt loses none to a party member's bare hands
//  R9  a charmed one (Charm Person's data 0x20 set) is on its charmer's side
//      (the data's top bit; facts 2.6 / spell_facts)
//  R10 a fight ends: 15 rounds without an attack end it (1.5.7); past
//      FIGHTCAP rounds (default 120) with no fighter's hit points changed
//      in the last 60 rounds (or past 4 x FIGHTCAP) it is reported as
//      endless - a slow fight that still wears its fighters down (only a
//      20 hitting, the Dark Queen's AC -10 heads) is not a stall
//  R11 a blow's target stands when the blow begins (no blows at the fallen)
//  R12 the attacks left never below 0; moves never below 0

namespace fc {

bool on = false;
bool ally_attacked = false;          // ("Attack Ally" answered Yes this fight: the NPC-run turn on the party)
// (the fuzzer: statuses put on others as a turn begins - a point where the game itself changes them)
void (*after_turn_start)(combat::Battle& b, int cur) = nullptr;
long acts = 0, ticks = 0;
std::string ctx;                    // the fuzzer's seed / fight / monster, printed with each break
std::map<std::string, int> broken;  // rule -> count
int print_max = getenv("FCMAX") ? atoi(getenv("FCMAX")) : 6;
int fight_cap = getenv("FIGHTCAP") ? atoi(getenv("FIGHTCAP")) : 120;

const char* act_name(combat::TestAct a)
{
    static const char* n[] = {"attack", "free attack", "attack end", "bite", "cast", "step", "bandage", "turn undead",
                              "special", "taunt", "flee", "turn start", "harm", "harm end", "guard", "use item"};
    const int k = static_cast<int>(a);
    return k >= 0 && k < static_cast<int>(sizeof n / sizeof *n) ? n[k] : "?";
}

std::string who(const combat::Battle& b, int i)
{
    if (i < 0 || i >= b.n) return "#" + std::to_string(i);
    const combat::Fighter& f = b.f[i];
    char nm[24], t[200];
    play::fighter_name(f, nm, sizeof nm);
    snprintf(t, sizeof t, "#%d %s (team %d hp %d/%d status %d up %d size %d delay %d moves %d atk %d/%d ac %d%s%s%s fx", i, nm, f.team(), f.hp(),
             f.hp_max(), f.status(), (int)f.up(), f.size, f.delay, f.moves, f.attacks[0], f.attacks[1], 60 - f.rec[0x19A],
             f.fleeing ? " fleeing" : "", f.guarding ? " guarding" : "", f.quick ? " quick" : "");
    std::string s = t;
    for (int k = 0; f.n_aff && k < *f.n_aff; ++k) {
        snprintf(t, sizeof t, " %02X:%02X", f.aff[k][0], f.aff[k][3]);
        s += t;
    }
    return s + ")";
}

void bad(const char* rule, const std::string& what)
{
    const int n = ++broken[rule];
    ++problems;
    if (n <= print_max)
        printf("!! RULE %s: %s [%s round %d]\n", rule, what.c_str(), ctx.c_str(), play::fg ? play::fg->b.round : -1);
}

// A state break told once a fight for each fighter and rule (the field is
// looked at again and again)
std::set<std::string> told;
void bad_once(const char* rule, int i, const std::string& what)
{
    const std::string key = std::string(rule) + "/" + std::to_string(i) + "/" + what.substr(0, 24);
    if (told.insert(key).second) bad(rule, what);
}

bool held(const combat::Battle& b, const combat::Fighter& f) { return combat::helpless(b, f) || combat::stunned(b, f); }
bool stands(const combat::Fighter& f) { return f.status() == party::Okay || f.status() == party::Animated; }

// The round each fighter last started a turn in (R7); the spell it had begun then (R5)
std::vector<int> turn_round, pending;
// Each fighter's hit points when the checker first saw it: a record stored
// over its maximum (the Dark Queen's lich, MONCHA.GLB #31: 104 of 81 - the
// original's LOAD MONSTER copies the record as it is, DQK 0x356DC) may stay
// up to there; R6 flags only a rise past both
std::vector<int> start_hp;
// Held at the blow's start (R3), standing (R11)
bool blow_held = false;
bool in_bite = false;               // (a hold's bite: Bite, then its Attack)
const void* last_fg = nullptr;
int last_round = -1;
bool endless_told = false;
unsigned long hp_seen = ~0ul;                 // (R10: the fighters' hit points, the round they last changed)
int  hp_round = 0;
long tapq_seen = 0, drain_seen = 0, charm_seen = 0;                           // (R14: taps during pages / the computer's play)
long qa_seen = 0;                             // (R13: a Quick member's own actions with any switch unticked)
// R17 / R18 (Tom, 2026-11-11: "statuses ... enabled when they should be ...
// disabled when they should be ... don't last too long or not long enough"):
// each fighter's effects at the last round (type, minutes), the acts since
struct FxSeen { uint8_t type; int minutes; };
std::vector<std::vector<FxSeen>> fx_prev;
long fx_clock = -1;
long fx_acts = 0;
long fx_count[4] = {};                        // (counted down, ran out, renewed, taken by an act)
// R18: the party's effects at the fight's last tick
std::vector<std::vector<FxSeen>> party_fx;
std::vector<std::string> party_names;           // (by name: one leaving at the fight's end moves the others up)
long party_fx_acts = 0;                       // (the acts when it was taken: any after it, the fight's last tick did more than end it)

void on_act(const combat::Battle& b, int i, combat::TestAct a, int t, int extra)
{
    using combat::TestAct;
    ++acts;
    ++fx_acts;
    if (i < 0 || i >= b.n) {
        bad("R1", std::string(act_name(a)) + " by fighter " + std::to_string(i) + " of " + std::to_string(b.n));
        return;
    }
    const combat::Fighter& f = b.f[i];
    const bool own = play::fg && play::fg->cur == i;
    // (FCACTLOG=n: 200 actions from the n-th traced - FCACTN=m: m of them)
    if (getenv("FCACTLOG") && acts >= atol(getenv("FCACTLOG")) && acts < atol(getenv("FCACTLOG")) + (getenv("FCACTN") ? atol(getenv("FCACTN")) : 200)) {
        printf("  [act %ld] round %d %s -> %d (%d): %s\n", acts, b.round, act_name(a), t, extra, who(b, i).c_str());
        // (FCROLLS=1: each blow's rolls too - the Show Rolls log, kept here)
        static combat::RollLog rolls;
        if (getenv("FCROLLS")) {
            if (a == TestAct::AttackEnd)
                for (int k = 0; k < rolls.n; ++k)
                    printf("      roll: on %d kind %d blow %d need %d nat %d mod %d\n", rolls.r[k].who, (int)rolls.r[k].kind,
                           rolls.r[k].blow, rolls.r[k].need, rolls.r[k].nat, rolls.r[k].mod);
            rolls.n = 0;
            combat::g_rolls = &rolls;
        }
    }
    switch (a) {
    case TestAct::AttackEnd:
        in_bite = false;
        // (but one that gets up again as it falls - Gateway's boar, "stands up and grins": b.risen)
        if (blow_held && t >= 0 && t < b.n && b.f[t].up() && b.f[t].size && b.risen != t)
            bad("R3", "a held target still stands after the blow: " + who(b, t) + " by " + who(b, i));
        // R8w: one hit only by magic weapons (stop_normal, the silver-only
        // undead, +1 / +2 / +3 needed) loses nothing to a party member's bare
        // hands (AD&D: an unarmed character has no magic to get through)
        if (b.fx && t >= 0 && t < b.n && f.member >= 0 && play::names && combat::weapon(f, *play::names) < 0 && !blow_held) {
            const combat::MonFx& m = b.fx->mon;
            const combat::Fighter& tg = b.f[t];
            auto hasm = [&](uint8_t type) { return type && tg.has(type); };
            if ((hasm(m.stop_normal) || hasm(m.silver_only) || hasm(m.need_plus1) || hasm(m.need_plus2) || hasm(m.need_plus3)) &&
                tg.hp() < extra)
                bad("R8w", "a bare-handed blow hurt one only magic hurts: " + who(b, t) + " (hp before " + std::to_string(extra) +
                               ") by " + who(b, i));
        }
        return;
    case TestAct::Harm:
        return;
    case TestAct::HarmEnd: {
        if (!b.fx) return;
        const combat::MonFx& m = b.fx->mon;
        const bool lost = f.hp() < extra;
        auto immune = [&](uint8_t type, int bit) { return type && f.has(type) && (t & bit); };
        if (lost && (immune(m.no_fire, 1) || immune(m.no_cold, 2) || immune(m.no_elec, 4)))
            bad("R8", "harm kind " + std::to_string(t) + " through an immunity: " + who(b, i) + " hp before " + std::to_string(extra));
        return;
    }
    default: break;
    }
    // R1 / R2: the actor can act
    if (!f.size || f.gone || f.escaped || !f.up() || !stands(f))
        bad("R1", std::string(act_name(a)) + " by one who can't act: " + who(b, i) + (own ? " (own turn)" : ""));
    // (the step a free attack held it in: the original takes it, the Game Fixes not - SPEC 12)
    else if (held(b, f) && !(a == TestAct::Step && !rules::fixes()))
        bad("R2", std::string(act_name(a)) + " by a held / stunned one: " + who(b, i) + (own ? " (own turn)" : ""));
    // R5: one fleeing strikes on its own turn only with its run over; no specials
    // (a hold's bite at the turn's start - Pool's mantis, its own rule: not a blow of its choosing)
    if (own && f.fleeing && ((a == TestAct::Attack && f.moves > 0 && !in_bite) || a == TestAct::Special))
        bad("R5", std::string(act_name(a)) + " on its own turn while fleeing (moves left " + std::to_string(f.moves) + "): " + who(b, i));
    // R14 a tap takes the party back from Quick (Tom, 2026-11-10): one of
    // the player's own characters off Quick is never played by the computer
    // on its own turn (an action while the fight is in its Ai state)
    // (its turn's start isn't a choice; a spell it began itself before is the player's)
    if (own && a != TestAct::TurnStart && !(a == TestAct::Cast && static_cast<size_t>(i) < pending.size() && pending[i]) &&
        play::fg->st == play::FSt::Ai && f.member >= 0 && !f.team() && !f.quick && !f.fleeing &&
        f.rec[0xF7] < 0x80 && f.status() != party::Animated && !play::qa_npc(f.member))
        bad("R14", std::string(act_name(a)) + " by the computer for one taken off Quick: " + who(b, i));
    // R13 Quick Actions (Tom, 2026-11-09): one of the player's own characters
    // played by Quick - or one of the party's NPCs (the computer's; their
    // switches too since Tom's 2026-11-09 addition) - does on its own turn
    // nothing its switches untick -
    // Melee: no blow at one next to it; Missile: no shot; Move: no step
    // (fleeing aside); Spells: no spell (one it began itself before - the
    // player's - aside); Items: no item; Turn Undead: no turn; Guard: no
    // guard. (Its free attacks and guard blows are the game's, not checked.)
    const bool qa_npc = !f.team() && play::qa_npc(f.member) && f.status() != party::Animated;
    if (own && play::qa_on() &&
        (qa_npc || (f.quick && !f.team() && f.member >= 0 && f.rec[0xF7] < 0x80 && f.status() != party::Animated))) {
        using namespace quickacts;
        const uint8_t q = f.qa;
        if ((q | quickacts::kBandage | quickacts::kSpells) != quickacts::kAll) ++qa_seen;
        const char* broke = nullptr;
        if (a == TestAct::Attack && !in_bite && t >= 0 && t < b.n) {
            const bool next = combat::adjacent(b, i, t);
            // (one next to it with Melee unticked: only a throw - a dagger, a
            // spear thrown at it, Missile ticked)
            int ammo = -1;
            if (next && !(q & kMelee) && !((q & kMissile) && combat::missile(b, f, &ammo) > 0)) broke = "a blow with Melee unticked";
            if (!next && !(q & kMissile)) broke = "a shot with Missile unticked";
        }
        if (a == TestAct::Step && !f.fleeing && !(q & kMove)) broke = "a step with Move unticked";
        // R19 (Tom, 2026-11-11: his PoolRad fighters charged with bows and
        // arrows): one of the player's own, Missile ticked, no foe next to
        // it, a bow / crossbow / sling it may use with its ammunition and a
        // foe in sight within its reach - no step, it shoots
        if (a == TestAct::Step && !f.fleeing && !qa_npc && (q & kMissile) && (f.attacks[0] > 0 || f.attacks[1] > 0) && play::names && f.items) {
            bool next = false;
            for (int c = 0; c < b.n && !next; ++c)
                next = c != i && b.f[c].up() && b.f[c].size && b.f[c].team() != f.team() && combat::adjacent(b, i, c);
            const int held = combat::weapon(f, *play::names);
            const bool stuck = held >= 0 && f.items[held][0x36];
            int reach = 0, wk = -1;
            for (int k = 0; k < f.n_items && !next && !stuck; ++k) {
                const items::TypeInfo& ti = play::names->type(f.items[k][0x2E]);
                // (a launcher or sling - thrown items, oil and holy water among
                // them, by the games' rating - that can hurt the foe it aims
                // at by the game's rating: Darkness's iron golem and a +0 bow)
                if (ti.slot != items::kSlotWeapon || !(ti.flags & 0x08) || (ti.flags & 0x10) || ti.range <= 1 ||
                    !(ti.classes & f.rec[0x12B]))
                    continue;
                // (one that hurts every foe up in sight by the game's rating -
                // Darkness's iron golem, a +3 weapon's, beside others: the
                // original rates against its target only)
                bool hurts = true;
                for (int c = 0; c < b.n && hurts; ++c) {
                    int sq;
                    if (c != i && b.f[c].up() && b.f[c].size && b.f[c].team() != f.team() && !combat::hidden(b, i, c) &&
                        combat::range(b, play::fg->t, i, c, false, &sq))
                        hurts = combat::ai_rating(b, i, f.items[k], 0, c) > 0;
                }
                if (!hurts || combat::ai_rating(b, i, f.items[k], 0) <= 0) continue;
                bool loaded = (ti.flags & 0xFF) == 0x0A;
                for (int j = 0; !loaded && j < f.n_items; ++j) {
                    if (!f.items[j][0x34]) continue;
                    const int ty = f.items[j][0x2E];
                    loaded = ((ti.flags & 0x01) && ty == b.arrow) || ((ti.flags & 0x80) && ty == b.quarrel);
                }
                const int r = ti.range - 1 > 1 ? ti.range - 1 : 1;
                if (loaded && r > reach) { reach = r; wk = k; }
            }
            for (int c = 0; c < b.n && reach > 0; ++c) {
                int sq;
                if (c == i || !b.f[c].up() || !b.f[c].size || b.f[c].team() == f.team() || combat::hidden(b, i, c)) continue;
                if (combat::range(b, play::fg->t, i, c, false, &sq) && sq <= reach) {
                    static std::string m;
                    m = "a step with Missile ticked, item " + std::to_string(wk) + " loaded (held " +
                        std::to_string(combat::weapon(f, *play::names)) + "), " + who(b, c) + " " +
                        std::to_string(sq) + " squares off in sight";
                    broke = m.c_str();
                    break;
                }
            }
        }
        if (a == TestAct::Cast && !(q & kSpells) && !(static_cast<size_t>(i) < pending.size() && pending[i])) broke = "a spell with Spells unticked";
        if (a == TestAct::UseItem && !(q & kItems)) broke = "an item with Items unticked";
        if (a == TestAct::TurnUndead && !(q & kTurn)) broke = "Turn Undead unticked";
        if (a == TestAct::Guard && !(q & kGuard)) broke = "a guard with Guard unticked";
        if (broke) {
            char m[16];
            snprintf(m, sizeof m, " (mask %02X)", q);
            bad(strncmp(broke, "a step with Missile", 19) ? "R13" : "R19", std::string(broke) + m + ": " + who(b, i));
        }
    }
    switch (a) {
    case TestAct::Bite: in_bite = true; break;
    case TestAct::Attack:
    case TestAct::FreeAttack:
        blow_held = t >= 0 && t < b.n && combat::helpless(b, b.f[t]);
        if (t < 0 || t >= b.n || !b.f[t].up() || !b.f[t].size)
            bad("R11", std::string(act_name(a)) + " at one down: " + who(b, t) + " by " + who(b, i));
        if (a == TestAct::Attack && extra <= 0) bad("R4", std::string("a blow with no attacks left: ") + who(b, i) + (own ? " (own turn)" : " (guard)"));
        break;
    case TestAct::TurnStart: {
        if (extra < 1 || extra > 20) bad("R7", "a turn with delay " + std::to_string(extra) + ": " + who(b, i));
        if (turn_round.size() < static_cast<size_t>(b.n)) turn_round.resize(b.n, -1);
        if (pending.size() < static_cast<size_t>(b.n)) pending.resize(b.n, 0);
        pending[i] = f.spell;
        if (turn_round[i] != b.round) {
            turn_round[i] = b.round;
            if (extra > 11 && extra != 20) bad("R7", "a round's first turn at delay " + std::to_string(extra) + ": " + who(b, i));
        }
        if (after_turn_start) after_turn_start(const_cast<combat::Battle&>(b), i);
        break;
    }
    default: break;
    }
}

// R18 the fight's end: the game's own list (its program's - none in
// PoolRad) leaves no party character with one of its effects, the charm
// and Darkness's dragon fear neither with Game Fixes; every other effect
// still running at the fight's last tick is kept (the animated dead's goes
// with them; a timed one at 1 minute may have just run out)
long end_kept = 0, end_taken = 0;
void fight_end()
{
    if (!play::pt || !play::d) return;
    const auto& fp = play::d->prof->fight;
    auto ended_by_fight = [&](uint8_t t) {
        for (uint8_t e : fp.ends)
            if (e && e == t) return true;
        if (rules::fixes() && ((fp.facts.charm && t == fp.facts.charm) || (fp.facts.dk.feared && t == fp.facts.dk.feared)))
            return true;
        return false;
    };
    for (int ci = 0; ci < play::pt->count; ++ci) {
        const party::Character& ch = play::pt->m[ci];
        char nm[20];
        ch.name(nm, sizeof nm);
        int i = -1;
        for (int j = 0; j < (int)party_names.size(); ++j)
            if (party_names[j] == nm) i = j;
        if (i < 0 || i >= (int)party_fx.size()) continue;
        for (int k = 0; k < ch.n_affects; ++k)
            if (ended_by_fight(ch.affects[k][0]))
                bad("R18", "effect " + std::to_string(ch.affects[k][0]) + " left after the fight on " + nm);
        for (const FxSeen& was : party_fx[i]) {
            if (ended_by_fight(was.type)) {
                ++end_taken;
                continue;
            }
            if (fp.facts.sp.animated && was.type == fp.facts.sp.animated) continue;
            if (was.minutes == 1) continue;
            if (acts != party_fx_acts) continue;        // (blows and spells after the last look)
            bool still = false;
            for (int k = 0; k < ch.n_affects; ++k)
                if (ch.affects[k][0] == was.type) still = true;
            if (still) ++end_kept;
            else {
                bad("R18", "effect " + std::to_string(was.type) + " (" + std::to_string(was.minutes) + " minutes) taken by the fight's end from " + nm);
                if (getenv("R18DBG")) {
                    printf("   before:");
                    for (const FxSeen& w : party_fx[i]) printf(" %02X:%d", w.type, w.minutes);
                    printf("\n   after:");
                    for (int k = 0; k < ch.n_affects; ++k) printf(" %02X:%d/%02X", ch.affects[k][0], ch.affects[k][1] | ch.affects[k][2] << 8, ch.affects[k][3]);
                    printf("\n   status %d hp %d\n", ch.rec[0x195], ch.hp());
                    printf("   ends:");
                    for (uint8_t e : fp.ends) printf(" %02X", e);
                    printf(" charm %02X feared %02X animated %02X\n", fp.facts.charm, fp.facts.dk.feared, fp.facts.sp.animated);
                }
            }
        }
    }
    party_fx.clear();
}

void install()
{
    on = true;
    combat::g_act_hook = on_act;
}

// The state after a tick
void tick()
{
    if (!on) return;
    ++ticks;
    const auto* fg = play::fg;
    if (!fg || !play::ml) {         // (after the fight's end its monsters are let go before the results)
        last_fg = nullptr;
        return;
    }
    const combat::Battle& b = fg->b;
    if (const char* wv = getenv("FCWATCH")) {           // (FCWATCH=i: fighter i as it changes)
        static std::string last;
        const int wi = atoi(wv);
        if (wi < b.n) {
            char t[400];
            snprintf(t, sizeof t, "round %d cur %d st %d menu [%s%s] | %s", b.round, fg->cur, (int)fg->st, play::menu.prompt,
                     play::menu.s, who(b, wi).c_str());
            if (t != last) printf("  [watch] %s\n", t);
            last = t;
        }
    }
    if (fg != last_fg) {
        last_fg = fg;
        last_round = -1;
        endless_told = false;
        hp_seen = ~0ul;
        hp_round = 0;
        turn_round.assign(b.n, -1);
        pending.assign(b.n, 0);
        start_hp.assign(combat::kMaxFighters, -1);
        told.clear();
        ally_attacked = false;
        fx_clock = -1;
        fx_prev.clear();
    }
    // R15 the Companion's Drained (Tom, 2026-11-10): a party character
    // drained in the fight - its levels fell (only a drain lowers them) or its
    // lost levels (0xE7) rose - shows Drained (the dead forever too)
    if (play::pt) {
        static std::vector<int> lv0, e70;
        static const void* lv_fight = nullptr;
        if (lv_fight != fg) {
            lv_fight = fg;
            lv0.assign(play::pt->count, -1);
            e70.assign(play::pt->count, -1);
        }
        for (int i = 0; i < play::pt->count && i < (int)lv0.size(); ++i) {
            const party::Character& ch = play::pt->m[i];
            int sum = 0;
            for (int c = 0; c < 8; ++c) sum += ch.rec[0x109 + c];
            if (lv0[i] < 0) lv0[i] = sum, e70[i] = ch.rec[0xE7];
            if (sum < lv0[i] || ch.rec[0xE7] > e70[i]) {
                ++drain_seen;
                play::PanelMember m;
                if (play::panel_member(i, m) && !m.drained) {
                    char nm[20];
                    ch.name(nm, sizeof nm);
                    bad("R15", std::string("drained in the fight (levels ") + std::to_string(lv0[i]) + " -> " + std::to_string(sum) +
                                   ", lost " + std::to_string(e70[i]) + " -> " + std::to_string(ch.rec[0xE7]) + "), not shown Drained: " + nm);
                }
            }
            lv0[i] = sum;
            e70[i] = ch.rec[0xE7];
        }
    }
    // R17 the effects' minutes, round by round (a round = a minute; time 0
    // for ever; at or under 1 it ends at the round's turn - Curse's end,
    // Darkness's / the Dark Queen's start - monster_fx_facts 2.1,
    // rules_round16 1): too long - one at 1 minute still there, not put on
    // again; a wrong count - one that didn't lose a minute; too short - one
    // with time left gone in a round nobody acted in
    // (measured by the game clock's minutes, which the effects follow -
    // the Dark Queen steps it as a round starts, the others as one ends)
    const long clock = play::vm ? play::vm->minutes_all() : 0;
    if (clock != fx_clock) {
        const int dt = static_cast<int>(clock - fx_clock);
        const bool next = fx_clock >= 0 && dt > 0 && dt <= 2 && (int)fx_prev.size() == b.n;
        const combat::MonFx* mf = b.fx ? &b.fx->mon : nullptr;
        for (int i = 0; next && i < b.n; ++i) {
            const combat::Fighter& f = b.f[i];
            // (one type may be held more than once - an item's lasting
            // Protection beside the spell's: each type's times compared as a set)
            std::map<int, std::vector<int>> was_t, now_t;
            for (const FxSeen& w : fx_prev[i]) was_t[w.type].push_back(w.minutes);
            for (int k = 0; f.n_aff && k < *f.n_aff; ++k) now_t[f.aff[k][0]].push_back(f.aff[k][1] | f.aff[k][2] << 8);
            for (auto& [type, wl] : was_t) {
                // (the stirge's / leech's hold keeps its blood in its time)
                if (mf && (type == mf->sucking || type == mf->blood)) continue;
                std::vector<int> nl = now_t.count(type) ? now_t[type] : std::vector<int>{};
                // the expected: the lasting ones as they were, each timed one a minute less, those at 1 gone
                std::multiset<int> want, got(nl.begin(), nl.end());
                for (int m : wl)
                    if (m == 0) want.insert(0);
                    else if (m > dt) want.insert(m - dt);
                    else ++fx_count[1];
                for (auto it = want.begin(); it != want.end();) {
                    auto g = got.find(*it);
                    if (g != got.end()) {
                        if (*it) ++fx_count[0];
                        got.erase(g);
                        it = want.erase(it);
                    } else ++it;
                }
                // left in want: gone early or counted wrongly; left in got: put on again, or kept too long
                for (int m : got) {
                    bool renew = false;
                    // (with an act in that minute any further entry is a new effect)
                    for (int w : wl)
                        if (m > w - dt + 1) renew = true;
                    if (renew || fx_acts) {
                        ++fx_count[2];
                        // (a want entry it may stand for)
                        if (!want.empty()) want.erase(want.begin());
                        continue;
                    }
                    bad_once("R17", i, "effect " + std::to_string(type) + " at " + std::to_string(m) + " minutes, not " + std::to_string(dt) + " less than it was (" +
                                           [&] { std::string t; for (int w : wl) t += std::to_string(w) + " "; return t; }() + "): " + who(b, i));
                }
                for (int m : want) {
                    if (fx_acts) ++fx_count[3];
                    else if (f.up())
                        bad_once("R17", i, "effect " + std::to_string(type) + " (" + std::to_string(m + dt) +
                                               " minutes left) gone in a round nobody acted in: " + who(b, i));
                }
            }
        }
        if (const char* tr = getenv("FXTRACE")) {        // (FXTRACE=i: fighter i's effects' minutes each round)
            const int ti = atoi(tr);
            if (ti < b.n && b.f[ti].n_aff) {
                printf("  [fxtrace] round %d minute %ld:", b.round, clock);
                for (int k = 0; k < *b.f[ti].n_aff; ++k) printf(" %02X:%d", b.f[ti].aff[k][0], b.f[ti].aff[k][1] | b.f[ti].aff[k][2] << 8);
                printf("\n");
            }
        }
        fx_prev.assign(b.n, {});
        for (int i = 0; i < b.n; ++i)
            for (int k = 0; b.f[i].n_aff && k < *b.f[i].n_aff; ++k)
                fx_prev[i].push_back(FxSeen{b.f[i].aff[k][0], b.f[i].aff[k][1] | b.f[i].aff[k][2] << 8});
        fx_clock = clock;
        fx_acts = 0;
    }
    if (const char* wn = getenv("AFFWATCH")) {        // (AFFWATCH=NAME: that member's effects as they change)
        static std::string last;
        for (int i = 0; play::pt && i < play::pt->count; ++i) {
            char nm[20];
            play::pt->m[i].name(nm, sizeof nm);
            if (strcmp(nm, wn)) continue;
            std::string t;
            for (int k = 0; k < play::pt->m[i].n_affects; ++k) {
                char e[16];
                snprintf(e, sizeof e, " %02X:%d", play::pt->m[i].affects[k][0], play::pt->m[i].affects[k][1] | play::pt->m[i].affects[k][2] << 8);
                t += e;
            }
            if (t != last) printf("  [affwatch] round %d st %d cur %d acts %ld:%s\n", b.round, (int)fg->st, fg->cur, acts, t.c_str());
            last = t;
        }
    }
    // (R18: the party's effects as the fight goes)
    if (play::pt) {
        party_fx_acts = acts;
        party_fx.assign(play::pt->count, {});
        party_names.assign(play::pt->count, {});
        for (int i = 0; i < play::pt->count; ++i) {
            char nm[20];
            play::pt->m[i].name(nm, sizeof nm);
            party_names[i] = nm;
        }
        for (int i = 0; i < play::pt->count; ++i)
            for (int k = 0; k < play::pt->m[i].n_affects; ++k)
                party_fx[i].push_back(FxSeen{play::pt->m[i].affects[k][0], play::pt->m[i].affects[k][1] | play::pt->m[i].affects[k][2] << 8});
    }
    if (b.round == last_round && ticks % 8) return;     // (the whole field: each round, and every 8th tick)
    last_round = b.round;
    for (int i = 0; i < b.n; ++i) {
        const combat::Fighter& f = b.f[i];
        if (f.gone) continue;
        // (but the electricity eaters: +8 a bolt, no limit - the original's handler, coab ovr013)
        if (i < (int)start_hp.size() && start_hp[i] < 0) start_hp[i] = f.hp();
        const int cap = i < (int)start_hp.size() && start_hp[i] > f.hp_max() ? start_hp[i] : f.hp_max();
        if (f.hp() > cap && f.hp_max() > 0 && !(b.fx && b.fx->mon.absorb_elec && f.has(b.fx->mon.absorb_elec)))
            bad_once("R6", i, "hit points over the maximum: " + who(b, i));
        if (f.up() && !stands(f)) bad_once("R6", i, "standing with status " + std::to_string(f.status()) + ": " + who(b, i));
        if (f.up() && f.size && f.hp() <= 0 && f.status() != party::Animated) bad_once("R6", i, "standing at 0 hit points: " + who(b, i));
        if (!f.up() && f.status() == party::Okay && f.size && f.hp() > 0) bad_once("R6", i, "Okay with hit points, yet not standing: " + who(b, i));
        if (f.attacks[0] < 0 || f.attacks[1] < 0 || f.moves < 0) bad_once("R12", i, "attacks / moves below 0: " + who(b, i));
        if (f.delay < 0 || f.delay > 20) bad_once("R7", i, "delay out of range: " + who(b, i));
        // R9: charmed - on the charmer's side (but after "Attack Ally" Yes - 0x7F33: every NPC-run one turns on the party, the original's, facts 2.4b)
        if (b.fx && b.fx->charm && f.up() && !(ally_attacked || (play::vm && play::vm->get(0x7F33))))
            for (int k = 0; f.n_aff && k < *f.n_aff; ++k)
                if (f.aff[k][0] == b.fx->charm && (f.aff[k][3] & 0x20) && ((f.aff[k][3] >> 7) & 1) != (f.team() ? 1 : 0))
                    bad_once("R9", i, "charmed, but not on the charmer's side: " + who(b, i));
    }
    // R1 / R2 on a party member's menu (the turn offered)
    if (fg->st == play::FSt::Menu && fg->cur >= 0 && fg->cur < b.n) {
        const combat::Fighter& f = b.f[fg->cur];
        if (!f.up() || !f.size) bad("R1", "a menu for one who can't act: " + who(b, fg->cur));
        else if (held(b, f)) bad("R2", "a menu for a held / stunned one: " + who(b, fg->cur));
    }
    {
        unsigned long hp = 0;   // (a hash of the hit points: wraps, never overflows)
        for (int i = 0; i < b.n; ++i) hp = hp * 7 + static_cast<unsigned long>(b.f[i].hp() + (b.f[i].up() ? 1000 : 0));
        if (hp != hp_seen) hp_seen = hp, hp_round = b.round;
    }
    if (b.round > fight_cap && (b.round - hp_round > 60 || b.round > 4 * fight_cap) && !endless_told) {
        endless_told = true;
        std::string s = "the fight goes on past round " + std::to_string(fight_cap) + " (no action until " +
                        std::to_string(b.no_action) + ", hit points last changed in round " + std::to_string(hp_round) + "):";
        for (int i = 0; i < b.n; ++i)
            if (b.f[i].up() && b.f[i].size) s += " " + who(b, i);
        bad("R10", s);
    }
}

void report()
{
    printf("== fight checks: %ld actions, %ld ticks, %ld Quick actions under unticked switches (R13), %ld taps taking Quick back (R14), %ld drains (R15), %ld charms put on (R16); effects (R17) %ld counted down, %ld ran out, %ld put on again, %ld taken by acts; fight ends (R18) %ld kept, %ld taken;", acts, ticks, qa_seen, tapq_seen, drain_seen, charm_seen, fx_count[0], fx_count[1], fx_count[2], fx_count[3], end_kept, end_taken);
    if (broken.empty()) printf(" no rule broken\n");
    for (auto& kv : broken) printf(" %s x%d", kv.first.c_str(), kv.second);
    if (!broken.empty()) printf("\n");
}

} // namespace fc
