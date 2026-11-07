// The cursor-key / tap layer check (sweep mode `navcheck [seed [n]]`;
// Tom, 2026-11-06: the Trade list's keys did nothing, the party menu had
// no highlight at its start, Memorize's highlight jumped - the playsim
// drives by keystrokes and never saw them). The monkey's random play, and
// on every settled screen showing a menu line or a list, the board's own
// way in (play::nav, nav_point, tap_highlight / tap) is checked against the
// rules of CLAUDE.md ("Cursor keys", "Menu highlight", "Tap highlight"):
//   N1 a menu's highlighted word is a white box (black on colour 15) over
//      the whole word, and only that word is lit
//   N2 a list (two or more lines a tap acts on - tap_target) has exactly
//      one line lit, the whole line's text inside the box
//   N3 Left / Right move the menu line's highlight (and back again)
//   N4 Up / Down move a list's lit line (and back again); a line lit that
//      Up / Down can move is a line a tap lights too (tap_target)
//   N5 Select (nav_point) is on the highlight, and a tap there lights
//      what is lit (the tap path acts on the same thing)
//   N6 after Select, the same menu shown again keeps its word lit (a
//      pick's place kept), the same list its line - unless the word was
//      Exit / Next / Prev (or the list changed under it)
// Reached: the monkey's screens (NCHEAT=n: n in 100 exploring turns open a
// Cheats tab screen; MCAMP biases to the camp), the party menu at boot.
// A failure is printed once per screen signature and rule.
// Modify Character has its own rules (M1-M5, probe_modify below); the
// monkey goes into it from a hall's party menu with NMODIFY=n (n in 100).
// Not covered: the Dark Queen's buttons (hl(): no pixel rules N1 / N2,
// the rest yes), the icon editor / gallery, the on-screen keyboard.

