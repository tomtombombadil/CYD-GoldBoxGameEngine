// The fight fuzzer (sweep mode `fightfuzz [seed [fights]]`, -DCYD_TEST_HOOKS):
// fight after fight from the saved game - each a monster picked at random
// from the game's monster files (MON<n>CHA.DAX blocks; the Dark Queen's
// MONCHA.GLB entries) in 1-6 copies at the save's LOAD MONSTER (FIGHTAT /
// start_fight's per-game address), the game's dice seeded per fight, the
// party's turns played at random (attack, move, cast a random fight spell,
// guard, turn, bandage, Quick ...) and, at the rounds' starts, statuses put
// on random fighters (asleep / held / paralysed / stunned / blessed / cursed
// / hasted / slowed / invisible / blinded / protected / silenced ...),
// random guards, the turned (fleeing) and the fallen (dying). The fight
// checker (fight_check.inc) checks every action and tick. Each fight's
// seed is FUZZSEED * 7919 + fight: FUZZONE=k plays fight k alone (the same
// steps - the replay); FUZZLOG=1 prints each fight's line.
// Env as the other modes (GAME, BOOTSLOT, MAREA / MBLOCK - Champions'
// script block, FIGHTAT, CACHE); FUZZFX=0: no statuses put on. Each game
// (its own copy of the folder, a `_CYD` beside it): Pool GAME=1, Curse 2,
// Secret 3, Darkness 4, Gateway 8 (its test saves in SAVE), Treasures 9
// (CACHE=/GOLDBOX/_CYD/<folder>), Champions 5 MAREA=2 MBLOCK=34
// FIGHTAT=8441, Death Knights 6 BOOTSLOT=B FIGHTAT=815B, Dark Queen 7
// FIGHTAT=8C11. Build: sweep.cpp with -fsanitize=address,undefined
// -DCYD_TEST_HOOKS (its header).

