// Quick Actions (Tom, 2026-11-09; app/features.h CYD_QUICK_ACTIONS): what
// each of the player's own characters may do when Quick plays it in a
// fight, and the page that sets it - the Game menu's Quick tab. Part of
// play.cpp (included inside its anonymous namespace).
//
// The switches are kept by the character's name in /GOLDBOX/_CYD/QUICK.TXT
// (engine/quickacts.*: a line a name). qa_mask holds the party's - a byte a
// member - read at a fight's start and when the page opens; the page
// writes the file at each change.
//
// The page is drawn INTO the game canvas in the game's look (its text
// screen's border, its font): the game screen waits on the card meanwhile
// (_CYD/QUICK.TMP) and comes back as it was. Layout (8 x 8 cells):
//   row 1  QUICK ACTIONS (and the circled "?" at the top right)
//   rows 2 - 3  the columns' headings Mel Mis Spl Itm Bnd Mov Trn Grd -
//          staggered: Mel Spl Bnd Trn on row 2, Mis Itm Mov Grd on row 3, so
//          the words stand apart (a column 3 cells wide from cell 15, the box
//          in its middle cell); Name on row 3
//   rows 5, 7 ... 19  a character each (up to 8): its name, its 8 boxes
//   rows 21 - 22  what the column under the cursor does
//   row 24  Exit (the menu line)
// The cursor: a row (-1 the "?", 0..n-1 a character, n Exit) and a column.

uint8_t qa_mask[party::kMaxParty] = {quickacts::kDefault, quickacts::kDefault, quickacts::kDefault, quickacts::kDefault,
                                     quickacts::kDefault, quickacts::kDefault, quickacts::kDefault, quickacts::kDefault};

struct QuickPage {
    bool on = false;
    bool help = false;                  // the "?" page over it
    bool saved = false;                 // the game screen waits on the card
    int8_t n = 0;                       // its rows: the party's own characters
    int8_t member[party::kMaxParty] = {};
    int8_t row = 0, col = 0;            // the cursor
    int fight_pal = 0;                  // the fight's colours as they were (none while it's up)
};
QuickPage qp;

constexpr int kQaGridX = 15 * 8;        // the first column's left
constexpr int kQaColW = 24;
constexpr int kQaRow0 = 5 * 8;          // the first character's row (pixels)
constexpr int kQaRowH = 16;
constexpr int kQaHelpX = 300, kQaHelpY = 16;   // the "?"'s centre

void qa_path(char* out, size_t cap, const char* file) { snprintf(out, cap, "%s/_CYD/%s", games::kRootDir, file); }

// QUICK.TXT whole (malloc'd, 0-ended; nullptr: none or no memory)
char* qa_read(size_t* n)
{
    *n = 0;
    char path[96];
    qa_path(path, sizeof path, "QUICK.TXT");
    plat::File f = plat::open(path, "r");
    if (!f) return nullptr;
    size_t len = f.size();
    if (len > quickacts::kFileMax) len = quickacts::kFileMax;
    char* t = static_cast<char*>(malloc(len + 1));
    if (t) {
        const int got = len ? f.read(reinterpret_cast<uint8_t*>(t), len) : 0;
        *n = got > 0 ? static_cast<size_t>(got) : 0;
        t[*n] = 0;
    }
    f.close();
    return t;
}

bool qa_member(int m)
{
    return pt && m >= 0 && m < pt->count && !pt->m[m].npc();
}

void qa_load()
{
    for (uint8_t& m : qa_mask) m = quickacts::kDefault;
    if (!d || !pt || !qa_on()) return;
    size_t n;
    char* t = qa_read(&n);
    if (!t) return;
    for (int i = 0; i < pt->count && i < party::kMaxParty; ++i) {
        if (!qa_member(i)) continue;
        char nm[24];
        pt->m[i].name(nm, sizeof nm);
        qa_mask[i] = quickacts::find(t, n, nm);
    }
    free(t);
}

// The file with member m's line as it is now (QUICK.NEW written whole, then
// QUICK.TXT replaced: a failed write never leaves it half written)
bool qa_write(int m)
{
    if (!qa_member(m)) return false;
    char nm[24];
    pt->m[m].name(nm, sizeof nm);
    size_t n;
    char* t = qa_read(&n);
    const size_t cap = n + 2 * quickacts::kNameMax + 160;
    char* out = static_cast<char*>(malloc(cap));
    const size_t len = out ? quickacts::set(t, n, nm, qa_mask[m], out, cap) : 0;
    free(t);
    char dir[64], tmp[96], path[96];
    snprintf(dir, sizeof dir, "%s/_CYD", games::kRootDir);
    qa_path(tmp, sizeof tmp, "QUICK.NEW");
    qa_path(path, sizeof path, "QUICK.TXT");
    bool ok = len > 0;
    if (ok) {
        if (!plat::exists(dir)) plat::mkdir(dir);
        plat::File f = plat::open(tmp, "w");
        ok = f && f.write(reinterpret_cast<const uint8_t*>(out), len) == len;
        if (f) f.close();
        ok = ok && plat::rename(tmp, path);
    }
    free(out);
    if (!ok) plat::log("[play] quick: QUICK.TXT not written\n");
    return ok;
}