namespace nvc {

int probes = 0, fails = 0;
int entered = 0, opened = 0;                // Modify: tries from a hall's party menu, opened
bool no_select = false;                     // (the boot's probe: no Select)
std::map<std::string, int> failed;          // signature + rule: times
std::map<int, int> screens;                 // screen: probes
std::set<std::string> sigs;

// A text cell drawn black on white (the highlight's box): only colours 0
// and 15, mostly 15, its spacing row / column white
bool lit_cell(int col, int row)
{
    if (col < 0 || col >= 40 || row < 0 || row >= 25) return false;
    int white = 0, edge = 0;
    for (int j = 0; j < 8; ++j)
        for (int i = 0; i < 8; ++i) {
            const uint8_t v = C.get(col * 8 + i, row * 8 + j);
            if (v != 0 && v != 15) return false;
            if (v == 15) {
                ++white;
                if (j == 0 || j == 7 || i == 0 || i == 7) ++edge;
            }
        }
    // (a bold white letter on black reaches 36 but its rim stays under 13 of 28)
    return white >= 24 && edge >= 15;
}
bool ink_cell(int col, int row)
{
    for (int j = 0; j < 8; ++j)
        for (int i = 0; i < 8; ++i)
            if (C.get(col * 8 + i, row * 8 + j)) return true;
    return false;
}

struct Run {
    int row, c0, c1;
};
std::vector<Run> lit_runs(int r0, int r1)
{
    std::vector<Run> v;
    for (int r = r0; r <= r1; ++r)
        for (int c = 0; c < 40; ++c) {
            if (!lit_cell(c, r)) continue;
            int e = c;
            while (e + 1 < 40 && lit_cell(e + 1, r)) ++e;
            v.push_back({r, c, e});
            c = e;
        }
    return v;
}

int menu_top() { return text::two_rows(play::menu) ? text::kMenuRow - 1 : text::kMenuRow; }

std::string sig()
{
    using namespace play;
    char t[200];
    snprintf(t, sizeof t, "g%d s%d f%d [%.24s|%.60s]", d ? (int)d->prof->game : 0, (int)screen,
             fg ? (int)fg->st : -1, menu_on ? menu.prompt : "", menu_on ? menu.s : "");
    return t;
}

// NAVSHOT=dir: the canvas at each failure as dir/nav<k>.ppm (scratch only - real game screens)
void shot()
{
    const char* dir = getenv("NAVSHOT");
    if (!dir) return;
    static const uint8_t ega[16][3] = {{0, 0, 0},       {0, 0, 170},   {0, 170, 0},   {0, 170, 170},
                                       {170, 0, 0},     {170, 0, 170}, {170, 85, 0},  {170, 170, 170},
                                       {85, 85, 85},    {85, 85, 255}, {85, 255, 85}, {85, 255, 255},
                                       {255, 85, 85},   {255, 85, 255}, {255, 255, 85}, {255, 255, 255}};
    char path[300];
    snprintf(path, sizeof path, "%s/nav%d_%d.ppm", dir, play::d ? (int)play::d->prof->game : 0, fails);
    FILE* f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6 320 200 255\n");
    for (int y = 0; y < 200; ++y)
        for (int x = 0; x < 320; ++x) fwrite(ega[C.get(x, y) & 15], 1, 3, f);
    fclose(f);
}

void fail(const char* rule, const char* detail)
{
    const std::string k = sig() + " " + rule;
    if (failed[k]++) return;
    ++fails;
    printf("!! NAV %s %s: %s (shot %d)\n", rule, sig().c_str(), detail, fails);
    shot();
}

bool settled()
{
    using namespace play;
    if (!d || !pt || input_mode != Input::None || note_until || screen == Screen::Title || screen == Screen::Won ||
        screen == Screen::Icon || walk_pending() || journal_waiting())
        return false;
    if (fg && fg->st != FSt::Menu && fg->st != FSt::Aim && fg->st != FSt::Move && fg->st != FSt::DoneMenu &&
        fg->st != FSt::Speed && fg->st != FSt::AllyAsk && fg->st != FSt::FleeAsk && fg->st != FSt::ContinueAsk &&
        fg->st != FSt::AbortAsk)
        return false;
    if (screen == Screen::Fight && !fg) return false;
    // (a print or pause the game waits on under the last menu line: Select is "press a key")
    if (screen == Screen::Game && waiting && vm->wait() != ecl::Wait::Menu && vm->wait() != ecl::Wait::ListMenu &&
        vm->wait() != ecl::Wait::Who)
        return false;
#if CYD_ICON_GALLERY
    if (icon_gallery()) return false;
#endif
    return true;
}

// The rows (outside the menu line) a tap acts on, as lines of a list
std::vector<Run> tap_rows()
{
    std::vector<Run> v;
    const int top = play::menu_on ? menu_top() : 25;
    for (int r = 0; r < top; ++r) {
        static const int xs[] = {2, 6, 12, 18, 22, 28, 34};
        for (int x : xs) {
            int row = -1, c0 = 0, c1 = 0;
            if (play::tap_target(x * 8 + 2, r * 8 + 2, &row, &c0, &c1) && row == r) {
                v.push_back({r, c0, c1});
                break;
            }
        }
    }
    return v;
}

void settle()
{
    for (int k = 0; k < 40 && play::d; ++k) {
        g_now += 50;
        play::tick(g_now, C);
    }
}

// The board's Select: the highlight lit, the tap, the highlight put back
void select_at(int x, int y)
{
    int y0, y1;
    if (play::tap_highlight(x, y, C, &y0, &y1)) play::tap_highlight_blink(C);
    play::tap(x, y, C);
    if (play::d) play::tap_highlight_end(C);
}

bool menu_word_lit(int k, bool* whole)
{
    int col = 0, row = text::kMenuRow;
    text::item_cell(play::menu, k, &col, &row);
    const int n = play::menu.end[k] - play::menu.start[k] + 1;
    int lit = 0;
    for (int i = 0; i < n; ++i) lit += lit_cell(col + i, row);
    *whole = lit == n;
    return lit > 0;
}

// The list's place in the engine's terms (every list the keys move)
long list_state()
{
    using namespace play;
    long v = pt ? pt->selected : 0;
    v = v * 131 + pm_sel;
    v = v * 131 + plist.index;
    v = v * 131 + plist.top;
    v = v * 131 + sl.sel;
    v = v * 131 + sl.top;
    v = v * 131 + list_sel;
    return v;
}
long list_len()
{
    using namespace play;
    return (pt ? pt->count : 0) * 1000000L + plist.n * 1000L + sl.n;
}

// The lit line: on a list (tap rows given) the row whose own columns hold
// a lit run; otherwise a run of 3+ lit cells one row high (a picture's
// white is lit above or below too); -1: none
int lit_line(const std::vector<Run>& rows)
{
    const std::vector<Run> lit = lit_runs(0, play::menu_on ? menu_top() - 1 : 24);
    for (const Run& l : lit)
        for (const Run& r : rows)
            if (rows.size() >= 2 && l.row == r.row && l.c1 >= r.c0 && l.c0 <= r.c1) return l.row;
    if (rows.size() >= 2) return -1;
    for (const Run& l : lit) {
        if (l.c1 - l.c0 < 2 || l.row < 1 || l.row > 22) continue;
        bool box = true;
        for (int c = l.c0; c <= l.c1 && box; ++c)
            if (lit_cell(c, l.row - 1) || lit_cell(c, l.row + 1)) box = false;
        if (box) return l.row;
    }
    return -1;
}

// ---- Modify Character (Tom, 2026-11-06: its menu line's highlight never
// moved - fixed in v0.173.2, mod_menu). M1 Up / Down reach the menu line
// within 9 presses from any item and come back; M2 there Left / Right move
// the boxed word (N1 / N3); M3 Select there = a tap on the lit word (Keep /
// Exit leave, Add / Subtract / Next / Prev act as their taps); M4 on an
// item Left / Right change its value exactly as modify_step (the games'
// arrows) does, and the item being changed is drawn in the mark colour
// (13; the Dark Queen's 15) - no other item; M5 Select on the name opens
// the name keyboard (Select on another item keeps it).

bool newer_modify() { return play::secret_files() || play::darkness_files() || play::gateway() || play::krynn() || play::hl(); }

// The cells of item i's value (independent of the engine's tap / Select maps)
void mod_cells(int i, int* row, int* c0, int* c1)
{
    const bool nw = newer_modify();
    if (i < 6) {
        *row = (nw ? 9 : 7) + i;
        *c0 = 1;
        *c1 = 11;                           // (Darkness's / the Dark Queen's "*" at 12 left out)
    } else if (i == 6) {
        *row = nw ? 3 : 18;
        *c0 = nw ? 20 : 1;
        *c1 = nw ? 38 : 6;
    } else {
        *row = 1;
        *c0 = 1;
        *c1 = 16;
    }
}
bool mod_marked(int i)
{
    const uint8_t mark = play::hl() ? 15 : 13;
    int row, c0, c1;
    mod_cells(i, &row, &c0, &c1);
    for (int y = row * 8; y < row * 8 + 8; ++y)
        for (int x = c0 * 8; x < c1 * 8 + 8; ++x)
            if (C.get(x, y) == mark) return true;
    return false;
}
void mod_mark_check(const char* when)
{
    char t[120];
    if (!mod_marked(play::mod_item)) {
        snprintf(t, sizeof t, "%s: item %d not drawn in the mark colour", when, play::mod_item);
        fail("M4", t);
    }
    for (int j = 0; j < 8; ++j)
        if (j != play::mod_item && mod_marked(j)) {
            snprintf(t, sizeof t, "%s: item %d marked besides item %d", when, j, play::mod_item);
            fail("M4", t);
            break;
        }
}
bool same_char(const party::Character& a, const party::Character& b)
{
    return !memcmp(a.rec, b.rec, sizeof a.rec) && !memcmp(a.ext, b.ext, sizeof a.ext);
}
bool in_modify() { return play::d && play::screen == play::Screen::Modify && play::modder && play::input_mode == play::Input::None; }

// M4: the item's Left / Right against modify_step from the same start
void mod_arrows()
{
    using namespace play;
    static party::Character a0, by_key;
    char t[160];
    for (int dir = 1; dir >= -1; dir -= 2) {
        party::Character* ch = pt->sel();
        if (!ch || !in_modify() || mod_menu) return;
        a0 = *ch;
        const int item = mod_item;
        if (!nav(dir > 0 ? Nav::Right : Nav::Left, C) || !in_modify()) {
            fail("M4", dir > 0 ? "Right on an item did nothing / left Modify" : "Left on an item did nothing / left Modify");
            return;
        }
        if (mod_item != item || mod_menu) {
            snprintf(t, sizeof t, "%s moved off item %d (now %d, menu %d)", dir > 0 ? "Right" : "Left", item, mod_item, (int)mod_menu);
            fail("M4", t);
            return;
        }
        mod_mark_check(dir > 0 ? "after Right" : "after Left");
        by_key = *pt->sel();
        *pt->sel() = a0;
        modify_step(dir, C);
        if (!same_char(by_key, *pt->sel())) {
            snprintf(t, sizeof t, "%s on item %d: not the arrows' value (stat %d / hp %d by key, %d / %d by the arrow)",
                     dir > 0 ? "Right" : "Left", item, item < 6 ? by_key.stat(item) : 0, by_key.hp(),
                     item < 6 ? pt->sel()->stat(item) : 0, pt->sel()->hp());
            fail("M4", t);
        }
    }
}

// The menu line's checks (N1 / N3 as for every menu; M3's Select point)
void mod_menu_checks(bool pixels)
{
    using namespace play;
    char t[160];
    if (menu.count < 2) return;
    const int sel = menu.selected;
    if (sel < 0 || sel >= menu.count) {
        fail("M2", "no word highlighted on the menu line");
        return;
    }
    bool whole = false;
    if (pixels && (!menu_word_lit(sel, &whole) || !whole)) {
        snprintf(t, sizeof t, "word %d '%.*s' not a whole white box", sel, menu.end[sel] - menu.start[sel] + 1, menu.s + menu.start[sel]);
        fail("M2", t);
    }
    for (int k = 0; pixels && k < menu.count; ++k) {
        bool w;
        if (k != sel && menu_word_lit(k, &w)) {
            snprintf(t, sizeof t, "word %d lit besides %d", k, sel);
            fail("M2", t);
            break;
        }
    }
    static party::Character a0;
    a0 = *pt->sel();
    nav(Nav::Right, C);
    if (!in_modify() || !mod_menu) {
        fail("M2", "Right on the menu line left it");
        return;
    }
    if (menu.selected == sel) {
        snprintf(t, sizeof t, "Right left the highlight on word %d of %d (mod_word %d)", sel, menu.count, mod_word);
        fail("M2", t);
    }
    else if (pixels && (!menu_word_lit(menu.selected, &whole) || !whole)) fail("M2", "the word moved to isn't lit");
    if (!same_char(a0, *pt->sel())) fail("M2", "Right on the menu line changed a value");
    nav(Nav::Left, C);
    if (!in_modify() || menu.selected != sel) {
        snprintf(t, sizeof t, "Left didn't come back (%d -> %d)", sel, in_modify() ? menu.selected : -1);
        fail("M2", t);
        return;
    }
    if (pixels && (!menu_word_lit(sel, &whole) || !whole)) fail("M2", "back on the first word, it isn't lit");
    int x = -1, y = -1;
    if (!nav_point(&x, &y)) {
        fail("M3", "no Select point on the menu line");
        return;
    }
    int row = -1, c0 = 0, c1 = 0;
    int wc = 0, wr = 0;
    text::item_cell(menu, sel, &wc, &wr);
    if (!tap_target(x, y, &row, &c0, &c1) || row != wr || c0 != wc) {
        snprintf(t, sizeof t, "Select at %d,%d lights row %d cells %d-%d, the word is at %d of row %d", x, y, row, c0, c1, wc, wr);
        fail("M3", t);
    }
    if (pixels && !lit_cell(x / 8, y / 8)) {
        snprintf(t, sizeof t, "Select at %d,%d is not on the lit word", x, y);
        fail("M3", t);
    }
}

// M3: Select on the lit word does what its tap does
void mod_select_word()
{
    using namespace play;
    static party::Character a0, by_key;
    char t[160];
    int x, y;
    if (!in_modify() || !mod_menu || !nav_point(&x, &y)) return;
    const int sel = menu.selected;
    const char key = text::key(menu, sel);
    const int item = mod_item;
    a0 = *pt->sel();
    select_at(x, y);
    if (!d) return;
    switch (key) {
    case 'K':
    case 'E':
        if (screen != Screen::PartyMenu || modder) {
            snprintf(t, sizeof t, "Select on '%c' didn't leave Modify (screen %d)", key, (int)screen);
            fail("M3", t);
        }
        return;
    case 'A':
    case 'S':
        if (!in_modify()) {
            snprintf(t, sizeof t, "Select on '%c' left Modify", key);
            fail("M3", t);
            return;
        }
        by_key = *pt->sel();
        *pt->sel() = a0;
        modify_step(key == 'A' ? 1 : -1, C);
        if (!same_char(by_key, *pt->sel())) {
            snprintf(t, sizeof t, "Select on '%c' (item %d): not the arrows' value", key, item);
            fail("M3", t);
        }
        break;
    case 'N':
    case 'P':
        if (!in_modify() || mod_item != (item + (key == 'N' ? 1 : 7)) % 8) {
            snprintf(t, sizeof t, "Select on '%c': item %d -> %d", key, item, in_modify() ? mod_item : -1);
            fail("M3", t);
            return;
        }
        break;
    default:
        snprintf(t, sizeof t, "a word '%c' Modify doesn't know", key);
        fail("M3", t);
        return;
    }
    if (in_modify() && menu.selected != sel) {
        snprintf(t, sizeof t, "after Select on word %d the highlight is on %d", sel, menu.selected);
        fail("M3", t);
    }
}

// M5: Select on an item; on the name, the keyboard (left again by Esc)
void mod_select_item()
{
    using namespace play;
    char t[120];
    int x, y;
    if (!in_modify() || mod_menu || !nav_point(&x, &y)) return;
    const int item = mod_item;
    select_at(x, y);
    if (!d) return;
    if (item == 7) {
        if (screen != Screen::Modify || input_mode != Input::Text || engine_ask != EngineAsk::ModName) {
            snprintf(t, sizeof t, "Select on the name: no name keyboard (screen %d input %d)", (int)screen, (int)input_mode);
            fail("M5", t);
            return;
        }
        back(C);
        if (!in_modify() || mod_item != 7) fail("M5", "Esc from the name keyboard didn't come back to Modify on the name");
        return;
    }
    if (!in_modify() || mod_item != item || mod_menu) {
        snprintf(t, sizeof t, "Select on item %d: now item %d menu %d screen %d", item, mod_item, (int)mod_menu, (int)screen);
        fail("M5", t);
    }
}

void probe_modify()
{
    using namespace play;
    if (!in_modify() || !pt->sel()) return;
    const bool pixels = !hl();
    ++probes;
    ++screens[(int)screen];
    sigs.insert(sig());
    char t[160];
    if (!mod_menu) {
        mod_mark_check("shown");
        mod_arrows();
        if (!in_modify()) return;
        // M1: Down to the menu line, Up back to the item
        const int k = mod_item;
        int n = 0;
        while (n < 9 && in_modify() && !mod_menu) {
            nav(Nav::Down, C);
            ++n;
            if (in_modify() && !mod_menu) mod_mark_check("after Down");
        }
        if (!in_modify()) return;
        if (!mod_menu) {
            snprintf(t, sizeof t, "Down 9 times from item %d: not on the menu line (item %d)", k, mod_item);
            fail("M1", t);
            return;
        }
        for (int i = 0; i < n && in_modify(); ++i) nav(Nav::Up, C);
        if (!in_modify() || mod_menu || mod_item != k) {
            snprintf(t, sizeof t, "Up %d times from the menu line: item %d menu %d, not item %d", n, mod_item, (int)mod_menu, k);
            fail("M1", t);
            return;
        }
        // (the other way: Up to the menu line)
        n = 0;
        while (n < 9 && in_modify() && !mod_menu) {
            nav(Nav::Up, C);
            ++n;
        }
        if (!in_modify()) return;
        if (!mod_menu) {
            snprintf(t, sizeof t, "Up 9 times from item %d: not on the menu line", k);
            fail("M1", t);
            return;
        }
    }
    mod_menu_checks(pixels);
    if (!in_modify()) return;
    if (no_select) return;
    if (pick(2) == 0) {
        // the name: Up from the menu line, Select (its keyboard), Esc, Down
        nav(Nav::Up, C);
        if (!in_modify() || mod_menu || mod_item != 7) {
            snprintf(t, sizeof t, "Up from the menu line: item %d menu %d, not the name", mod_item, (int)mod_menu);
            fail("M1", t);
            return;
        }
        mod_mark_check("on the name");
        mod_select_item();
        if (!in_modify()) return;
        nav(Nav::Down, C);
        if (!in_modify() || !mod_menu) {
            fail("M1", "Down from the name: not the menu line");
            return;
        }
    } else if (pick(3) == 0) {
        // Select on a stat / the hit points
        nav(Nav::Down, C);
        for (int i = pick(7); i > 0 && in_modify(); --i) nav(Nav::Down, C);
        mod_select_item();
        return;
    }
    if (pick(3) == 0) {
        // (any word now and then: Keep / Exit leave)
        const int w = pick(menu.count);
        for (int i = 0; i < w && in_modify(); ++i) nav(Nav::Right, C);
        mod_select_word();
    }
}

// The monkey's way in (NMODIFY): the party menu's Modify line lit by the
// cursor keys, then Select - for a random member
bool enter_modify()
{
    using namespace play;
    if (screen != Screen::PartyMenu || !pt || !pt->count || input_mode != Input::None) return false;
    bool has = false;
    for (int l = 0; l < pm_lines; ++l) has |= pm_key(pm_item[l]) == 'M';
    if (!has) return false;
    pt->selected = pick(pt->count);
    draw_party_menu(C);
    for (int k = 0; k < 2 * pm_lines + 2 && pm_key(pm_item[pm_sel]) != 'M'; ++k) nav(Nav::Down, C);
    int x, y;
    if (pm_key(pm_item[pm_sel]) != 'M' || !nav_point(&x, &y)) return false;
    ++entered;
    select_at(x, y);
    if (!d) return false;
    settle();
    // (Champions / Gateway: "Pick Character Select Exit" first - Select on its first word)
    if (screen == Screen::PartyMenu && menu_on && menu.count >= 1 && !is_exit_word(0) && pm_pick_for == 'M') {
        menu.selected = 0;
        if (nav_point(&x, &y)) select_at(x, y);
        if (!d) return false;
        settle();
    }
    if (screen == Screen::Modify) ++opened;
    else if (getenv("NMODTRACE")) printf("  [modify not opened: screen %d, prompt '%s']\n", (int)screen, menu.prompt);
    return screen == Screen::Modify;
}

void probe()
{
    using namespace play;
    if (!settled()) return;
    settle();
    if (!settled()) return;
    const bool pixels = !hl();
    if (screen == Screen::Modify) {         // (its own rules: M1-M5)
        probe_modify();
        return;
    }
    const bool has_menu = menu_on && menu.count >= 2 &&
                          !(screen == Screen::Game && waiting && vm->wait() == ecl::Wait::Menu && vm->items() == 1);
    std::vector<Run> rows = tap_rows();
    // The party beside a menu line (exploring, camp, Magic, shop, Cast,
    // Alter): the chosen member's name drawn white as the originals draw it
    // (draw_party, no box) - Up / Down move pt->selected (party_nav)
    bool side = false;
    if (pt && pt->count >= 2 && rows.size() >= 2 &&
        (screen == Screen::Game || screen == Screen::Camp || screen == Screen::Magic || screen == Screen::Shop ||
         screen == Screen::Cast || screen == Screen::Alter)) {
        side = true;
        for (const Run& r : rows)
            if (r.row < 4 || r.row >= 4 + pt->count) side = false;
    }
    if (side && !menu_on) return;           // (a message over it: not a screen to pick on)
    const std::vector<Run> lit = pixels ? lit_runs(0, menu_on ? menu_top() - 1 : 24) : std::vector<Run>();
    ++probes;
    ++screens[(int)screen];
    sigs.insert(sig());
    char t[200];
    // N1: the menu's word
    if (has_menu && !pixels && (menu.selected < 0 || menu.selected >= menu.count)) fail("N1", "no word highlighted");
    if (has_menu && pixels) {
        const int sel = menu.selected;
        bool whole = false;
        if (sel < 0 || sel >= menu.count) fail("N1", "no word highlighted");
        else if (!menu_word_lit(sel, &whole) || !whole) {
            snprintf(t, sizeof t, "word %d '%.*s' not a whole white box", sel, menu.end[sel] - menu.start[sel] + 1,
                     menu.s + menu.start[sel]);
            fail("N1", t);
        }
        for (int k = 0; k < menu.count; ++k) {
            bool w;
            if (k != sel && menu_word_lit(k, &w)) {
                snprintf(t, sizeof t, "word %d lit besides %d", k, sel);
                fail("N1", t);
                break;
            }
        }
    }
    // N2: the list's line
    int lit_row = -1;
    if (pixels && rows.size() >= 2 && !side) {
        int n = 0;
        for (const Run& r : rows)
            for (const Run& l : lit)
                if (l.row == r.row && l.c1 >= r.c0 && l.c0 <= r.c1) {
                    ++n;
                    lit_row = r.row;
                    // the line's text inside the box: from its first letter to
                    // the end of its first piece (words one space apart; a
                    // column further on - AC, HP - may stay outside)
                    int first = -1;
                    for (int c = r.c0; c <= r.c1 && c < 40 && first < 0; ++c)
                        if (ink_cell(c, r.row) || lit_cell(c, r.row)) first = c;
                    int end = first;
                    for (int c = first; c >= 0 && c <= r.c1 && c < 40; ++c) {
                        const bool on = ink_cell(c, r.row);
                        if (on) end = c;
                        else if (c + 1 < 40 && !ink_cell(c + 1, r.row) && !lit_cell(c + 1, r.row)) break;
                    }
                    if (first >= 0 && (l.c0 > first || l.c1 < end)) {
                        snprintf(t, sizeof t, "row %d: text %d-%d, the box %d-%d", r.row, first, end, l.c0, l.c1);
                        fail("N2", t);
                    }
                    break;
                }
        if (n == 0) {
            snprintf(t, sizeof t, "%zu tappable lines (rows %d-%d), none lit", rows.size(), rows.front().row, rows.back().row);
            fail("N2", t);
        } else if (n > (screen == Screen::PartyMenu ? 2 : 1)) {
            snprintf(t, sizeof t, "%d of %zu lines lit", n, rows.size());
            fail("N2", t);
        }
    }
    // N3: Left / Right on the menu line
    if (has_menu) {
        const int sel = menu.selected;
        const int cnt = menu.count;
        if (!nav(Nav::Right, C)) fail("N3", "Right did nothing");
        else if (menu.selected == sel && !(rows.size() >= 2 && screen == Screen::PartyMenu)) fail("N3", "Right left the highlight");
        else {
            bool whole = false;
            if (pixels && menu_on && menu.selected >= 0 && menu.selected < menu.count &&
                (!menu_word_lit(menu.selected, &whole) || !whole))
                fail("N3", "the word moved to isn't lit");
            const int to = menu.selected;
            nav(Nav::Left, C);
            if (menu_on && menu.selected != sel) {
                snprintf(t, sizeof t, "Left didn't come back (%d -> %d -> %d of %d, now %d words)", sel, to, menu.selected, cnt,
                         menu.count);
                fail("N3", t);
            } else if (pixels && menu_on && sel >= 0 && sel < menu.count && (!menu_word_lit(sel, &whole) || !whole)) {
                fail("N3", "back on the first word, it isn't lit");
            }
        }
        if (!d || !settled()) return;
    }
    // N4: Up / Down on a list (or a lit line that might be one)
    const int lit_any = pixels ? lit_line(rows) : -1;
    if (rows.size() >= 2 || lit_any >= 0) {
        const bool listy = rows.size() >= 2;
        const bool by_member = side;
        const long st0 = list_state();
        const int before = pixels && !by_member ? (lit_row >= 0 ? lit_row : lit_any) : by_member ? pt->selected : 0;
        const Screen s0 = screen;
        const bool moved_key = nav(Nav::Down, C);
        if (!d) return;
        if (screen != s0) {
            fail("N4", "Down changed the screen");
            return;
        }
        int after = -1;
        if (by_member) after = pt->selected;
        else if (pixels) after = lit_line(listy ? tap_rows() : std::vector<Run>());
        else after = list_state() == st0 ? 0 : 1;
        if (listy && !moved_key) fail("N4", "Down did nothing on a list");
        else if (listy && moved_key && after == before && (by_member || lit_row >= 0)) {
            // (a list scrolled under a fixed box is a move too: compare the line's text)
            snprintf(t, sizeof t, "Down left the lit line where it was (row %d, list %d of %d top %d, spells %d)", before, plist.index, plist.n, plist.top, sl.sel);
            fail("N4", t);
        } else if (!listy && moved_key && !by_member && after != before && after >= 0) {
            int row = -1, c0, c1;
            if (!tap_target(10 * 8 + 2, after * 8 + 2, &row, &c0, &c1) &&
                !tap_target(20 * 8 + 2, after * 8 + 2, &row, &c0, &c1)) {
                snprintf(t, sizeof t, "Down moved a lit line (row %d -> %d) a tap doesn't light", before, after);
                fail("N4", t);
            }
        }
        if (moved_key) nav(Nav::Up, C);
        if (!d || !settled()) return;
    }
    // N5: Select on the highlight
    int x = -1, y = -1;
    if ((has_menu || (rows.size() >= 2 && lit_row >= 0)) && nav_point(&x, &y)) {
        int row = -1, c0 = 0, c1 = 0;
        const bool tt = tap_target(x, y, &row, &c0, &c1);
        if (pixels && (has_menu || lit_row >= 0)) {
            if (!lit_cell(x / 8, y / 8)) {
                snprintf(t, sizeof t, "Select at %d,%d (cell %d,%d) is not on the highlight", x, y, x / 8, y / 8);
                fail("N5", t);
            }
        }
        if (tt && has_menu && menu.selected >= 0 && menu.selected < menu.count && y >= text::kMenuTapTop) {
            int wc = 0, wr = 0;
            text::item_cell(menu, menu.selected, &wc, &wr);
            if (row >= 0 && (row != wr || c0 != wc)) {
                snprintf(t, sizeof t, "Select's tap lights cells %d-%d of row %d, the word is at %d of row %d", c0, c1, row, wc, wr);
                fail("N5", t);
            }
        }
        if (!tt && (has_menu || rows.size() >= 2)) {
            snprintf(t, sizeof t, "Select at %d,%d: a tap there lights nothing", x, y);
            fail("N5", t);
        }
    }
    // N6: Select, then the same menu / list keeps its place (now and then)
    if (x >= 0 && !no_select && pick(3) == 0 && settled()) {
        const std::string s0 = sig();
        const int sel = menu.selected;
        const char key = has_menu && sel >= 0 && sel < menu.count ? text::key(menu, sel) : 0;
        const bool exitish = has_menu && sel >= 0 && (is_exit_word(sel) || key == 'N' || key == 'P');
        const long st0 = list_state(), len0 = list_len();
        select_at(x, y);
        if (!d) return;
        settle();
        if (!d || !settled() || sig() != s0 || exitish) return;
        if (has_menu && menu.selected != sel) {
            snprintf(t, sizeof t, "after Select on word %d the menu came back on %d", sel, menu.selected);
            fail("N6", t);
        }
        // (creation's pick lists: each Select the next list, under the same words)
        if (rows.size() >= 2 && screen != Screen::CreatePick && list_len() == len0 && list_state() != st0) {
            snprintf(t, sizeof t, "after Select the list's place moved (party %d, line %d, item %d, spell %d)", pt->selected,
                     pm_sel, plist.index, sl.sel);
            fail("N6", t);
        }
    }
}

void report()
{
    printf("== navcheck: %d probes, %zu screen signatures, %d failures\n", probes, sigs.size(), fails);
    printf("  Modify: %d tries from a hall's party menu, %d opened, %d probes\n", entered, opened,
           screens.count((int)play::Screen::Modify) ? screens[(int)play::Screen::Modify] : 0);
    printf("  screens probed:");
    for (auto& s : screens) printf(" %d(%d)", s.first, s.second);
    printf("\n");
    if (getenv("NAVSIGS"))
        for (auto& s : sigs) printf("  sig %s\n", s.c_str());
    problems += fails;
}

} // namespace nvc
