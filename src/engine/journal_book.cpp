// The typeset journals (the Steam / SNEG PDFs): their text as a book
// (BOOK.TXT) and their numbered sections (journal.h, make_book).
#include "journal.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <new>

#include "pdf.h"
#include "pdf_text.h"

namespace journal {

namespace {

void put16(uint8_t* p, unsigned v)
{
    p[0] = static_cast<uint8_t>(v);
    p[1] = static_cast<uint8_t>(v >> 8);
}
void put32(uint8_t* p, uint32_t v)
{
    for (int i = 0; i < 4; ++i) p[i] = static_cast<uint8_t>(v >> (8 * i));
}
uint32_t get32(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | static_cast<uint32_t>(p[3]) << 24; }

char lower(char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c + 32) : c; }
bool is_lower(char c) { return c >= 'a' && c <= 'z'; }
bool is_letter(char c) { return is_lower(lower(c)); }
bool is_digit(char c) { return c >= '0' && c <= '9'; }

// Text written out in pieces of 512
struct Writer {
    Output*  o = nullptr;
    uint32_t pos = 0;                 // where the next byte goes
    uint8_t  buf[512];
    int      n = 0;
    bool     ok = true;
    void flush()
    {
        if (n && ok) ok = o->write_at(pos - static_cast<uint32_t>(n), buf, static_cast<size_t>(n));
        n = 0;
    }
    void put(const char* s, int len)
    {
        for (int i = 0; i < len; ++i) {
            if (n == static_cast<int>(sizeof buf)) flush();
            buf[n++] = static_cast<uint8_t>(s[i]);
            ++pos;
        }
    }
};

// A page's text sizes: where each one's lowest line is, how much of it
constexpr int kSizes = 16;
struct Stats {
    float    size[kSizes];
    float    min_y[kSizes];
    float    max_y[kSizes];
    uint32_t chars[kSizes];
    int      n = 0;
    void add(float s, float y, int len)
    {
        for (int i = 0; i < n; ++i)
            if (std::fabs(size[i] - s) < 0.05f) {
                if (y < min_y[i]) min_y[i] = y;
                if (y > max_y[i]) max_y[i] = y;
                chars[i] += static_cast<uint32_t>(len);
                return;
            }
        if (n == kSizes) return;
        size[n] = s;
        min_y[n] = y;
        max_y[n] = y;
        chars[n] = static_cast<uint32_t>(len);
        ++n;
    }
    // The lowest line of this size (or of any size close to it)
    float bottom(float s) const
    {
        float b = 1e9f;
        for (int i = 0; i < n; ++i)
            if (std::fabs(size[i] - s) <= 0.35f && min_y[i] < b) b = min_y[i];
        return b;
    }
    // The highest line of this size (or close to it)
    float top(float s) const
    {
        float t = -1e9f;
        for (int i = 0; i < n; ++i)
            if (std::fabs(size[i] - s) <= 0.35f && max_y[i] > t) t = max_y[i];
        return t;
    }
    // The size most of the page's text has
    float body() const
    {
        int best = -1;
        for (int i = 0; i < n; ++i)
            if (best < 0 || chars[i] > chars[best]) best = i;
        return best >= 0 ? size[best] : 0;
    }
};

// A page number ("13 14", the spread's two): no part of the text
bool page_number(const pdf::TextLine& l)
{
    for (int i = 0; i < l.len; ++i)
        if (!is_digit(l.text[i]) && l.text[i] != ' ') return false;
    return true;
}

// The last line a paragraph took
struct Prev {
    bool  have = false;
    float x = 0, y = 0, size = 0;
    int   page = 0;
    bool  bottom = false;             // at the bottom of its column (the text may go on in the next)
    bool  open = false;               // its sentence not ended (a word broken, no full stop)
    bool  solo = false;               // a heading alone on its line: the next line starts a paragraph
};

bool same_size(float a, float b) { return std::fabs(a - b) <= 0.35f; }

// How line l goes on from p: 0 - not (other text), 1 - a new paragraph of
// the same text, 2 - the same paragraph, 3 - the same paragraph in the
// next column / page (or a new one there: a heading can start it)
int flow(const Prev& p, const pdf::TextLine& l, int page, bool top)
{
    if (!p.have) return 0;
    const float em = l.size > 1 ? l.size : 1;
    const float dy = p.y - l.y, dx = l.x - p.x;
    // Further down the same column (the text goes on under a picture;
    // lines made short by a picture beside them start further right)
    if (page == p.page && dy > 0.3f * em && std::fabs(dx) <= 6 * em) {
        if (!same_size(p.size, l.size)) return 1;
        // (a gap; an indented first line - not a line moved right by a
        // picture beside it mid-sentence)
        if (dy > 1.45f * em || (dx > 0.4f * em && (dx < 2 * em || !p.open))) return 1;
        return 2;
    }
    // The next column or page: when the last line ended its column - or,
    // in the next column of the same page, stopped short of it (a picture
    // under it) and l starts that column; on the next page too when the
    // last line stopped mid-sentence
    if (!same_size(p.size, l.size)) return 0;
    if (page == p.page && l.y > p.y + em && l.x > p.x + 6 * em && (p.bottom || top)) return 3;
    if (page != p.page && (p.bottom || (p.open && top))) return 3;
    return 0;
}

bool near_bottom(const pdf::TextLine& l, const Stats& st) { return l.y - st.bottom(l.size) <= 2.6f * l.size; }
bool near_top(const pdf::TextLine& l, const Stats& st) { return st.top(l.size) - l.y <= 2.6f * l.size; }
bool open_end(const pdf::TextLine& l)
{
    const char c = l.len ? l.text[l.len - 1] : '.';
    return !(c == '.' || c == '!' || c == '?' || c == '"' || c == '\'' || c == ')' || c == ':');
}
Prev prev_of(const pdf::TextLine& l, int page, bool bottom)
{
    Prev p;
    p.have = true;
    p.x = l.x;
    p.y = l.y;
    p.size = l.size;
    p.page = page;
    p.bottom = bottom;
    p.open = open_end(l);
    return p;
}

// Lines into paragraphs: '\n' between paragraphs, a line's words joined
// to the last's with a space, a word broken at the line's end ("ship-" /
// "ripping") joined again
struct Join {
    Writer* w = nullptr;
    Prev    p;
    bool    any = false;              // a paragraph written
    bool    hyphen = false;           // the last line's '-' held back
    void reset()
    {
        p = Prev{};
        any = false;
        hyphen = false;
    }
    void add(const pdf::TextLine& l, bool same_para, int page, bool bottom)
    {
        int len = l.len;
        if (p.solo) same_para = false;
        if (any) {
            if (same_para) {
                if (hyphen && !is_lower(l.text[0])) w->put("-", 1);
                if (!hyphen) w->put(" ", 1);
            } else {
                if (hyphen) w->put("-", 1);
                w->put("\n", 1);
            }
        }
        hyphen = len >= 2 && l.text[len - 1] == '-' && is_lower(l.text[len - 2]);
        if (hyphen) --len;
        w->put(l.text, len);
        any = true;
        p = prev_of(l, page, bottom);
    }
    void end()
    {
        if (hyphen) w->put("-", 1);
        hyphen = false;
    }
};

bool heading_alone(const char* s, int n);

// Pass 1: the page's sizes
struct StatsSink : pdf::TextSink {
    Stats st;
    bool line(const pdf::TextLine& l) override
    {
        if (!page_number(l)) st.add(l.size, l.y, l.len);
        return true;
    }
};

// Pass 2: the book's paragraphs and the sections
struct Book : pdf::TextSink {
    Writer  book, sect;
    Join    bj, sj;
    Stats   st;
    int     page = 0;
    Section* secs = nullptr;
    int     max = 0;
    BookInfo* info = nullptr;
    // The section being gathered
    bool    open = false;
    bool    keep = false;             // (false: a heading seen again - its text passed over)
    float   body = 0;
    bool    in_body = false;          // its first body-size line taken (before: the heading's sub-heads)

