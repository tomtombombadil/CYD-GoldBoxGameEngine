// Scroll bundles (bundle.h): the store of the bundles' scrolls and the
// changes Secret of the Silver Blades makes to them. Own code from the facts
// read in its GAME.OVR (Join 0x29391, Halve 0x29089, the spell taken off
// 0x32ACA, the .STF loader 0x25981).
#include "bundle.h"

#include <cstdlib>
#include <cstring>
#include <new>

namespace bundle {

namespace {

constexpr size_t kRec = items::kRecordSize;
constexpr int kIdAt = 0x08;
const uint8_t kMark[5] = {0, 'B', 'N', 'D', 'L'};

struct Entry {
    uint16_t id = 0;
    uint8_t  n = 0;
    uint8_t  rec[kMax][kRec] = {};
};

Kind g_kind = Kind::None;
bool g_secret = false;
Entry** g_list = nullptr;
int g_n = 0, g_cap = 0;
uint16_t g_next = 1;

Entry* find(uint16_t id)
{
    if (!id) return nullptr;
    for (int i = 0; i < g_n; ++i)
        if (g_list[i]->id == id) return g_list[i];
    return nullptr;
}

Entry* entry(const uint8_t* r) { return stored(r) ? find(id_of(r)) : nullptr; }

void set_id(uint8_t* r, uint16_t id)
{
    r[kIdAt] = static_cast<uint8_t>(id);
    r[kIdAt + 1] = static_cast<uint8_t>(id >> 8);
}

// A new, empty entry with an id of its own
Entry* fresh()
{
    if (g_n == g_cap) {
        const int cap = g_cap ? g_cap * 2 : 4;
        Entry** l = static_cast<Entry**>(realloc(g_list, sizeof(Entry*) * cap));
        if (!l) return nullptr;
        g_list = l;
        g_cap = cap;
    }
    Entry* e = new (std::nothrow) Entry;
    if (!e) return nullptr;
    do {
        e->id = g_next++;
        if (!g_next) g_next = 1;
    } while (!e->id || find(e->id));
    g_list[g_n++] = e;
    return e;
}

void drop(Entry* e)
{
    for (int i = 0; i < g_n; ++i)
        if (g_list[i] == e) {
            delete e;
            g_list[i] = g_list[--g_n];
            return;
        }
}

// The record as a bundle: Secret's own fields (GAME.OVR 0x294D8-0x29523)
// or the Enhancement's mark; its id
void become(uint8_t* r, uint16_t id)
{
    if (g_kind == Kind::Secret) {
        r[0x34] = 0;                    // unreadied
        r[0x31] = 0x4D;                 // "Bundle of"
        r[0x30] = 1;                    // (the count, shown as a number)
        r[0x2F] = 0x27;                 // "Scroll"
        r[0x3C] = r[0x3D] = r[0x3E] = 0;
        r[0x39] = 1;
        r[0x2E] = kSecretType;
        r[0x35] = 0;                    // identified
    } else {
        memcpy(r, kMark, sizeof kMark);
        r[0x34] = 0;
        r[0x35] = 0;
        r[0x3C] = r[0x3D] = r[0x3E] = 0;
        r[0x39] = 1;
    }
    set_id(r, id);
}

}  // namespace

void configure(bool secret, bool enhancement_on)
{
    g_secret = secret;
    g_kind = secret ? Kind::Secret : enhancement_on ? Kind::Marked : Kind::None;
}

Kind kind() { return g_kind; }
bool secret() { return g_secret; }

bool marked(const uint8_t* r) { return memcmp(r, kMark, sizeof kMark) == 0; }

bool stored(const uint8_t* r)
{
    if (!r) return false;
    return g_secret ? r[0x2E] == kSecretType : marked(r);
}

bool is(const uint8_t* r) { return g_kind != Kind::None && stored(r); }

uint16_t id_of(const uint8_t* r) { return static_cast<uint16_t>(r[kIdAt] | r[kIdAt + 1] << 8); }

int count(const uint8_t* r)
{
    const Entry* e = entry(r);
    return e ? e->n : 0;
}

uint8_t* scroll(const uint8_t* r, int k)
{
    Entry* e = entry(r);
    return e && k >= 0 && k < e->n ? e->rec[k] : nullptr;
}

bool make(uint8_t* r)
{
    if (g_kind == Kind::None || is(r)) return false;
    Entry* e = fresh();
    if (!e) return false;
    memcpy(e->rec[0], r, kRec);
    e->n = 1;
    become(r, e->id);
    return true;
}

bool add(uint8_t* r, const uint8_t* s)
{
    Entry* e = entry(r);
    if (!e || e->n >= kMax) return false;
    memcpy(e->rec[e->n++], s, kRec);
    r[0x39] = e->n;
    return true;
}

void take(uint8_t* r, int k, uint8_t* out)
{
    Entry* e = entry(r);
    if (!e || k < 0 || k >= e->n) return;
    if (out) memcpy(out, e->rec[k], kRec);
    memmove(e->rec[k], e->rec[k + 1], (e->n - k - 1) * kRec);
    --e->n;
    r[0x39] = e->n;
}

// Secret's Halve of a bundle (GAME.OVR 0x29089): n = the count / 2 go to the
// new item - the bundle's FIRST n scrolls; one alone is that scroll's own
// record (0x290F4), else a bundle of them (count, weight and the count word
// n, its value the sum of theirs - 0x2910D), unreadied. The bundle keeps the
// rest (count / weight / count word set, value less the taken ones' -
// 0x291EF); one left: the bundle becomes that scroll's record (0x2919E)
bool split(uint8_t* r, int n, uint8_t* out)
{
    Entry* e = entry(r);
    if (!e || n <= 0 || n >= e->n) return false;
    const int left = e->n - n;
    auto value = [](const uint8_t* x) { return x[0x3A] | x[0x3B] << 8; };
    auto set_value = [](uint8_t* x, int v) {
        x[0x3A] = static_cast<uint8_t>(v);
        x[0x3B] = static_cast<uint8_t>(v >> 8);
    };
    int moved = 0;
    for (int k = 0; k < n; ++k) moved += value(e->rec[k]);
    if (n == 1) {
        memcpy(out, e->rec[0], kRec);
    } else {
        Entry* ne = fresh();
        if (!ne) return false;
        e = entry(r);                   // (the list may have moved)
        memcpy(out, r, kRec);
        memcpy(ne->rec[0], e->rec[0], n * kRec);
        ne->n = static_cast<uint8_t>(n);
        set_id(out, ne->id);
        out[0x39] = static_cast<uint8_t>(n);
        if (g_kind == Kind::Secret) {
            out[0x37] = static_cast<uint8_t>(n);
            out[0x38] = 0;
            out[0x30] = static_cast<uint8_t>(n);
        }
        set_value(out, moved);
    }
    out[0x34] = 0;
    memmove(e->rec[0], e->rec[n], left * kRec);
    e->n = static_cast<uint8_t>(left);
    if (left == 1) {
        memcpy(r, e->rec[0], kRec);
        drop(e);
        return true;
    }
    r[0x39] = static_cast<uint8_t>(left);
    if (g_kind == Kind::Secret) {
        r[0x37] = static_cast<uint8_t>(left);
        r[0x38] = 0;
        r[0x30] = static_cast<uint8_t>(left);
    }
    set_value(r, (value(r) - moved) & 0xFFFF);
    return true;
}

bool start(uint8_t* r)
{
    Entry* e = fresh();
    set_id(r, e ? e->id : 0);
    r[0x39] = 0;
    return e != nullptr;
}

bool clone(uint8_t* copy)
{
    Entry* e = entry(copy);
    if (!e) return false;
    Entry* ne = fresh();
    if (!ne) return false;
    e = entry(copy);
    memcpy(ne->rec[0], e->rec[0], e->n * kRec);
    ne->n = e->n;
    set_id(copy, ne->id);
    return true;
}

void clear()
{
    for (int i = 0; i < g_n; ++i) delete g_list[i];
    free(g_list);
    g_list = nullptr;
    g_n = g_cap = 0;
}

void gc(bool (*live)(uint16_t id, void* ctx), void* ctx)
{
    for (int i = 0; i < g_n;) {
        if (live(g_list[i]->id, ctx)) {
            ++i;
            continue;
        }
        delete g_list[i];
        g_list[i] = g_list[--g_n];
    }
    if (!g_n) clear();
}

size_t bytes_used() { return static_cast<size_t>(g_n) * sizeof(Entry) + static_cast<size_t>(g_cap) * sizeof(Entry*); }

int file_records(const uint8_t* r) { return stored(r) ? 1 + count(r) : 1; }

void file_record(const uint8_t* r, uint8_t* out, size_t stride)
{
    memset(out, 0, stride);
    memcpy(out, r, kRec < stride ? kRec : stride);
    if (!stored(r)) return;
    set_id(out, 0);
    out[0x39] = static_cast<uint8_t>(count(r));
}

uint32_t read_scrolls(uint8_t* r, dax::ByteSource& src, uint32_t at, uint32_t stride)
{
    const int n = r[0x39];
    Entry* e = n > 0 && n <= kMax && stride >= kRec ? fresh() : nullptr;
    if (e) {
        for (int k = 0; k < n; ++k)
            if (src.read_at(at + k * stride, e->rec[e->n], kRec) == kRec) ++e->n;
        set_id(r, e->id);
        r[0x39] = e->n;
    } else {
        set_id(r, 0);
    }
    return static_cast<uint32_t>(n) * stride;
}

bool remove_spell(uint8_t* r, int s, uint8_t count_at, uint8_t one_spell)
{
    Entry* e = entry(r);
    if (!e) return false;
    for (int k = 0; k < e->n; ++k) {
        uint8_t* x = e->rec[k];
        int line = -1;
        for (int j = 0; j < 3; ++j)
            if ((x[0x3C + j] & 0x7F) == s) line = j;
        if (line < 0) continue;
        x[0x3C + line] = 0;
        x[count_at] = static_cast<uint8_t>(x[count_at] - 1);
        if (x[count_at] >= one_spell) return false;
        // Used up: out of the bundle (the bundle's words, weight and value
        // as they were - GAME.OVR 0x32B7C)
        memmove(e->rec[k], e->rec[k + 1], (e->n - k - 1) * kRec);
        --e->n;
        r[0x39] = e->n;
        if (e->n == 1) {
            memcpy(r, e->rec[0], kRec);
            drop(e);
            return true;
        }
        return false;
    }
    return false;
}

}  // namespace bundle