namespace ff {

struct Mon {
    int area, id;
};
std::vector<Mon> mons;

void list_monsters()
{
    const std::string folder = game_dir.substr(game_dir.rfind('/') + 1);
    char path[300];
    for (int disk = 0; disk <= 3; ++disk) {             // (the Dark Queen's: DISK2/MONCHA.GLB)
        if (disk) snprintf(path, sizeof path, "/GOLDBOX/%s/DISK%d/MONCHA.GLB", folder.c_str(), disk);
        else snprintf(path, sizeof path, "/GOLDBOX/%s/MONCHA.GLB", folder.c_str());
        if (plat::exists(path)) break;
    }
    if (plat::exists(path)) {
        plat::File f = plat::open(path, "r");
        plat::FileSource src(f);
        for (int id = 1; id < 512; ++id) {
            uint32_t pos, len;
            if (hlib::find_entry(src, static_cast<uint16_t>(id), pos, len) && len >= 4) mons.push_back({-1, id});
        }
        f.close();
        return;
    }
    for (int area = 0; area <= 9; ++area) {
        if (area == 0) snprintf(path, sizeof path, "/GOLDBOX/%s/MONCHA.DAX", folder.c_str());
        else snprintf(path, sizeof path, "/GOLDBOX/%s/MON%dCHA.DAX", folder.c_str(), area);
        plat::File f = plat::open(path, "r");
        if (!f) continue;
        plat::FileSource src(f);
        static dax::Index dir;
        if (dax::read_index(src, dir) == dax::Status::Ok)
            for (int i = 0; i < dir.count; ++i) mons.push_back({area ? area : 1, dir.entries[i].id});
        f.close();
    }
}

// A status put on a fighter as the game keeps its effects
void put_fx(combat::Fighter& f, int type, int minutes)
{
    if (!type || !f.n_aff || *f.n_aff >= f.max_aff) return;
    if (combat::Monster* m = f.rec.monster()) {
        if (!m->room_for_aff(*f.n_aff + 1)) return;
        f.aff = m->aff;
    }
    uint8_t* a = f.aff[(*f.n_aff)++];
    memset(a, 0, party::kAffectSize);
    a[0] = static_cast<uint8_t>(type);
    a[1] = static_cast<uint8_t>(minutes);
}

std::mt19937 fr;
int fpick(int n) { return n > 0 ? static_cast<int>(fr() % static_cast<unsigned>(n)) : 0; }
std::string steps;                  // what the fuzzer did this fight (printed with a broken rule)
int fxs_put = 0;

void stir(combat::Battle& b, int cur)
{
    if (!b.fx || (getenv("FUZZFX") && !atoi(getenv("FUZZFX")))) return;
    const combat::Facts& fx = *b.fx;
    const uint8_t types[] = {fx.held[0], fx.held[1], fx.held[2], fx.held[3], fx.sp.stun, fx.bless,  fx.curse,
                             fx.haste,   fx.slow,    fx.invisible, fx.blinded, fx.prot_evil, fx.prot_good, fx.silence,
                             fx.entangle, fx.faerie, fx.bestow, fx.fumbling, fx.cough, fx.blink};
    const int n_types = sizeof types / sizeof *types;
    const int tries = fpick(4) == 0 ? 1 + fpick(2) : 0;
    for (int k = 0; k < tries; ++k) {
        const int i = fpick(b.n);
        if (i == cur) continue;          // (never the one whose turn begins: its state is set)
        combat::Fighter& f = b.f[i];
        if (!f.up() || !f.size) continue;
        const int r = fpick(100);
        char t[64];
        if (r < 55) {
            const int ty = types[fpick(n_types)];
            if (!ty) continue;
            put_fx(f, ty, 1 + fpick(4));
            snprintf(t, sizeof t, " r%d:fx%02X>%d", b.round, ty, i);
            ++fxs_put;
        } else if (r < 80) {
            f.guarding = true;
            snprintf(t, sizeof t, " r%d:guard>%d", b.round, i);
        } else if (r < 90) {
            if (f.member >= 0) continue;    // (the turned: monsters - not a charmed party member)
            f.fleeing = true;
            snprintf(t, sizeof t, " r%d:flee>%d", b.round, i);
        } else {
            combat::damage(b, i, f.hp() + 1 + fpick(5));
            snprintf(t, sizeof t, " r%d:fell>%d", b.round, i);
        }
        steps += t;
    }
}

// A party member's turn at random
void party_turn()
{
    using namespace play;
    const int r = fpick(100);
    char t[24];
    snprintf(t, sizeof t, " r%d:%d:", fg->b.round, fg->cur);
    steps += t;
    if (r < 22) {
        steps += "Q";
        if (!tap_word('Q')) tap_item(0);
    } else if (r < 45) {
        steps += "A";
        if (!tap_word('A')) tap_item(fpick(menu.count));
    } else if (r < 62) {
        steps += "M";
        if (!tap_word('M')) tap_item(fpick(menu.count));
    } else if (r < 74) {
        combat::Fighter& f = fg->b.f[fg->cur];
        if (f.member >= 0 && load_magic() && d->prof->fight.n_spells) {
            const int sp = d->prof->fight.spells[fpick(d->prof->fight.n_spells)].spell;
            snprintf(t, sizeof t, "C%02X", sp);
            steps += t;
            f.rec[magic::kListAt] = static_cast<uint8_t>(sp);
            fight_spell_chosen(sp, C);
        } else {
            tap_item(fpick(menu.count));
        }
    } else if (r < 84) {
        steps += "G";
        if (!tap_word('G') && tap_word('D')) tap_word('G');
    } else {
        steps += "?";
        tap_item(fpick(menu.count));
    }
}

// One fight to its end (true: it ended)
bool fight(int k)
{
    using namespace play;
    long it = 0;
    int moves_made = 0, aim_tries = 0;

    int same = 0, last_sig = -1;
    while (fg || screen == Screen::Fight) {
        if (++it > 3000000) {
            fc::bad("R10", "the fuzzer's tick limit (" + steps.substr(0, 200) + ")");
            return false;
        }
        g_now += 20;
        play::tick(g_now, C);
        fc::tick();
        if (!d) return false;
        if (!fg) break;
        if (fg->b.round > fc::fight_cap + 5) return false;
        if (input_mode != Input::None) {
            play::input_key('1', C);
            play::input_key('\n', C);
            continue;
        }
        const int sig = static_cast<int>(screen) * 1000 + static_cast<int>(fg->st) * 10 + fg->cur;
        same = sig == last_sig ? same + 1 : 0;
        last_sig = sig;
        if (same > 200000) {
            char t[120];
            snprintf(t, sizeof t, "fight stuck: screen %d st %d cur %d menu [%s%s]", (int)screen, (int)fg->st, fg->cur, menu.prompt, menu.s);
            fc::bad("STUCK", t);
            return false;
        }
        if (it % 50) continue;
        if (screen != Screen::Fight) {
            if (screen == Screen::Cast) {
                if (cr.stage == CastRun::Whom) {
                    play::tap((cr.on_camp ? 17 : 1) * 8 + 2, (4 + 1) * 8 + 2, C);
                    tap_word('S');
                } else if (cr.stage == CastRun::Flame) tap_word(fpick(2) ? 'H' : 'C');
                else if (cr.stage == CastRun::FlameAbort) tap_word('N');
                else play::back(C);
            } else {
                play::back(C);
            }
            continue;
        }
        switch (fg->st) {
        case FSt::Menu:
            moves_made = aim_tries = 0;
            party_turn();
            break;
        case FSt::Move:
            if (moves_made++ < 1 + fpick(4)) play::fight_dir(fpick(8), C);
            else if (hl()) tap_word('D');
            else play::tap(4, text::kMenuRow * 8 + 2, C);
            break;
        case FSt::Aim:
            ++aim_tries;
            if (aim_tries > 30) tap_word('E');
            else if (fpick(3) == 0) tap_word('N');
            else if (!tap_word('T')) tap_word('N');
            break;
        case FSt::DoneMenu:
            if (!tap_word('G') && !tap_word('E')) play::back(C);
            break;
        case FSt::Speed:
            if (!tap_word('E')) tap_item(menu.count - 1);       // (its Exit: Esc isn't the Play Test's)
            break;
        case FSt::AbortAsk: tap_word(fpick(2) ? 'Y' : 'N'); break;
        case FSt::ContinueAsk: tap_word('N'); break;
        case FSt::FlameAsk: tap_word(fpick(2) ? 'H' : 'C'); break;
        case FSt::FlameAbort: tap_word('N'); break;
        case FSt::AllyAsk:
            if (fpick(2)) {
                fc::ally_attacked = true;
                tap_word('Y');
            } else {
                tap_word('N');
            }
            break;
        case FSt::FleeAsk: tap_word(fpick(2) ? 'Y' : 'N'); break;
        case FSt::UseAim: play::back(C); break;
        case FSt::Results:
        case FSt::Destroyed: play::tap(2, 2, C); break;
        default: break;
        }
    }
    (void)k;
    return true;
}

} // namespace ff

