// The waits and the frames within an attack (Tom, 2026-11-06: speed 0 was
// waited as 4, a sweep's blows resolved all at once, a big monster's skull
// on one of its squares - every earlier test was blind to them). With
// FXCHECK=1 the fight fuzzer (fightfuzz, -DCYD_TEST_HOOKS) records the
// fight's own events (play_fight.inc CYD_TEST_FX: a page shown, a
// picture's frame, a page's wait, the attack's pause, the field put back,
// a sweep's blow, Quick) with the playsim's clock (FUZZTICK=1: 1 ms ticks)
// and checks them against the originals' rules (SPEC 12 "Game speed",
// combat_fx_facts.md) at the game speed FXSPEED (0-9):
//   W1 a skull frame lasts 10 ms, a burst's 70, a flight's step its own
//      delay (the spell's / missile's flight), measured frame to frame
//   W2 the attack's pause before its first page: 10 ms + 10 a speed step
//      (ours, SPEC 12; the originals 100 always)
//   W3 a page's wait before the next page: 2 game delays (speed x 100 ms,
//      Pool x 225; speed 0 none)
//   W4 Quick's wait: 200 ms
//   F1 a skull blinks 9 frames, each over every square of the fallen's
//      footprint (a 2 x 2 monster: 4)
//   F2 the field is put back (the fallen's icon gone) after its skull's
//      last frame and before the next page or blow; the fallen is down then
//   F3 a sweep's next blow only after the pages of the one before (hit,
//      fall, skull, the icon gone) - none still queued
// W5, at speeds 0, 4, 9 at a fight's start: a letter (speed x 3 ms, Pool
// x 10), the game delay - messages, notes (speed x 100, Pool x 225),
// sparkles' rounds (speed + 1). Not checked: ECL DELAY (the VM's own
// game_delay, private - the same formula), the event pictures' frames
// (their delays x 100 ms at every speed: no rule of the originals' found).