uint8_t quick_mask(const combat::Fighter& f)
{
    if (!qa_on() || f.team() || !f.quick || f.member < 0 || f.member >= party::kMaxParty || !qa_member(f.member) ||
        f.rec[0xF7] >= 0x80 || f.status() == party::Animated)
        return quickacts::kDefault;
    return qa_mask[f.member];
}

// ---- The page
bool qa_save_screen(pic::Canvas& c)
{
    char path[96];
    qa_path(path, sizeof path, "QUICK.TMP");
    plat::File f = plat::open(path, "w");
    if (!f) {
        char dir[64];
        snprintf(dir, sizeof dir, "%s/_CYD", games::kRootDir);
        if (!plat::exists(dir)) plat::mkdir(dir);
        f = plat::open(path, "w");
    }
    if (!f) return false;
    uint8_t row[pic::kScreenW];
    bool ok = c.w <= pic::kScreenW;
    for (int y = 0; ok && y < c.h; ++y) {
        c.get_row(0, y, c.w, row);
        ok = f.write(row, static_cast<size_t>(c.w)) == static_cast<size_t>(c.w);
    }
    f.close();
    return ok;
}

bool qa_restore_screen(pic::Canvas& c)
{
    char path[96];
    qa_path(path, sizeof path, "QUICK.TMP");
    plat::File f = plat::open(path, "r");
    if (!f) return false;
    uint8_t row[pic::kScreenW];
    bool ok = c.w <= pic::kScreenW;
    for (int y = 0; ok && y < c.h; ++y) {
        ok = f.read(row, static_cast<size_t>(c.w)) == c.w;
        if (ok) c.put_row(0, y, c.w, row);
    }
    f.close();
    plat::remove(path);
    dirty(0, pic::kScreenH);
    return ok;
}

// A fill (colour 0: true black, whatever paper the game screen has - the Dark Queen's grey)
void qa_fill(pic::Canvas& c, int x, int y, int w, int h, uint8_t col)
{
    if (col == 0) c.fill_black(x, y, w, h);
    else c.fill(x, y, w, h, col);
}

// Text in colour fg (paper: a colour behind it, -1 none) from pixel (x, y)
void qa_text(pic::Canvas& c, const char* s, int x, int y, uint8_t fg = 15, int paper = -1)
{
    const int n = static_cast<int>(strlen(s));
    if (paper >= 0) qa_fill(c, x, y, n * 8, 8, static_cast<uint8_t>(paper));
    for (int i = 0; i < n; ++i) {
        const char ch = s[i] >= ' ' && s[i] <= '~' ? s[i] : ' ';
        font::draw_glyph_scaled(c, d->font, font::glyph_of(d->font, ch), x + i * 8, y, fg, 1);
    }
}

void qa_frame(pic::Canvas& c, int x, int y, int w, int h, uint8_t col)
{
    qa_fill(c, x, y, w, 1, col);
    qa_fill(c, x, y + h - 1, w, 1, col);
    qa_fill(c, x, y, 1, h, col);
    qa_fill(c, x + w - 1, y, 1, h, col);
}

int qa_row_y(int r) { return kQaRow0 + r * kQaRowH; }

void qa_box(pic::Canvas& c, int r, int k)
{
    const int x = kQaGridX + k * kQaColW + 9, y = qa_row_y(r);      // (a pixel clear of a 15-letter name)
    qa_fill(c, x, y, 8, 8, 0);
    qa_frame(c, x, y, 7, 7, 15);
    if (qa_mask[qp.member[r]] >> k & 1) qa_fill(c, x + 2, y + 2, 3, 3, 15);
}

// The circled "?" (the Settings' "?" key in the game's look)
void qa_qmark(pic::Canvas& c)
{
    // (a ring of radius 6: the points with dx * dx + dy * dy close to 36)
    for (int dy = -6; dy <= 6; ++dy)
        for (int dx = -6; dx <= 6; ++dx) {
            const int r2 = dx * dx + dy * dy;
            if (r2 >= 30 && r2 <= 42) c.set(kQaHelpX + dx, kQaHelpY + dy, 15);
        }
    qa_text(c, "?", kQaHelpX - 3, kQaHelpY - 3);
}