static void sweep_fightfuzz(unsigned seed, int fights)
{
    using namespace play;
    fc::install();
    fc::after_turn_start = [](combat::Battle& b, int cur) { if (play::ml) ff::stir(b, cur); };
    ff::list_monsters();
    printf("  %zu monsters\n", ff::mons.size());
    if (ff::mons.empty()) return;
    const int one = getenv("FUZZONE") ? atoi(getenv("FUZZONE")) : -1;
    int ended = 0, run = 0, skipped = 0;
    std::map<std::string, int> before;
    for (int k = 0; k < fights; ++k) {
        if (one >= 0 && k != one) continue;
        const unsigned fs = seed * 7919u + static_cast<unsigned>(k);
        ff::fr.seed(fs);
        rnd.seed(fs);
        g_now = 1000;
        if (!monkey_boot()) {
            problem("boot");
            break;
        }
        const ff::Mon m = ff::mons[ff::fpick(static_cast<int>(ff::mons.size()))];
        const int copies = 1 + ff::fpick(6);
        play::rng = create::Dice(fs);
        char t[120];
        snprintf(t, sizeof t, "seed %u fight %d (FUZZONE=%d) MON%d #%d x%d", seed, k, k, m.area, m.id, copies);
        fc::ctx = t;
        ff::steps.clear();
        if (!start_fight(m.area, m.id, copies)) {
            ++skipped;
            continue;
        }
        ++run;
        before = fc::broken;
        const bool ok = ff::fight(k);
        if (ok) {
            ++ended;
            pump(200000);          // (the treasure, the script after it)
        }
        if (fc::broken != before || getenv("FUZZLOG"))
            printf("  fight %d: %s - %s; steps:%s\n", k, t, ok ? "ended" : "NOT ENDED", ff::steps.substr(0, 600).c_str());
        if (k % 100 == 0) printf("  ... %d fights (%d run, %d ended)\n", k, run, ended);
    }
    printf("== fuzz: %d fights run (%d skipped: no monster), %d ended, %d statuses put on\n", run, skipped, ended, ff::fxs_put);
    fc::report();
}