    void close()
    {
        if (!open) return;
        open = false;
        if (!keep) return;
        sj.end();
        Section& s = secs[info->sections - 1];
        s.len = sect.pos - s.at;
        if (s.len == 0) --info->sections;
    }
    bool line(const pdf::TextLine& l) override
    {
        if (page_number(l)) return true;
        const bool bottom = near_bottom(l, st);
        // The book
        const bool top = near_top(l, st);
        const int f = flow(bj.p, l, page, top);
        // A section's heading? (a paragraph's first line)
        char kind = 0;
        int num = 0;
        const bool heading = f != 2 && section_heading(l.text, l.len, &kind, &num);
        bj.add(l, f >= 2 && !heading, page, bottom);
        if (heading) {
            bj.p.solo = heading_alone(l.text, l.len);
            close();
            open = true;
            bool dup = false;
            for (int i = 0; i < info->sections && !dup; ++i) dup = secs[i].kind == kind && secs[i].number == num;
            keep = !dup && info->sections < max && num > 0 && num < 65536;
            body = st.body();
            sj.reset();
            if (keep) {
                Section& s = secs[info->sections++];
                s.kind = kind;
                s.number = static_cast<uint16_t>(num);
                s.at = sect.pos;
                s.len = 0;
                const char* k = "JTPI";
                for (int i = 0; i < 4; ++i)
                    if (k[i] == kind) ++info->count[i];
            }
            // ("Tale 3: Wells throughout ..." - the heading on the text's
            // own line: the text started)
            in_body = same_size(l.size, body);
            sj.p = Prev{};
            if (keep) sj.add(l, false, page, bottom);
            else sj.p = prev_of(l, page, bottom);
            sj.p.solo = bj.p.solo;
            return true;
        }
        if (!open) return true;
        const bool is_body = same_size(l.size, body);
        int g = flow(sj.p, l, page, top);
        if (!in_body) {
            // Under the heading: its sub-heads (any size) straight below
            const float dy = sj.p.y - l.y;
            if (page == sj.p.page && dy > 0 && dy <= 2.8f * (l.size > sj.p.size ? l.size : sj.p.size) &&
                std::fabs(l.x - sj.p.x) <= 4 * l.size)
                g = g >= 2 ? 2 : 1;
        } else if (!is_body) {
            return true;                  // (a caption, a page's heading: not the section's)
        }
        if (g == 0) {
            close();
            return true;
        }
        if (is_body) in_body = true;
        if (keep) {
            sj.add(l, g >= 2, page, bottom);
            if (sect.pos - secs[info->sections - 1].at > 12000) close();      // (something went wrong: enough)
        } else {
            sj.p = prev_of(l, page, bottom);
        }
        return true;
    }
};

bool word_ci(const char* s, int n, int* i, const char* w)
{
    int k = *i;
    for (; *w; ++w, ++k)
        if (k >= n || lower(s[k]) != *w) return false;
    if (k < n && is_letter(s[k])) return false;
    *i = k;
    return true;
}
void spaces(const char* s, int n, int* i)
{
    while (*i < n && (s[*i] == ' ' || s[*i] == '#')) ++*i;
}
int digits(const char* s, int n, int* i)
{
    int v = 0, k = 0;
    while (*i < n && is_digit(s[*i]) && k < 5) v = v * 10 + (s[(*i)++] - '0'), ++k;
    if (k == 0 || (*i < n && (is_digit(s[*i]) || is_letter(s[*i])))) return -1;
    return v;
}
// After the number: the line's end or a ':' (the text may follow)
bool heading_end(const char* s, int n, int i, bool colon)
{
    while (i < n && s[i] == ' ') ++i;
    if (i >= n) return !colon;
    return s[i] == ':';
}

bool parse_heading(const char* s, int n, char* kind, int* number, int* end)
{
    int i = 0;
    while (i < n && (s[i] == ' ' || s[i] == '*')) ++i;
    const int start = i;
    if (word_ci(s, n, &i, "journal")) {
        spaces(s, n, &i);
        if (!word_ci(s, n, &i, "entry")) return false;
        spaces(s, n, &i);
        const int v = digits(s, n, &i);
        if (v <= 0 || !heading_end(s, n, i, false)) return false;
        *kind = 'J';
        *number = v;
        *end = i;
        return true;
    }
    i = start;
    bool tavern = word_ci(s, n, &i, "tavern");
    spaces(s, n, &i);
    if (word_ci(s, n, &i, "tale")) {
        spaces(s, n, &i);
        if (word_ci(s, n, &i, "number") || word_ci(s, n, &i, "no")) {
            if (i < n && s[i] == '.') ++i;
            spaces(s, n, &i);
        }
        const int v = digits(s, n, &i);
        // ("Tale 3:" - bare "Tale" only with its colon, not "tale 3 of ...")
        if (v <= 0 || !heading_end(s, n, i, !tavern)) {
            if (!(tavern && v > 0 && heading_end(s, n, i, true))) return false;
        }
        *kind = 'T';
        *number = v;
        *end = i;
        return true;
    }
    i = start;
    if (word_ci(s, n, &i, "proclamation")) {
        spaces(s, n, &i);
        const int r0 = i;
        while (i < n && is_letter(s[i])) ++i;
        const int v = roman_number(s + r0, i - r0);
        if (v <= 0 || !heading_end(s, n, i, false)) {
            if (!(v > 0 && heading_end(s, n, i, true))) return false;
        }
        *kind = 'P';
        *number = v;
        *end = i;
        return true;
    }
    i = start;
    if (word_ci(s, n, &i, "the")) spaces(s, n, &i);
    if (word_ci(s, n, &i, "introduction") && heading_end(s, n, i, false)) {
        *kind = 'I';
        *number = 1;
        *end = i;
        return true;
    }
    return false;
}

// The heading is the whole line ("Proclamation LIX", "Journal Entry 7:" -
// not "Tale 3: Wells throughout ...")
bool heading_alone(const char* s, int n)
{
    char kind;
    int num, i = 0;
    if (!parse_heading(s, n, &kind, &num, &i)) return false;
    while (i < n && (s[i] == ' ' || s[i] == ':')) ++i;
    return i >= n;
}

} // namespace

