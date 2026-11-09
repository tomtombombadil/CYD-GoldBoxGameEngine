#include "quickacts.h"

#include <cstring>

namespace quickacts {

const char* const kHead[kCols] = {"Mel", "Mis", "Spl", "Itm", "Bnd", "Mov", "Trn", "Grd"};
const char* const kHelp[kCols][2] = {
    {"Melee: attacks foes next to it.", "Unticked: no melee attacks."},
    {"Missile: shoots and throws.", "Unticked: no missile attacks."},
    {"Spells: Magic On, casts its spells.", "Unticked: Magic Off, as fights start."},
    {"Items: potions, wands, magic items.", "Unticked: uses no items."},
    {"Bandage First: the dying, first thing.", "Unticked: the game's own order."},
    {"Move: steps toward enemies.", "Unticked: holds its square."},
    {"Turn: turns undead.", "Unticked: never turns undead."},
    {"Guard: guards with nothing to do.", "Unticked: just ends its turn."},
};

namespace {

constexpr const char* kComment =
    "# Quick Actions 2: Mel Mis Spl Itm Bnd Mov Trn Grd (1 ticked, 0 not), then the character's name\n";
// The first version's files (v0.184.0, "# Quick Actions: ..."): their Spl
// did nothing (no Quick spells for the party then) - read as unticked, the
// behaviour they were made with; rewritten in this version at the next set
constexpr const char* kTag2 = "# Quick Actions 2:";

bool new_file(const char* t, size_t n)
{
    const size_t k = strlen(kTag2);
    return t && n >= k && memcmp(t, kTag2, k) == 0;
}

char upper(char c) { return c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c; }

// The line's name (after "dddddddd ") equal to the cleaned name, case aside
bool same(const char* a, size_t na, const char* b)
{
    while (na && (a[na - 1] == ' ' || a[na - 1] == '\r' || a[na - 1] == '\t')) --na;
    const size_t nb = strlen(b);
    if (na != nb) return false;
    for (size_t i = 0; i < na; ++i)
        if (upper(a[i]) != upper(b[i])) return false;
    return true;
}

// A character's line: 8 switches, a space, a name; *mask its switches
bool parse(const char* s, size_t n, uint8_t* mask)
{
    if (n < kCols + 2 || s[0] == '#' || s[kCols] != ' ') return false;
    uint8_t m = 0;
    for (int k = 0; k < kCols; ++k) {
        if (s[k] == '1') m = static_cast<uint8_t>(m | 1 << k);
        else if (s[k] != '0') return false;
    }
    *mask = m;
    return true;
}

// Calls fn(start, length) for each line (without its '\n')
template <typename Fn> void lines(const char* t, size_t n, Fn fn)
{
    size_t i = 0;
    while (i < n) {
        size_t e = i;
        while (e < n && t[e] != '\n') ++e;
        fn(i, e - i);
        i = e + 1;
    }
}

} // namespace

void clean_name(const char* name, char* out)
{
    size_t n = 0;
    for (; name && name[n] && n < static_cast<size_t>(kNameMax); ++n) {
        const char c = name[n];
        out[n] = static_cast<unsigned char>(c) < ' ' || c == 0x7F ? ' ' : c;
    }
    while (n && out[n - 1] == ' ') --n;
    out[n] = 0;
}

uint8_t find(const char* text, size_t n, const char* name)
{
    char nm[kNameMax + 1];
    clean_name(name, nm);
    uint8_t found = kDefault;
    if (!text || !nm[0]) return kDefault;
    const bool old = !new_file(text, n);
    lines(text, n, [&](size_t at, size_t len) {
        uint8_t m;
        if (parse(text + at, len, &m) && same(text + at + kCols + 1, len - kCols - 1, nm))
            found = old ? static_cast<uint8_t>(m & ~kSpells) : m;
    });
    return found;
}

size_t set(const char* text, size_t n, const char* name, uint8_t mask, char* out, size_t cap)
{
    char nm[kNameMax + 1];
    clean_name(name, nm);
    if (!nm[0] || !out) return 0;
    if (!text) n = 0;
    size_t o = 0;
    bool ok = true, done = false;
    auto put = [&](const char* s, size_t len) {
        if (o + len > cap) {
            ok = false;
            return;
        }
        memcpy(out + o, s, len);
        o += len;
    };
    auto put_mask = [&](uint8_t v) {
        char l[kCols + 1];
        for (int k = 0; k < kCols; ++k) l[k] = (v >> k & 1) ? '1' : '0';
        l[kCols] = ' ';
        put(l, sizeof l);
    };
    auto put_line = [&]() {
        put_mask(mask);
        put(nm, strlen(nm));
        put("\n", 1);
    };
    // (a first version's file: its comment line replaced, its lines' Spl
    // unticked as they were read)
    const bool old = n > 0 && !new_file(text, n);
    if (n == 0 || text[0] != '#' || old) put(kComment, strlen(kComment));
    lines(text, n, [&](size_t at, size_t len) {
        uint8_t m;
        const bool line = parse(text + at, len, &m);
        if (line && same(text + at + kCols + 1, len - kCols - 1, nm)) {
            if (!done) put_line();            // (a name twice: the first kept, set)
            done = true;
            return;
        }
        if (len == 0 && at + 1 >= n) return;  // (the text's last newline)
        if (old && at == 0 && len >= 16 && memcmp(text, "# Quick Actions:", 16) == 0) return;
        if (old && line) {
            put_mask(static_cast<uint8_t>(m & ~kSpells));
            put(text + at + kCols + 1, len - kCols - 1);
            put("\n", 1);
            return;
        }
        put(text + at, len);
        put("\n", 1);
    });
    if (!done) put_line();
    return ok ? o : 0;
}

} // namespace quickacts