namespace fxc {

struct Ev {
    play::FxEv ev;
    uint32_t at, wait;
    int n, kind, who, step;
};
std::vector<Ev> evs;
std::map<std::string, int> broken;
std::map<std::string, long> seen;           // what was measured (counts)
int speed = 4;
bool pool = false;
std::string ctx;
bool on = false;                            // (inside a fuzzed fight: the clock at FUZZTICK)

void fx_hook_base_mark() {}
void bad(const char* rule, const std::string& what)
{
    if (broken[rule]++ < 6) printf("!! FX %s %s: %s\n", rule, ctx.c_str(), what.c_str());
}

uint32_t delay_unit() { return pool ? 225u : 100u; }

// the fallen's size when last seen standing (a skull's footprint)
int last_size[combat::kMaxFighters];

int skull_who = -1, skull_frames = 0;
bool skull_open = false;                    // a skull drawn, the field not put back yet
uint32_t pause_at = 0, wait_at = 0, wait_ms = 0;
bool pause_on = false, wait_on = false;

void hook(play::FxEv ev, uint32_t wait, int n)
{
    using namespace play;
    if (getenv("FXLOG")) {
        static const char* const kNames[] = {"Page", "Frame", "PageWait", "AttackPause", "FieldBack", "SweepBlow", "Quick", "RollsClear"};
        std::string line;
        if (ev == FxEv::Page && fg && n >= 0 && n < fg->pages) {
            const Page& p = fg->page[n];
            for (int k = 0; k < p.lines; ++k) line += std::string(fg->ptext + p.line_at[k]) + " / ";
        }
        if (const char* dir = getenv("FXSHOT")) {
            // (the canvas at each event of a sweep and after: FXSHOT=dir)
            static int shot = 0, left = 0;
            static int sweeps = 0;
            if (ev == FxEv::SweepBlow && n == 1 && ++sweeps == (getenv("FXSHOTAT") ? atoi(getenv("FXSHOTAT")) : 1)) left = 400;
            if (left > 0 && ev != FxEv::RollsClear) {
                --left;
                char fn[256];
                snprintf(fn, sizeof fn, "%s/fx_%04d_%s.pgm", dir, shot++, kNames[static_cast<int>(ev)]);
                if (FILE* f = fopen(fn, "wb")) {
                    fprintf(f, "P5 320 200 15\n");
                    for (int y = 0; y < 200; ++y)
                        for (int x = 0; x < 320; ++x) fputc(C.get(x, y), f);
                    fclose(f);
                }
            }
        }
        printf("  fx %u %s wait %u n %d snd %d %s\n", static_cast<unsigned>(g_now), kNames[static_cast<int>(ev)], static_cast<unsigned>(wait), n, (ev == FxEv::Page && fg && n >= 0 && n < fg->pages) ? fg->page[n].sound : -1, line.c_str());
    }
    if (!fg || !on) return;
    for (int i = 0; i < fg->b.n && i < combat::kMaxFighters; ++i)
        if (fg->b.f[i].up() && fg->b.f[i].size) last_size[i] = fg->b.f[i].size;
    Ev e{ev, g_now, wait, n, fg->fx.kind, fg->fx.who, fg->fx_step - 1};
    char t[160];
    // E1 (2026-11-13: the Dark Queen's breaths had empty words): every page
    // says something besides the names - a word left unread shows nothing
    if (ev == FxEv::Page && n >= 0 && n < fg->pages) {
        const Page& p = fg->page[n];
        bool said = false;
        for (int k = 0; k < p.lines && !said; ++k)
            if (!(p.names & (1u << k)) && fg->ptext[p.line_at[k]]) said = true;
        ++seen["E1 page words"];
        if (!said && p.lines > 0) {
            std::string line;
            for (int k = 0; k < p.lines; ++k) line += std::string(fg->ptext + p.line_at[k]) + " / ";
            snprintf(t, sizeof t, "a page of names only (an empty word? made at %p, base %p, Ev %d, word %d): %s", play::g_test_empty_at, (void*)&fx_hook_base_mark, play::g_test_ev, play::g_test_word, line.c_str());
            bad("E1", t);
        }
    }
    if (ev == FxEv::Page) {
        if (pause_on) {
            ++seen["W2 attack pause"];
            const uint32_t want = 10 + 10 * static_cast<uint32_t>(speed);
            if (g_now - pause_at != want) {
                snprintf(t, sizeof t, "the attack's pause %u ms, the rule %u", g_now - pause_at, want);
                bad("W2", t);
            }
            pause_on = false;
        }
        if (wait_on && n > 0) {
            ++seen["W3 page wait"];
            const uint32_t want = 2u * static_cast<uint32_t>(speed) * delay_unit();
            const uint32_t got = g_now - wait_at;
            // (a tick's slack: the page shows on the first tick past its time, at least 1 ms)
            if (got != want && !(want == 0 && got <= 1)) {
                snprintf(t, sizeof t, "page %d's wait %u ms (set %u), the rule %u", n, got, wait_ms, want);
                bad("W3", t);
            }
        }
        wait_on = false;
        if (skull_open) bad("F2", "a page shown with the last one's skull still on the field");
    }
    if (ev == FxEv::Frame) {
        if (!evs.empty() && evs.back().ev == FxEv::Frame && evs.back().kind == e.kind && e.step > 0) {
            const uint32_t got = g_now - evs.back().at;
            const uint32_t want = evs.back().wait;
            const char* what = e.kind == Fx::Skull ? "W1 skull" : e.kind == Fx::Burst ? "W1 burst" : "W1 flight";
            ++seen[what];
            const uint32_t rule = e.kind == Fx::Skull ? 10 : e.kind == Fx::Burst ? 70 : want;
            if (got != rule) {
                snprintf(t, sizeof t, "%s frame %d after %u ms, the rule %u", what + 3, e.step, got, rule);
                bad("W1", t);
            }
        }
        if (e.kind == Fx::Skull) {
            if (e.step == 0) {
                skull_who = e.who;
                skull_frames = 0;
            }
            ++skull_frames;
            skull_open = true;
            const int who = e.who;
            const int sz = who >= 0 && who < combat::kMaxFighters ? last_size[who] : 1;
            int sx[4], sy[4];
            const int want = combat::squares(sz ? sz : 1, sx, sy);
            ++seen[sz > 1 ? "F1 skull (big)" : "F1 skull"];
            if (n != want) {
                snprintf(t, sizeof t, "skull frame %d on %d squares, fighter %d's footprint (size %d) %d", e.step, n, who, sz, want);
                bad("F1", t);
            }
        }
    }
    if (ev == FxEv::PageWait) {
        if (skull_who >= 0 && skull_frames && skull_frames != 9) {
            snprintf(t, sizeof t, "fighter %d's skull blinked %d frames, the rule 9", skull_who, skull_frames);
            bad("F1", t);
        }
        skull_who = -1;
        skull_frames = 0;
        wait_on = true;
        wait_at = g_now;
        wait_ms = wait;
    }
    if (ev == FxEv::AttackPause) {
        pause_on = true;
        pause_at = g_now;
    }
    if (ev == FxEv::FieldBack) {
        ++seen["F2 field back"];
        if (skull_open && skull_who >= 0 && skull_who < fg->b.n && fg->b.f[skull_who].up()) {
            snprintf(t, sizeof t, "the field put back with fighter %d (its skull shown) standing", skull_who);
            bad("F2", t);
        }
        skull_open = false;
    }
    if (ev == FxEv::SweepBlow) {
        ++seen["F3 sweep blow"];
        if (n > 1 && (fg->pages != 0 || fg->fx_on)) {
            snprintf(t, sizeof t, "blow %d with %d pages queued", n, fg->pages);
            bad("F3", t);
        }
        if (n > 1 && skull_open) bad("F3", "a blow with the last one's skull still on the field");
    }
    // K8 Show Rolls (Tom, 2026-10-31: every blow's roll on its page): an
    // attack's rolls let go with a to-hit roll no page took
    if (ev == FxEv::RollsClear && opts.rolls) {
        ++seen["K8 rolls"];
        if (n > 0 && !wait) {
            snprintf(t, sizeof t, "%d blow(s) whose to-hit roll no page showed", n);
            bad("K8", t);
        }
    }
    // K9 the Battle view (CYD_COMPANION_VIEWS): a dot for each fighter on the
    // field, where it stands, its side
    if (ev == FxEv::Page) {
        PanelField pf;
        if (panel_field(pf)) {
            ++seen["K9 battle view"];
            if (pf.n != fg->b.n) bad("K9", "the Battle view's fighters aren't the fight's");
            for (int i = 0; i < fg->b.n && i < pf.n; ++i) {
                const combat::Fighter& f = fg->b.f[i];
                PanelDot dot;
                const bool on = panel_dot(i, dot);
                if (f.up() && f.size && (!on || dot.x != f.x || dot.y != f.y)) {
                    snprintf(t, sizeof t, "fighter %d at %d,%d: its dot %s at %d,%d", i, f.x, f.y, on ? "" : "missing", dot.x, dot.y);
                    bad("K9", t);
                    break;
                }
            }
        }
    }
    if (ev == FxEv::Quick) {
        ++seen["W4 quick"];
        if (wait != 200) bad("W4", "Quick's wait isn't 200 ms");
    }
    evs.push_back(e);
    if (evs.size() > 64) evs.erase(evs.begin(), evs.begin() + 32);
}

// a fight begins: the state cleared
void begin(const char* c)
{
    ctx = c;
    on = true;
    evs.clear();
    skull_who = -1;
    skull_frames = 0;
    skull_open = pause_on = wait_on = false;
    memset(last_size, 0, sizeof last_size);
    pool = play::pool_files();
}

// The waits outside a fight, from each program's rules
void rules()
{
    using namespace play;
    for (int s : {0, 4, 9}) {
        vm->set(0x4BFC, static_cast<uint16_t>(s));
        const uint32_t unit = pool_files() ? 225 : 100, letter = pool_files() ? 10 : 3;
        char t[120];
        if (char_ms() != static_cast<uint32_t>(s) * letter) {
            snprintf(t, sizeof t, "speed %d: a letter %u ms, the rule %u", s, char_ms(), s * letter);
            bad("W5", t);
        }
        if (game_delay_ms() != static_cast<uint32_t>(s) * unit) {
            snprintf(t, sizeof t, "speed %d: the game delay %u ms, the rule %u", s, game_delay_ms(), s * unit);
            bad("W5", t);
        }
        // (a sparkle's rounds: the game speed + 1)
        if (fg) {
            const Fx e = hit_fx(0, true);
            if (e.loops != s + 1) {
                snprintf(t, sizeof t, "speed %d: sparkles %d rounds, the rule %d", s, e.loops, s + 1);
                bad("W5", t);
            }
        }
        ++seen["W5 rules (letter, game delay, sparkles)"];
    }
    vm->set(0x4BFC, static_cast<uint16_t>(speed));
}

void install()
{
    play::g_fx_hook = hook;
}

void report()
{
    printf("== fxcheck (speed %d):", speed);
    for (auto& s : seen) printf(" %s %ld;", s.first.c_str(), s.second);
    printf("\n");
    int n = 0;
    for (auto& b : broken) {
        printf("  %s broken %d times\n", b.first.c_str(), b.second);
        n += b.second;
    }
    if (!n) printf("  no rule broken\n");
    problems += n;
}

} // namespace fxc