// The cursor's place: ink 15 to show it, 0 to take it away
void qa_cursor(pic::Canvas& c, uint8_t ink)
{
    if (qp.row < 0) {
        qa_frame(c, kQaHelpX - 10, 8, 21, 16, ink);
    } else if (qp.row >= qp.n) {
        // Exit, the menu line's word: the games' highlight (a white box
        // round the word, black letters)
        qa_text(c, "Exit", 0, text::kMenuRow * 8, ink ? 0 : 15, ink ? 15 : 0);
    } else {
        qa_frame(c, kQaGridX + qp.col * kQaColW, qa_row_y(qp.row) - 3, kQaColW, 14, ink);
    }
}

void qa_help_line(pic::Canvas& c)
{
    qa_fill(c, 8, 21 * 8, pic::kScreenW - 16, 16, 0);
    const char* a = qp.row < 0 ? "How Quick Actions works." : qp.row >= qp.n ? "Back to the game." : quickacts::kHelp[qp.col][0];
    const char* b = qp.row < 0 || qp.row >= qp.n ? "" : quickacts::kHelp[qp.col][1];
    qa_text(c, a, 8, 21 * 8);
    qa_text(c, b, 8, 22 * 8);
}

void qa_draw(pic::Canvas& c)
{
    text_screen(c);
    qa_text(c, "QUICK ACTIONS", (pic::kScreenW - 13 * 8) / 2, 8);
    qa_qmark(c);
    qa_text(c, "Name", 8, 3 * 8);
    for (int k = 0; k < quickacts::kCols; ++k) qa_text(c, quickacts::kHead[k], kQaGridX + k * kQaColW, (2 + (k & 1)) * 8);
    for (int r = 0; r < qp.n; ++r) {
        char nm[24];
        pt->m[qp.member[r]].name(nm, sizeof nm);
        nm[15] = 0;
        qa_text(c, nm, 8, qa_row_y(r));
        for (int k = 0; k < quickacts::kCols; ++k) qa_box(c, r, k);
    }
    if (qp.n == 0) qa_text(c, "No characters of your own.", 8, qa_row_y(0));
    qa_text(c, "Exit", 0, text::kMenuRow * 8);
    qa_help_line(c);
    qa_cursor(c, 15);
    dirty(0, pic::kScreenH);
}

// The "?" page: how it works, each column a line (38 characters at most)
void qa_draw_help(pic::Canvas& c)
{
    static const char* const kLines[] = {
        "Quick plays a character in a fight:",
        "tick what each of your own characters",
        "may do then. All but Bnd ticked, as",
        "they start, is the game's own Quick.",
        "Kept by name for every saved game and",
        "the later games. Settings: Quick",
        "Actions switches it all off.",
        "",
        "Mel  Melee: attack foes next to it",
        "Mis  Missile: shoot and throw",
        "Spl  Spells: cast spells",
        "Itm  Items: potions, wands and such",
        "Bnd  Bandage First: the dying first",
        "Mov  Move: step toward enemies",
        "Trn  Turn Undead: turn undead",
        "Grd  Guard: guard with nothing to do",
        "Unticked: never. Bnd unticked: the",
        "game's own order. Grd unticked: the",
        "turn just ends.",
    };
    text_screen(c);
    qa_text(c, "QUICK ACTIONS", (pic::kScreenW - 13 * 8) / 2, 8);
    int row = 3;
    for (const char* l : kLines) qa_text(c, l, 8, 8 * row++);
    qa_text(c, "Tap to close.", 8, 8 * 22);
    dirty(0, pic::kScreenH);
}

void qa_move(pic::Canvas& c, int row, int col)
{
    qa_cursor(c, 0);
    qp.row = static_cast<int8_t>(row);
    qp.col = static_cast<int8_t>(col);
    qa_cursor(c, 15);
    qa_help_line(c);
    dirty(0, pic::kScreenH);
}

void qa_toggle(pic::Canvas& c, int r, int k)
{
    const int m = qp.member[r];
    qa_mask[m] = static_cast<uint8_t>(qa_mask[m] ^ (1 << k));
    qa_box(c, r, k);
    dirty(qa_row_y(r), qa_row_y(r) + 8);
    qa_write(m);
}

void qa_close(pic::Canvas& c)
{
    if (!qp.on) return;
    qp.on = false;
    qp.help = false;
    fight_palette_on = qp.fight_pal;
    if (!qa_restore_screen(c)) {
        // (the screen didn't come back from the card: the game draws its own again)
        plat::log("[play] quick: the screen didn't come back from the card\n");
        c.clear(0);
        if (fg && screen == Screen::Fight) draw_fight(c);
        else if (screen == Screen::Game) after_move_redraw();
        redraw_menu(c);
        dirty(0, pic::kScreenH);
    }
    plat::log("[play] quick: page closed\n");
}