bool section_heading(const char* s, int n, char* kind, int* number)
{
    int end = 0;
    return parse_heading(s, n, kind, number, &end);
}

bool make_book(dax::ByteSource& src, Output& book_out, Output& sect_out, Section* sections, int max, BookInfo& info,
               Progress progress, void* ctx)
{
    info = BookInfo{};
    pdf::Doc* doc = new (std::nothrow) pdf::Doc;
    int* pages = static_cast<int*>(malloc(sizeof(int) * pdf::kMaxPages));
    uint32_t* more = nullptr;
    bool no_mem = false;
    bool ok = doc && pages && pdf::open_large(src, *doc, &more, &no_mem);
    const int n = ok ? pdf::pages(src, *doc, pages, pdf::kMaxPages) : 0;
    uint32_t* offs = n > 0 ? static_cast<uint32_t*>(calloc(static_cast<size_t>(n + 1), 4)) : nullptr;
    pdf::TextReader* tr = n > 0 && offs ? new (std::nothrow) pdf::TextReader : nullptr;
    Book* bk = tr && tr->ok() ? new (std::nothrow) Book : nullptr;
    StatsSink* ss = bk ? new (std::nothrow) StatsSink : nullptr;
    if (bk) doc->window = tr->window();       // (object streams inflate in the reader's window)
    info.no_memory = !doc || !pages || no_mem || (n > 0 && !ss);
    ok = ok && n > 0 && ss;
    if (ok) {
        Book& b = *bk;
        b.book.o = &book_out;
        b.book.pos = kBookHeader + 4u * static_cast<uint32_t>(n + 1);
        b.sect.o = &sect_out;
        b.bj.w = &b.book;
        b.sj.w = &b.sect;
        b.secs = sections;
        b.max = max;
        b.info = &info;
        for (int p = 0; p < n && b.book.ok && b.sect.ok; ++p) {
            offs[p] = b.book.pos;
            ss->st = Stats{};
            const int got = tr->page(src, *doc, pages[p], *ss);
            if (got < 0) {
                ++info.failed;
                if (tr->no_memory()) info.no_memory = true;
                continue;
            }
            b.st = ss->st;
            b.page = p + 1;
            b.bj.reset();
            tr->page(src, *doc, pages[p], b);
            b.bj.end();
            ++info.pages;
            if (progress) progress(p + 1, n, ctx);
        }
        b.close();
        offs[n] = b.book.pos;
        b.book.flush();
        b.sect.flush();
        info.chars = b.book.pos - (kBookHeader + 4u * static_cast<uint32_t>(n + 1));
        ok = b.book.ok && b.sect.ok && info.pages > 0;
        if (ok) {
            uint8_t h[kBookHeader] = {'G', 'B', 'B', '1', 1, 0, 0, 0};
            put16(h + 6, static_cast<unsigned>(n));
            uint8_t o4[4];
            ok = book_out.write_at(0, h, sizeof h);
            for (int i = 0; ok && i <= n; ++i) {
                put32(o4, offs[i]);
                ok = book_out.write_at(kBookHeader + 4u * static_cast<uint32_t>(i), o4, 4);
            }
        }
    }
    delete ss;
    delete bk;
    delete tr;
    free(offs);
    free(more);
    free(pages);
    delete doc;
    return ok;
}

int book_pages(dax::ByteSource& book)
{
    uint8_t h[kBookHeader];
    if (book.read_at(0, h, sizeof h) != sizeof h || memcmp(h, "GBB1", 4) != 0 || h[4] != 1) return 0;
    const int n = h[6] | h[7] << 8;
    return kBookHeader + 4u * static_cast<uint32_t>(n + 1) <= book.size() ? n : 0;
}

bool book_page(dax::ByteSource& book, int p, uint32_t* at, uint32_t* len)
{
    const int n = book_pages(book);
    if (p < 1 || p > n) return false;
    uint8_t o[8];
    if (book.read_at(kBookHeader + 4u * static_cast<uint32_t>(p - 1), o, 8) != 8) return false;
    const uint32_t a = get32(o), b = get32(o + 4);
    if (b < a || b > book.size()) return false;
    *at = a;
    *len = b - a;
    return true;
}

} // namespace journal
