// Quick Actions (Tom, 2026-11-09; app/features.h CYD_QUICK_ACTIONS): what
// each of the player's own characters may do when Quick plays it in a
// fight. Part of play.cpp (included inside its anonymous namespace).
//
// The switches are kept by the character's name in /GOLDBOX/_CYD/QUICK.TXT
// (engine/quickacts.*: a line a name). qa_mask holds the party's - a byte a
// member - read at a fight's start and when the Quick tab opens; a toggle
// writes the file at once.
//
// The page that sets them is the ENGINE's Game menu's Quick tab (Tom,
// 2026-11-09: "I intended it for the game/settings menu of the engine where
// you could use smaller fonts and be more descriptive ... the whole
// screen"): ui/viewer.cpp draws it in the engine's look through the model
// below (play::quick_begin / quick_name / quick_get / quick_toggle) - the
// game screen isn't touched, the game waits while the menu is up (as for
// the other tabs). v0.185.0's page on the game canvas (QUICK.TMP) is gone.

uint8_t qa_mask[party::kMaxParty] = {quickacts::kDefault, quickacts::kDefault, quickacts::kDefault, quickacts::kDefault,
                                     quickacts::kDefault, quickacts::kDefault, quickacts::kDefault, quickacts::kDefault};

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

// A party member with switches: the whole party - the player's own
// characters and (Tom, 2026-11-09) its NPCs, the computer's members
bool qa_member(int m)
{
    return pt && m >= 0 && m < pt->count && m < party::kMaxParty;
}
bool qa_npc(int m) { return qa_member(m) && pt->m[m].npc(); }
// A member's switches as they start (not in QUICK.TXT): the player's own -
// the game's Quick (Magic Off); an NPC - the game's AI (kComputer, spells
// always). The kind at the time decides; once its line is written, the line
uint8_t qa_start(int m) { return qa_npc(m) ? quickacts::kComputer : quickacts::kDefault; }

void qa_load()
{
    for (int i = 0; i < party::kMaxParty; ++i) qa_mask[i] = qa_start(i);
    if (!d || !pt || !qa_on()) return;
    size_t n;
    char* t = qa_read(&n);
    if (!t) return;
    for (int i = 0; i < pt->count && i < party::kMaxParty; ++i) {
        if (!qa_member(i)) continue;
        char nm[24];
        pt->m[i].name(nm, sizeof nm);
        qa_mask[i] = quickacts::find(t, n, nm, qa_start(i));
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
    // Monsters (charmed onto the party's side too), the animated dead: the
    // game's own AI (spells always - every game's spell step: control over 0x7F)
    if (!qa_member(f.member) || f.status() == party::Animated) return quickacts::kComputer;
    // The party's NPCs (Tom, 2026-11-09 - our comfort, SPEC 11): their own
    // switches, kComputer as they start (the original's AI: with the
    // defaults nothing changes); one charmed onto the other side, or the
    // switch off: the game's AI
    if (qa_npc(f.member)) return qa_on() && !f.team() ? qa_mask[f.member] : quickacts::kComputer;
    // (one of the player's own with the control byte on just now: the computer's)
    if (f.rec[0xF7] >= 0x80) return quickacts::kComputer;
    // The player's own characters: the switches, or the game's start (Magic
    // Off: no spells - the originals' Alt-M has no key here) with Quick
    // Actions off; one charmed onto the other side keeps only its Spells
    // (the games' Magic flag reaches the charmed too: the gate tests only
    // the control byte and the flag)
    const uint8_t m = qa_on() && qa_member(f.member) ? qa_mask[f.member] : quickacts::kDefault;
    if (f.team() || !f.quick) return static_cast<uint8_t>(quickacts::kComputer & ~quickacts::kSpells) | (m & quickacts::kSpells);
    return m;
}

// ---- The Quick tab's model (play.h): the rows are the whole party - its
// own characters and its NPCs - in party order, up to 8
int8_t qa_row[party::kMaxParty] = {};
int8_t qa_rows = 0;

bool qa_ready()
{
    if (!qa_on() || !d || !pt || input_mode != Input::None) return false;
    return pt->count > 0;
}

int qa_begin()
{
    qa_rows = 0;
    if (!qa_ready()) return 0;
    qa_load();
    for (int i = 0; i < pt->count && i < party::kMaxParty; ++i)
        if (qa_member(i)) qa_row[qa_rows++] = static_cast<int8_t>(i);
    plat::log("[play] quick: tab open, %d characters\n", qa_rows);
    memlog::note("Quick Actions tab");
    return qa_rows;
}

// Row r's party member (-1: none - the party changed since quick_begin)
int qa_row_member(int r)
{
    if (r < 0 || r >= qa_rows) return -1;
    const int m = qa_row[r];
    return qa_member(m) ? m : -1;
}
