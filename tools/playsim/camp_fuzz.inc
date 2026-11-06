// The camp fuzzer (rule tests, 2026-11-01; sweep's `campfuzz <seed> <n>`):
// the party camps and does random camp things - rest of random lengths,
// memorize, scribe, cast, Fix, View, Alter, items (ready, use, trade,
// drop, halve, join), the effects list - n actions, leaving and making
// camp again now and then; a fight a rest runs into is played Quick. After
// every action the party checker (P1-P7) and the camp's own rules:
//   CF1 money never rises in camp (nothing there pays the party)
//   CF2 items never multiply: the units (a pile's count, else 1) never rise
//   CF3 back at the camp's menu every member's carried weight is their
//       items' and coins' (W1's rule) - nothing left stale
// Breaks print "!! CAMP CFn" (counted in `problems`). Each run saves over
// the slots: its own copy of the game folder (the monkey's SAVE.MONKEY
// way of starting again).

namespace cfz {

using namespace play;
std::map<std::string, int> broken;

char ring[64][120];
int ri = 0;
void note_act(const char* what)
{
    snprintf(ring[ri++ % 64], sizeof ring[0], "%s: screen %d vf %d tm %d vault %d tr %d camping %d menu [%s%s]", what, (int)screen, (int)view_from, take_mode, (int)vault_on(), (int)treasure_on(), (int)camping, menu.prompt,
             menu.s);
}
void bad(const char* rule, const std::string& what)
{
    const int n = ++broken[rule];
    ++problems;
    if (n <= 6) printf("!! CAMP %s: %s [screen %d menu %s%s]\n", rule, what.c_str(), (int)screen, menu.prompt, menu.s);
    if (n <= 2 && getenv("CFRING"))
        for (int k = 0; k < 64; ++k) printf("    %s\n", ring[(ri + k) % 64]);
}

long party_worth()
{
    long w = 0;
    for (int i = 0; i < pt->count; ++i) {
        const party::Character& c = pt->m[i];
        for (int m = 0; m < 7; ++m) w += static_cast<long>(c.money(m)) * (m < 5 ? (m == 0 ? 1 : m == 1 ? 10 : m == 2 ? 100 : m == 3 ? 200 : 1000) : 5000);
    }
    return w;
}
long units()
{
    long n = 0;
    for (int i = 0; i < pt->count; ++i)
        for (int k = 0; k < pt->m[i].n_items; ++k) n += pt->m[i].items[k][0x39] ? pt->m[i].items[k][0x39] : 1;
    return n;
}
int weight_of(const party::Character& c)
{
    long w = 0;
    for (int k = 0; k < c.n_items; ++k) {
        const uint8_t* it = c.items[k];
        const int wt = it[0x37] | it[0x38] << 8, cnt = it[0x39];
        w += cnt > 0 ? (wt * cnt) & 0xFFFF : wt;
    }
    for (int m = 0; m < 7; ++m) w += c.money(m);
    return static_cast<int>(w & 0xFFFF);
}

void run(unsigned seed, int rounds)
{
    rnd.seed(seed);
    struct stat st;
    const bool own = stat((game_dir + "/SAVE").c_str(), &st) != 0;
    const std::string save = own ? game_dir : game_dir + "/SAVE", keep = own ? game_dir + ".MONKEY" : game_dir + "/SAVE.MONKEY";
    if (system(("rm -rf '" + keep + "' && cp -r '" + save + "' '" + keep + "'").c_str()) != 0) return;
    const std::string back_up = "rm -rf '" + save + "' && cp -r '" + keep + "' '" + save + "'";
    if (!monkey_boot()) {
        problem("campfuzz: the game doesn't boot");
        return;
    }
    if (CYD_CHEATS && getenv("CFSANCT")) opts.cheats = true;
    int boots = 0, camps = 0, rests = 0, actions = 0;
    long worth0 = -1, units0 = -1;
    bool was_camping = false;
    std::map<int, int> screens;
    for (int r = 0; r < rounds; ++r) {
        if (!d || screen == Screen::Title || !pt || pt->count < 1) {
            if (++boots > 100 || system(back_up.c_str()) != 0 || !monkey_boot()) break;
            continue;
        }
        if (fg) {                       // (a rest ran into a fight: Quick)
            pump(60000);
            worth0 = units0 = -1;
            continue;
        }
        if (!camping) {
            was_camping = false;
            if (walking()) {
                if (getenv("CFSANCT") && cheats_ready()) cheat_open(2, C);
                else if (!tap_word('E') && !tap_word('C')) tap_item(pick(menu.count));
                ticks(5);
                if (camping) ++camps;
            } else {
                // (a script's text, a menu outside the camp: answered at random)
                if (menu.count) tap_item(pick(menu.count));
                else if (input_mode != Input::None) play::input_key('\n', C);
                else play::back(C);
                ticks(20);
            }
            continue;
        }
        if (!was_camping) {
            was_camping = true;
            worth0 = party_worth();
            units0 = units();
        }
        ++actions;
        ++screens[(int)screen];
        note_act("before");
        const int a = pick(100);
        const Screen sc0 = screen;
        const bool wt0 = waiting, ran0 = resume_after_note, cfs0 = camp_from_script;
        const int th0 = (int)then;
        const uint32_t nu0 = note_until;
        if (screen == Screen::Rest && !rest.running && pick(3) == 0) {
            // a rest of random length: days / hours / minutes up, then Rest
            for (int k = pick(6); k > 0; --k) {
                const char unit = "DHM"[pick(3)];
                tap_word(unit);
                if (!tap_word('A')) tap_word('I');      // (Pool's "Inc")
            }
            if (tap_word('R')) ++rests;
        } else if (a < 45 && menu.count > 0) {
            tap_item(pick(menu.count));
        } else if (a < 62) {
            play::tap(8 + pick(38) * 8, 8 + pick(22) * 8, C);        // a list's line, the party, a spell
        } else if (a < 72) {
            static const play::Nav navs[] = {play::Nav::Up, play::Nav::Down, play::Nav::Left, play::Nav::Right};
            play::nav(navs[pick(4)], C);
        } else if (a < 78) {
            int x, y;
            if (play::nav_point(&x, &y)) play::tap(x, y, C);
        } else if (a < 83) {
            play::back(C);
        } else if (a < 90 && input_mode != Input::None) {
            play::input_key(pick(3) ? static_cast<char>('0' + pick(10)) : '\n', C);
        } else {
            ticks(60);
        }
        {
            char k[24];
            snprintf(k, sizeof k, "after a=%d", a);
            note_act(k);
            if (getenv("CFRING") && sc0 == Screen::Camp && screen == Screen::Shop)
                printf("  [camp -> shop: a=%d waiting %d then %d resume_after_note %d camp_from_script %d note_until %u; now: waiting %d then %d]\n", a,
                       (int)wt0, th0, (int)ran0, (int)cfs0, (unsigned)nu0, (int)waiting, (int)then);
        }
        ticks(5);
        if (!d || !pt) continue;
        pcheck::check();
        if (!camping || fg) continue;
        const long w = party_worth(), u = units();
        if (worth0 >= 0 && w > worth0) {
            char t[96];
            snprintf(t, sizeof t, "the party's money %ld -> %ld", worth0, w);
            bad("CF1", t);
        }
        if (units0 >= 0 && u > units0) {
            char t[96];
            snprintf(t, sizeof t, "item units %ld -> %ld", units0, u);
            bad("CF2", t);
        }
        worth0 = w;
        units0 = u;
        if (screen == Screen::Camp)
            for (int i = 0; i < pt->count; ++i) {
                const party::Character& c = pt->m[i];
                if (c.encumbrance() != weight_of(c)) {
                    char nm[16], t[120];
                    c.name(nm, sizeof nm);
                    snprintf(t, sizeof t, "%s carries %d, the items and coins weigh %d", nm, c.encumbrance(), weight_of(c));
                    bad("CF3", t);
                }
            }
    }
    printf("  campfuzz: %d actions in camp, %d camps, %d rests, %d boots; screens:", actions, camps, rests, boots);
    for (const auto& kv : screens) printf(" %d:%d", kv.first, kv.second);
    printf("\n== camp checks:");
    if (broken.empty()) printf(" no rule broken\n");
    else {
        for (const auto& kv : broken) printf(" %s x%d", kv.first.c_str(), kv.second);
        printf("\n");
    }
    pcheck::report();
}

} // namespace cfz
