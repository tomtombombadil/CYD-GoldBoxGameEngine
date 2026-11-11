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
long tapq_seen = 0;                           // (R14: taps during pages / the computer's play)
long qa_seen = 0;                             // (R13: a Quick member's own actions with any switch unticked)

void on_act(const combat::Battle& b, int i, combat::TestAct a, int t, int extra)
{
    using combat::TestAct;
    ++acts;
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
        if (a == TestAct::Cast && !(q & kSpells) && !(static_cast<size_t>(i) < pending.size() && pending[i])) broke = "a spell with Spells unticked";
        if (a == TestAct::UseItem && !(q & kItems)) broke = "an item with Items unticked";
        if (a == TestAct::TurnUndead && !(q & kTurn)) broke = "Turn Undead unticked";
        if (a == TestAct::Guard && !(q & kGuard)) broke = "a guard with Guard unticked";
        if (broke) {
            char m[16];
            snprintf(m, sizeof m, " (mask %02X)", q);
            bad("R13", std::string(broke) + m + ": " + who(b, i));
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
    printf("== fight checks: %ld actions, %ld ticks, %ld Quick actions under unticked switches (R13), %ld taps taking Quick back (R14);", acts, ticks, qa_seen, tapq_seen);
    if (broken.empty()) printf(" no rule broken\n");
    for (auto& kv : broken) printf(" %s x%d", kv.first.c_str(), kv.second);
    if (!broken.empty()) printf("\n");
}

} // namespace fc
