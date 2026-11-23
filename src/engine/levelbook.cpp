#include "engine/levelbook.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>


namespace levelbook {
namespace {

const Store* g_store = nullptr;
constexpr size_t kMax = 24 * 1024;      // (past it the oldest lines go)

char* load(size_t* n_out)
{
    *n_out = 0;
    return g_store && g_store->load ? g_store->load(n_out) : nullptr;
}

void save(const char* text, size_t n)
{
    if (!g_store || !g_store->save) return;
    while (n > kMax) {                  // (the oldest lines dropped)
        const char* nl = static_cast<const char*>(memchr(text, '\n', n));
        if (!nl) return;
        n -= static_cast<size_t>(nl + 1 - text);
        text = nl + 1;
    }
    g_store->save(text, n);
}

// A line's fields (tab-separated) into out; how many
int fields(const char* line, size_t len, char out[6][24])
{
    int k = 0;
    size_t j = 0;
    for (size_t i = 0; i <= len && k < 6; ++i) {
        if (i == len || line[i] == '\t') {
            out[k][j] = 0;
            ++k;
            j = 0;
        } else if (j < 23) {
            out[k][j++] = line[i];
        }
    }
    return k;
}

// Every line of the text, called with its place: f(line, len, fields, n)
template <class F> void each(const char* text, F f)
{
    const char* p = text;
    while (*p) {
        const char* e = strchr(p, '\n');
        const size_t len = e ? static_cast<size_t>(e - p) : strlen(p);
        char fl[6][24];
        const int n = fields(p, len, fl);
        f(p, len, fl, n);
        if (!e) break;
        p = e + 1;
    }
}

} // namespace

void set_store(const Store* s) { g_store = s; }
bool have_file() { return g_store != nullptr; }

int hp(const char* name, int cls, int level)
{
    size_t n;
    char* text = load(&n);
    if (!text) return -1;
    int v = -1;
    each(text, [&](const char*, size_t, char (*f)[24], int k) {
        if (k >= 5 && f[0][0] == 'H' && strcmp(f[1], name) == 0 && atoi(f[2]) == cls && atoi(f[3]) == level) v = atoi(f[4]);
    });
    free(text);
    return v;
}

void set_hp(const char* name, int cls, int level, int hp)
{
    if (!g_store || !name || !name[0]) return;
    size_t n;
    char* text = load(&n);
    char line[80];
    const int ln = snprintf(line, sizeof line, "H\t%s\t%d\t%d\t%d\n", name, cls, level, hp < 0 ? 0 : hp);
    char* out = static_cast<char*>(malloc(n + static_cast<size_t>(ln) + 1));
    if (!out) {
        free(text);
        return;
    }
    size_t m = 0;
    if (text) {                         // (the old line for that level left out)
        each(text, [&](const char* p, size_t len, char (*f)[24], int k) {
            if (k >= 5 && f[0][0] == 'H' && strcmp(f[1], name) == 0 && atoi(f[2]) == cls && atoi(f[3]) == level) return;
            if (!len) return;
            memcpy(out + m, p, len);
            m += len;
            out[m++] = '\n';
        });
        free(text);
    }
    memcpy(out + m, line, static_cast<size_t>(ln));
    m += static_cast<size_t>(ln);
    save(out, m);
    free(out);
}

void push_drain(const char* name, const Drain& d)
{
    if (!g_store || !g_store->append || !name || !name[0]) return;
    char line[80];
    const int ln = snprintf(line, sizeof line, "D\t%s\t%d\t%d\t%d\t%d\n", name, d.cls, d.level, d.hp, d.former ? 1 : 0);
    g_store->append(line, static_cast<size_t>(ln));
}

namespace {
// The text less the drain lines `drop` says go (by their place in the file)
template <class P> void rewrite(const char* name, P drop)
{
    size_t n;
    char* text = load(&n);
    if (!text) return;
    char* out = static_cast<char*>(malloc(n + 1));
    if (!out) {
        free(text);
        return;
    }
    size_t m = 0;
    int idx = 0;
    each(text, [&](const char* p, size_t len, char (*f)[24], int k) {
        if (!len) return;
        if (k >= 6 && f[0][0] == 'D' && strcmp(f[1], name) == 0 && drop(idx++)) return;
        memcpy(out + m, p, len);
        m += len;
        out[m++] = '\n';
    });
    free(text);
    save(out, m);
    free(out);
}
} // namespace

bool pop_drain(const char* name, bool (*valid)(const Drain&, void*), void* user, Drain* out)
{
    size_t n;
    char* text = load(&n);
    if (!text) return false;
    int idx = 0, found = -1;
    Drain best;
    each(text, [&](const char*, size_t len, char (*f)[24], int k) {
        if (!len || k < 6 || f[0][0] != 'D' || strcmp(f[1], name) != 0) return;
        Drain d;
        d.cls = static_cast<uint8_t>(atoi(f[2]));
        d.level = static_cast<uint8_t>(atoi(f[3]));
        d.hp = static_cast<uint8_t>(atoi(f[4]));
        d.former = atoi(f[5]) != 0;
        if (!valid || valid(d, user)) {         // (the latest one that fits)
            found = idx;
            best = d;
        }
        ++idx;
    });
    free(text);
    if (found < 0) return false;
    rewrite(name, [&](int i) { return i == found; });
    if (out) *out = best;
    return true;
}

void clear_drains(const char* name)
{
    if (!g_store || !name || !name[0]) return;
    rewrite(name, [](int) { return true; });
}

} // namespace levelbook