// What a canvas point is on the page: 0 nothing, 1 a box (*r, *k), 2 a name
// (*r), 3 a heading (*k), 4 the "?", 5 Exit
int qa_hit(int x, int y, int* r, int* k)
{
    // (the "?": its ring and the box round it, rows 1-2 - Grd's heading under it on row 3)
    if (x >= kQaHelpX - 11 && x <= kQaHelpX + 11 && y >= 4 && y < 24) return 4;
    if (y >= text::kMenuRow * 8 - 4 && x < 48) return 5;
    const int col = x >= kQaGridX && x < kQaGridX + quickacts::kCols * kQaColW ? (x - kQaGridX) / kQaColW : -1;
    if (y >= 2 * 8 - 2 && y < 3 * 8 + 12 && col >= 0) {
        *k = col;
        return 3;
    }
    const int rr = (y - kQaRow0 + 4) / kQaRowH;
    if (y >= kQaRow0 - 4 && rr >= 0 && rr < qp.n) {
        *r = rr;
        if (col >= 0) {
            *k = col;
            return 1;
        }
        if (x >= 8 && x < kQaGridX) return 2;
    }
    return 0;
}

// The tap highlight: the cursor goes where the stylus touches
bool qa_press(int x, int y, pic::Canvas& c)
{
    if (qp.help) return false;
    int r = qp.row, k = qp.col;
    switch (qa_hit(x, y, &r, &k)) {
    case 1: qa_move(c, r, k); return true;
    case 2: qa_move(c, r, qp.col); return true;
    case 3: qa_move(c, qp.row >= 0 && qp.row < qp.n ? qp.row : (qp.n ? 0 : qp.n), k); return true;
    case 4: qa_move(c, -1, qp.col); return true;
    case 5: qa_move(c, qp.n, qp.col); return true;
    default: return false;
    }
}

void qa_tap(int x, int y, pic::Canvas& c)
{
    if (qp.help) {
        qp.help = false;
        qa_draw(c);
        return;
    }
    int r = qp.row, k = qp.col;
    const int what = qa_hit(x, y, &r, &k);
    if (what == 1) {
        if (qp.row != r || qp.col != k) qa_move(c, r, k);
        qa_toggle(c, r, k);
    } else if (what == 3) {
        if (qp.row < 0 || qp.row >= qp.n) qa_move(c, qp.n ? 0 : qp.n, k);
        else if (qp.col != k) qa_move(c, qp.row, k);
    } else if (what == 4) {
        qp.help = true;
        qp.row = -1;
        qa_draw_help(c);
    } else if (what == 5) {
        qa_close(c);
    }
}

bool qa_nav(Nav n, pic::Canvas& c)
{
    if (qp.help) {
        qp.help = false;
        qa_draw(c);
        return true;
    }
    const int rows = qp.n + 2;              // ("?", the characters, Exit)
    if (n == Nav::Up || n == Nav::Down) {
        const int step = n == Nav::Up ? -1 : 1;
        qa_move(c, (qp.row + 1 + step + rows) % rows - 1, qp.col);
        return true;
    }
    if (qp.row < 0 || qp.row >= qp.n) return true;      // (the "?", Exit: one thing on their line)
    const int step = n == Nav::Left ? -1 : 1;
    qa_move(c, qp.row, (qp.col + step + quickacts::kCols) % quickacts::kCols);
    return true;
}

bool qa_point(int* x, int* y)
{
    if (qp.help) {
        *x = pic::kScreenW / 2;
        *y = 100;
    } else if (qp.row < 0) {
        *x = kQaHelpX;
        *y = kQaHelpY;
    } else if (qp.row >= qp.n) {
        *x = 8;
        *y = text::kMenuRow * 8 + 3;
    } else {
        *x = kQaGridX + qp.col * kQaColW + 12;
        *y = qa_row_y(qp.row) + 3;
    }
    return true;
}

bool qa_ready()
{
    if (!qa_on() || !d || !pt || input_mode != Input::None || !d->font.loaded) return false;
    for (int i = 0; i < pt->count && i < party::kMaxParty; ++i)
        if (qa_member(i)) return true;
    return false;
}

bool qa_open(pic::Canvas& c)
{
    if (qp.on) return true;
    if (!qa_ready()) return false;
    cv = &c;
    tap_highlight_end(c);
    qa_load();
    qp = QuickPage{};
    for (int i = 0; i < pt->count && i < party::kMaxParty; ++i)
        if (qa_member(i)) qp.member[qp.n++] = static_cast<int8_t>(i);
    if (!qa_save_screen(c)) {
        // (no room on the card for the game screen: no page - it would be lost)
        plat::log("[play] quick: the screen couldn't wait on the card\n");
        return false;
    }
    qp.saved = true;
    qp.on = true;
    qp.fight_pal = fight_palette_on;
    fight_palette_on = 0;                   // (the page in the game's text colours, a fight's swap aside)
    qa_draw(c);
    plat::log("[play] quick: page open, %d characters\n", qp.n);
    memlog::note("Quick Actions page");
    return true;
}
