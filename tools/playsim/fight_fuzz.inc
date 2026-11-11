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
    if (getenv("FXLOG")) {
        printf("  menu [%s] at", menu.s);
        for (int k = 0; k < menu.count; ++k) {
            int col = 0, row = 0;
            text::item_cell(menu, k, &col, &row);
            printf(" %d,%d", col * 8 + 2, row * 8 + 2);
        }
        printf("\n");
    }
    char t[24];
    snprintf(t, sizeof t, " r%d:%d:", fg->b.round, fg->cur);
    steps += t;
    if (getenv("FUZZMOVEATK")) {        // (Tom's way: Move, then a step into the foe)
        steps += "M";
        if (!tap_word('M')) tap_item(0);
        return;
    }
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
    // (FUZZTICK=ms: the clock's step - 1 for the fight's waits measured to the ms)
    static const uint32_t step = getenv("FUZZTICK") ? static_cast<uint32_t>(atoi(getenv("FUZZTICK"))) : 20;
    const int every = step >= 20 ? 50 : static_cast<int>(50 * 20 / step);
    while (fg || screen == Screen::Fight) {
        if (++it > 3000000L * (step >= 20 ? 1 : 20 / step)) {
            fc::bad("R10", "the fuzzer's tick limit (" + steps.substr(0, 200) + ")");
            return false;
        }
        g_now += step;
        play::tick(g_now, C);
        fc::tick();
        if (!d) return false;
        if (!fg) break;
        if (fg->b.round > fc::fight_cap + 5 && fc::endless_told) return false;
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
        if (it % every) continue;
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
            if (getenv("FUZZMOVEATK")) {
                // (toward the nearest standing foe; a step into it attacks)
                const combat::Battle& b = fg->b;
                const combat::Fighter& f = b.f[fg->cur];
                int best = -1, bd = 1 << 30;
                for (int i = 0; i < b.n; ++i)
                    if (b.f[i].team() != f.team() && b.f[i].up() && b.f[i].size) {
                        const int dd = (b.f[i].x - f.x) * (b.f[i].x - f.x) + (b.f[i].y - f.y) * (b.f[i].y - f.y);
                        if (dd < bd) bd = dd, best = i;
                    }
                if (best < 0 || moves_made++ > 12) {
                    play::tap(4, text::kMenuRow * 8 + 2, C);
                    break;
                }
                const int dx = (b.f[best].x > f.x) - (b.f[best].x < f.x), dy = (b.f[best].y > f.y) - (b.f[best].y < f.y);
                static const int kWay[3][3] = {{7, 0, 1}, {6, -1, 2}, {5, 4, 3}};
                const int w = kWay[dy + 1][dx + 1];
                if (w < 0) play::tap(4, text::kMenuRow * 8 + 2, C);
                else play::fight_dir(w, C);
                break;
            }
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
        case FSt::NpcShare:
        case FSt::More:
        case FSt::Results:
        case FSt::Destroyed: play::tap(2, 2, C); break;
        case FSt::Pages:
        case FSt::Ai:
            // FUZZTAPQ=n (Tom, 2026-11-10): one pass in n a tap on the field
            // while pages show or the computer plays - it takes the party off
            // Quick and skips nothing (fight_check R14: no computer turn for
            // them after; here: the page's wait and the pictures untouched)
            if (const char* tq = getenv("FUZZTAPQ"); tq && fpick(atoi(tq) > 0 ? atoi(tq) : 50) == 0) {
                const uint32_t until = fg->until;
                const bool fx_on = fg->fx_on;
                const int pages = fg->pages, at = fg->at;
                const FSt st = fg->st;
                play::tap(100, 60, C);
                ++fc::tapq_seen;
                if (fg && st == FSt::Pages && (fg->until != until || fg->fx_on != fx_on || fg->pages != pages || fg->at != at))
                    fc::bad("R14", "a tap during the pages moved them on");
                if (fg)
                    for (int i = 0; i < fg->b.party_size && i < fg->b.n; ++i) {
                        const combat::Fighter& o = fg->b.f[i];
                        if (o.quick && o.member >= 0 && !o.team() && o.rec[0xF7] < 0x80 && o.status() != party::Animated && !o.fleeing)
                            fc::bad("R14", "a tap left one on Quick: " + fc::who(fg->b, i));
                    }
            }
            break;
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
    if (getenv("FXCHECK")) {
        fxc::install();
        fxc::speed = getenv("FXSPEED") ? atoi(getenv("FXSPEED")) : 4;
    }
    fc::after_turn_start = [](combat::Battle& b, int cur) {
        if (play::ml) ff::stir(b, cur);
        // FUZZSWEEP=1: the monsters under 1 hit die (record 0xE5 = 0), so a
        // fighter's blow sweeps them (fx_check F3: a sweep's frames in order)
        if (getenv("FUZZSWEEP"))
            for (int i = b.party_size; i < b.n; ++i) b.f[i].rec[0xE5] = 0;
        // (FUZZSWEEPLV=n: the party's attack level n - a level 1 party sweeps nothing)
        if (getenv("FUZZSWEEPLV"))
            for (int i = 0; i < b.party_size; ++i) b.f[i].rec[0xDD] = static_cast<uint8_t>(atoi(getenv("FUZZSWEEPLV")));
    };
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
            // (why: the title again - the party lost a fight the save's own
            // script starts at once, Death Knights' save A outdoors - or the
            // party menu / no game; a lost boot fight is the dice, not a rule:
            // that fight is passed over - Death Knights seed 2 fight 31, 2026-11-09)
            if (play::d && play::pt && !play::pt->count && !play::fg && play::screen == play::Screen::PartyMenu) {
                printf("  (fight %d: the save's own opening fight lost while booting - passed over)\n", k);
                ++skipped;
                continue;
            }
            char t[120];
            snprintf(t, sizeof t, "boot (screen %d, game %d, party %d, a fight %d)", static_cast<int>(play::screen), play::d != nullptr,
                     play::pt ? play::pt->count : -1, play::fg != nullptr);
            problem(t);
            break;
        }
        if (getenv("FXLOG")) printf("  the save's speed %d\n", vm ? vm->get(0x4BFC) & 0xFF : -1);
        const ff::Mon m = ff::mons[ff::fpick(static_cast<int>(ff::mons.size()))];
        const int copies = 1 + ff::fpick(6);
        play::rng = create::Dice(fs);
        if (getenv("FXCHECK")) vm->set(0x4BFC, static_cast<uint16_t>(fxc::speed));     // (the speed the waits are checked at)
        char t[120];
        snprintf(t, sizeof t, "seed %u fight %d (FUZZONE=%d) MON%d #%d x%d", seed, k, k, m.area, m.id, copies);
        fc::ctx = t;
        ff::steps.clear();
        // (FUZZOWN=1: the saved game's own fight, its monsters as they are - Pool's goblins: sweeps)
        // (FUZZWALK=1: no fight made - the party walks where the save stands
        // until the game's own encounter starts one: Tom's Podol Plaza goblins)
        auto walk_to_fight = [&]() {
            static const play::Act acts[] = {play::Act::Forward, play::Act::Forward, play::Act::Forward,
                                             play::Act::TurnLeft, play::Act::TurnRight};
            for (int st = 0; st < 3000 && !play::fg; ++st) {
                if (play::walking()) play::act(acts[ff::fpick(5)], C);
                else if (play::menu.count > 0) tap_item(ff::fpick(play::menu.count));
                else play::tap(2, 2, C);
                for (int t = 0; t < 10 && !play::fg; ++t) {
                    g_now += 20;
                    play::tick(g_now, C);
                }
            }
            return play::fg != nullptr;
        };
        if (getenv("FUZZWALK") ? !walk_to_fight() : getenv("FUZZOWN") ? !start_fight() : !start_fight(m.area, m.id, copies)) {
            ++skipped;
            continue;
        }
        ++run;
        // Quick Actions (Tom, 2026-11-09): QAOFF=1 the switch off; QAFUZZ=1
        // each party member's eight switches at random this fight (R13 checks them)
        // QANPC=1 (Tom, 2026-11-09: the party's NPCs on the Quick tab too):
        // the party's last member made an NPC (control byte 0x80 on) for the
        // fights - its switches kComputer as it starts, at random under QAFUZZ
        if (getenv("QANPC") && play::fg && play::pt && play::pt->count > 1) {
            const int last = play::pt->count - 1;
            play::pt->m[last].rec[0xF7] = static_cast<uint8_t>(0x80 | 50);
            for (int i = 0; i < play::fg->b.party_size; ++i)
                if (play::fg->b.f[i].member == last) play::fg->b.f[i].rec[0xF7] = static_cast<uint8_t>(0x80 | 50);
            play::qa_mask[last] = quickacts::kComputer;
        }
        play::opts.quick_acts = !getenv("QAOFF");
        if (getenv("QAFUZZ")) {
            std::string g = " grid";
            for (int i = 0; i < play::pt->count && i < party::kMaxParty; ++i) {
                play::qa_mask[i] = static_cast<uint8_t>(ff::fr() & 0xFF);
                char h[8];
                snprintf(h, sizeof h, " %02X", play::qa_mask[i]);
                g += h;
            }
            // (every other fight the whole party on Quick from the start)
            if (ff::fpick(2) && play::fg) {
                for (int i = 0; i < play::fg->b.party_size; ++i) play::fg->b.f[i].quick = true;
                g += " all-quick";
            }
            ff::steps += g;
        }
        before = fc::broken;
        if (getenv("FXCHECK")) {
            fxc::begin(t);
            if (vm->get(0x4BFC) != fxc::speed) fxc::bad("W0", "the fight began at another speed");
            fxc::rules();
        }
        const bool ok = ff::fight(k);
        fxc::on = false;
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
    if (getenv("FXCHECK")) fxc::report();
}
