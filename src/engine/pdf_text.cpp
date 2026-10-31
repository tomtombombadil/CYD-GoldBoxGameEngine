#include "pdf_text.h"

#include "inflate.h"
#include "pdf_internal.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <new>

namespace pdf {

using namespace detail;

// ---- characters ---------------------------------------------------------------

namespace {

// Latin-1's letters without their accents (0xC0 - 0xFF)
const char kLatin1[] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYTsaaaaaaaceeeeiiiidnooooo/ouuuuyty";

// WinAnsi's 0x80 - 0x9F (0: unused)
const uint16_t kWinAnsi80[32] = {0x20AC, 0, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160,
                                 0x2039, 0x0152, 0, 0x017D, 0, 0, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013,
                                 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0, 0x017E, 0x0178};

uint16_t win_ansi(int code)
{
    if (code >= 0x80 && code < 0xA0) return kWinAnsi80[code - 0x80];
    return static_cast<uint16_t>(code);
}

// Glyph names (a font's /Differences) -> Unicode: the ones the journals'
// text uses; single letters and "uniXXXX" / accented letters by rule
struct GlyphName {
    const char* name;
    uint16_t    u;
};
const GlyphName kGlyphs[] = {
    {"space", 0x20}, {"exclam", '!'}, {"quotedbl", '"'}, {"numbersign", '#'}, {"dollar", '$'}, {"percent", '%'},
    {"ampersand", '&'}, {"quotesingle", '\''}, {"parenleft", '('}, {"parenright", ')'}, {"asterisk", '*'},
    {"plus", '+'}, {"comma", ','}, {"hyphen", '-'}, {"period", '.'}, {"slash", '/'}, {"zero", '0'}, {"one", '1'},
    {"two", '2'}, {"three", '3'}, {"four", '4'}, {"five", '5'}, {"six", '6'}, {"seven", '7'}, {"eight", '8'},
    {"nine", '9'}, {"colon", ':'}, {"semicolon", ';'}, {"less", '<'}, {"equal", '='}, {"greater", '>'},
    {"question", '?'}, {"at", '@'}, {"bracketleft", '['}, {"backslash", '\\'}, {"bracketright", ']'},
    {"asciicircum", '^'}, {"underscore", '_'}, {"grave", '`'}, {"braceleft", '{'}, {"bar", '|'}, {"braceright", '}'},
    {"asciitilde", '~'}, {"quoteleft", 0x2018}, {"quoteright", 0x2019}, {"quotedblleft", 0x201C},
    {"quotedblright", 0x201D}, {"quotesinglbase", 0x201A}, {"quotedblbase", 0x201E}, {"endash", 0x2013},
    {"emdash", 0x2014}, {"ellipsis", 0x2026}, {"bullet", 0x2022}, {"periodcentered", 0xB7}, {"fi", 0xFB01},
    {"fl", 0xFB02}, {"ff", 0xFB00}, {"ffi", 0xFB03}, {"ffl", 0xFB04}, {"nbspace", 0xA0}, {"sfthyphen", 0xAD},
    {"germandbls", 0xDF}, {"ae", 0xE6}, {"AE", 0xC6}, {"oe", 0x153}, {"OE", 0x152}, {"dagger", 0x2020},
    {"daggerdbl", 0x2021}, {"trademark", 0x2122}, {"copyright", 0xA9}, {"registered", 0xAE}, {"degree", 0xB0},
    {"section", 0xA7}, {"paragraph", 0xB6}, {"multiply", 0xD7}, {"divide", 0xF7}, {"minus", 0x2212},
    {"guillemotleft", 0xAB}, {"guillemotright", 0xBB}, {"exclamdown", 0xA1}, {"questiondown", 0xBF},
    {"onehalf", 0xBD}, {"onequarter", 0xBC}, {"threequarters", 0xBE}, {"dotlessi", 0x131},
};

int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

uint16_t glyph_unicode(const char* nm, size_t n)
{
    char name[40];
    if (n == 0 || n >= sizeof name) return 0;
    memcpy(name, nm, n);
    name[n] = 0;
    char* dot = strchr(name, '.');            // ("a.sc", "T.alt": the plain glyph)
    if (dot && dot != name) *dot = 0;
    n = strlen(name);
    // Ligatures named by their parts ("f_i")
    if (strcmp(name, "f_f") == 0) return 0xFB00;
    if (strcmp(name, "f_i") == 0) return 0xFB01;
    if (strcmp(name, "f_l") == 0) return 0xFB02;
    if (strcmp(name, "f_f_i") == 0) return 0xFB03;
    if (strcmp(name, "f_f_l") == 0) return 0xFB04;
    char* us = strchr(name, '_');
    if (us && us != name) {
        *us = 0;
        n = strlen(name);
    }
    if (n == 1 && ((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z'))) return static_cast<uint8_t>(name[0]);
    for (const GlyphName& g : kGlyphs)
        if (strcmp(g.name, name) == 0) return g.u;
    if ((n == 7 || n == 5 || n == 6) && (memcmp(name, "uni", 3) == 0 || name[0] == 'u')) {
        const char* h = name[0] == 'u' && name[1] == 'n' && name[2] == 'i' ? name + 3 : name + 1;
        uint32_t v = 0;
        int k = 0;
        for (; h[k]; ++k) {
            const int d = hexval(h[k]);
            if (d < 0) break;
            v = v << 4 | static_cast<uint32_t>(d);
        }
        if (!h[k] && k >= 4 && v < 0x10000) return static_cast<uint16_t>(v);
    }
    // An accented letter ("eacute", "Odieresis"): its letter
    static const char* const kAccents[] = {"acute", "grave", "circumflex", "dieresis", "tilde", "ring", "cedilla", "caron",
                                           "macron", "breve", "ogonek", "dotaccent", "hungarumlaut", "slash", "stroke"};
    if (n > 1 && ((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z')))
        for (const char* a : kAccents)
            if (strcmp(name + 1, a) == 0) return static_cast<uint8_t>(name[0]);
    return 0;
}

// The code points a ToUnicode destination lists -> one: ligatures by their
// letters, else the first
uint16_t pick(const uint32_t* cp, int n)
{
    if (n <= 0) return 0;
    if (n >= 2 && cp[0] == 'f') {
        if (n == 2 && cp[1] == 'f') return 0xFB00;
        if (n == 2 && cp[1] == 'i') return 0xFB01;
        if (n == 2 && cp[1] == 'l') return 0xFB02;
        if (n == 3 && cp[1] == 'f' && cp[2] == 'i') return 0xFB03;
        if (n == 3 && cp[1] == 'f' && cp[2] == 'l') return 0xFB04;
    }
    return cp[0] < 0x10000 ? static_cast<uint16_t>(cp[0]) : 0;
}

} // namespace

int fold(uint32_t u, char out[4])
{
    int n = 0;
    auto put = [&](const char* s) {
        while (*s && n < 3) out[n++] = *s++;
    };
    if (u >= 0x20 && u < 0x7F) {
        out[n++] = static_cast<char>(u);
    } else if (u == 0xA0 || u == 0x2002 || u == 0x2003 || u == 0x2009 || u == 0x2007 || u == 0x2008 || u == 0x200A ||
               u == 0x2000 || u == 0x2001 || u == 0x2004 || u == 0x2005 || u == 0x2006 || u == 0x202F || u == 0x09) {
        out[n++] = ' ';
    } else if (u >= 0xC0 && u <= 0xFF) {
        if (u == 0xC6) put("AE");
        else if (u == 0xE6) put("ae");
        else if (u == 0xDF) put("ss");
        else out[n++] = kLatin1[u - 0xC0];
    } else {
        switch (u) {
        case 0x2018: case 0x2019: case 0x201A: case 0x201B: case 0x2032: case 0xB4: out[n++] = '\''; break;
        case 0x201C: case 0x201D: case 0x201E: case 0x201F: case 0x2033: case 0xAB: case 0xBB: out[n++] = '"'; break;
        case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2212: case 0xAD: out[n++] = '-'; break;
        case 0x2014: case 0x2015: put("--"); break;
        case 0x2026: put("..."); break;
        case 0x2022: case 0xB7: case 0x25CF: case 0x2023: out[n++] = '*'; break;
        case 0xFB00: put("ff"); break;
        case 0xFB01: put("fi"); break;
        case 0xFB02: put("fl"); break;
        case 0xFB03: put("ffi"); break;
        case 0xFB04: put("ffl"); break;
        case 0x152: put("OE"); break;
        case 0x153: put("oe"); break;
        case 0x131: out[n++] = 'i'; break;
        case 0xD7: out[n++] = 'x'; break;
        case 0xBD: put("1/2"); break;
        case 0xBC: put("1/4"); break;
        case 0xBE: put("3/4"); break;
        case 0xB0: put(" deg"); break;
        case 0xA9: put("(c)"); break;
        case 0xAE: put("(R)"); break;
        case 0x2122: put("TM"); break;
        case 0xA1: out[n++] = '!'; break;
        case 0xBF: out[n++] = '?'; break;
        case 0x160: out[n++] = 'S'; break;
        case 0x161: out[n++] = 's'; break;
        case 0x17D: out[n++] = 'Z'; break;
        case 0x17E: out[n++] = 'z'; break;
        case 0x178: out[n++] = 'Y'; break;
        case 0x192: out[n++] = 'f'; break;
        default: break;           // (dingbats, the journals' check boxes: nothing)
        }
    }
    out[n] = 0;
    return n;
}

// ---- the reader ------------------------------------------------------------------

namespace {

constexpr int kFonts = 24;                // a page's fonts (and the cache across pages)
constexpr int kMaxCids = 2048;            // a composite font's codes kept
constexpr int kTok = 1024;                // a string / token's bytes (longer ones cut)
constexpr int kLine = 600;                // a line's characters

struct Cid {
    uint16_t code, u, w;
};

struct Font {
    int      obj = 0;                     // the font's object number (0: free)
    bool     two = false;                 // two-byte codes (Type0)
    bool     marks = false;               // dingbats: drawn, not read
    uint16_t dw = 1000;                   // composite: /DW
    uint16_t u[256] = {};                 // simple: code -> Unicode
    uint16_t w[256] = {};                 // simple: widths (1/1000 of the size)
    Cid*     cid = nullptr;               // composite: sorted by code
    int      n_cid = 0;
    unsigned used = 0;                    // (the cache's age)
    ~Font() { free(cid); }
};

struct Mat {
    float a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;
};
Mat mul(const Mat& m, const Mat& n)       // m then n (PDF's row vectors: m x n)
{
    Mat r;
    r.a = m.a * n.a + m.b * n.c;
    r.b = m.a * n.b + m.b * n.d;
    r.c = m.c * n.a + m.d * n.c;
    r.d = m.c * n.b + m.d * n.d;
    r.e = m.e * n.a + m.f * n.c + n.e;
    r.f = m.e * n.b + m.f * n.d + n.f;
    return r;
}

// Numbers from a span of text ("[ 220 220 280 ... ]"): calls each(v) for
// numbers, open() / close() for nested '[' ']', name(s, n) for "/name"
template <class F>
void scan_span(const char* t, size_t n, F&& f)
{
    size_t i = 0;
    int depth = 0;
    while (i < n) {
        const char c = t[i];
        if (c == '[') {
            if (depth++ > 0) f.open();
            ++i;
        } else if (c == ']') {
            if (--depth > 0) f.close();
            ++i;
        } else if (c == '/') {
            const size_t s = ++i;
            while (i < n && !is_ws(t[i]) && !is_delim(t[i])) ++i;
            f.name(t + s, i - s);
        } else if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.') {
            char num[24];
            size_t k = 0;
            while (i < n && k + 1 < sizeof num && ((t[i] >= '0' && t[i] <= '9') || t[i] == '-' || t[i] == '+' || t[i] == '.'))
                num[k++] = t[i++];
            num[k] = 0;
            f.number(strtod(num, nullptr));
        } else {
            ++i;
        }
    }
}

// The span of an array value (inline, or the object it refers to, loaded
// into tmp)
bool array_span(dax::ByteSource& src, const Doc& doc, const Obj& o, int node, Obj& tmp, const char** t, size_t* n)
{
    const Obj* ro = nullptr;
    int rn = -1;
    if (!deref(src, doc, o, node, tmp, &ro, &rn) || rn < 0 || ro->node[rn].t != T::Array) return false;
    *t = ro->buf + ro->node[rn].s;
    *n = ro->node[rn].n;
    return true;
}

// ---- a ToUnicode CMap ---------------------------------------------------------

struct CMap : inflate::Output {
    // set(code, unicode)
    void (*set)(void* ctx, uint32_t code, uint16_t u) = nullptr;
    void* ctx = nullptr;
    enum Mode { None, Char, Range } mode = None;
    char tok[80];
    int  tl = 0;
    bool in_hex = false, in_word = false, in_arr = false;
    uint32_t vals[3][8];                  // up to 3 hex values' code points / bytes
    int  vn[3] = {}, nv = 0;
    uint32_t arr_code = 0, arr_hi = 0;    // a range with a list of destinations
    char word[24];
    int  wl = 0;

    static int decode_utf16(const char* h, int n, uint32_t* cp, int max)
    {
        // hex digits -> UTF-16BE units -> code points
        int k = 0;
        uint32_t hi = 0;
        for (int i = 0; i + 3 < n && k < max; i += 4) {
            uint32_t v = 0;
            for (int j = 0; j < 4; ++j) v = v << 4 | static_cast<uint32_t>(hexval(h[i + j]) & 15);
            if (v >= 0xD800 && v < 0xDC00) {
                hi = v;
                continue;
            }
            if (v >= 0xDC00 && v < 0xE000 && hi) {
                cp[k++] = 0x10000 + ((hi - 0xD800) << 10) + (v - 0xDC00);
                hi = 0;
                continue;
            }
            cp[k++] = v;
        }
        return k;
    }
    static uint32_t hex_number(const char* h, int n)
    {
        uint32_t v = 0;
        for (int i = 0; i < n && i < 8; ++i) v = v << 4 | static_cast<uint32_t>(hexval(h[i]) & 15);
        return v;
    }
    void hex_done()
    {
        tok[tl] = 0;
        if (mode == None) return;
        if (in_arr) {
            // a range's destinations one by one
            uint32_t cp[4];
            const int k = decode_utf16(tok, tl, cp, 4);
            if (arr_code <= arr_hi && set) set(ctx, arr_code, pick(cp, k));
            ++arr_code;
            return;
        }
        if (nv >= 3) return;
        if (mode == Char && nv == 1) {
            uint32_t cp[4];
            const int k = decode_utf16(tok, tl, cp, 4);
            if (set) set(ctx, vals[0][0], pick(cp, k));
            nv = 0;
            return;
        }
        vals[nv][0] = hex_number(tok, tl);
        vn[nv] = tl;
        if (mode == Range && nv == 2) {
            // <lo> <hi> <dst>: dst's last unit counts up
            uint32_t cp[4];
            const int k = decode_utf16(tok, tl, cp, 4);
            const uint32_t lo = vals[0][0], hi = vals[1][0];
            if (k >= 1 && hi >= lo && set)
                for (uint32_t c = lo; c <= hi && c - lo < 4096; ++c) {
                    uint32_t one[4];
                    memcpy(one, cp, sizeof(uint32_t) * static_cast<size_t>(k));
                    one[k - 1] += c - lo;
                    set(ctx, c, pick(one, k));
                }
            nv = 0;
            return;
        }
        ++nv;
    }
    void word_done()
    {
        word[wl] = 0;
        if (strcmp(word, "beginbfchar") == 0) mode = Char;
        else if (strcmp(word, "beginbfrange") == 0) mode = Range;
        else if (strncmp(word, "end", 3) == 0) mode = None;
        nv = 0;
        wl = 0;
    }
    bool put(uint8_t b) override
    {
        const char c = static_cast<char>(b);
        if (in_hex) {
            if (c == '>') {
                in_hex = false;
                hex_done();
            } else if (hexval(c) >= 0 && tl < static_cast<int>(sizeof tok) - 1) {
                tok[tl++] = c;
            }
            return true;
        }
        if (in_word) {
            if (is_ws(c) || is_delim(c)) {
                in_word = false;
                word_done();
            } else {
                if (wl < static_cast<int>(sizeof word) - 1) word[wl++] = c;
                return true;
            }
        }
        if (c == '<') {
            in_hex = true;
            tl = 0;
        } else if (c == '[') {
            if (mode == Range && nv == 2) {
                in_arr = true;
                arr_code = vals[0][0];
                arr_hi = vals[1][0];
            }
        } else if (c == ']') {
            if (in_arr) {
                in_arr = false;
                nv = 0;
            }
        } else if (!is_ws(c) && !is_delim(c)) {
            in_word = true;
            wl = 0;
            word[wl++] = c;
        }
        return true;
    }
};

} // namespace

// ---- content streams -------------------------------------------------------------

struct TextReader::Impl : inflate::Output {
    Obj*     a = nullptr;
    Obj*     b = nullptr;
    uint8_t* window = nullptr;
    Font*    font[kFonts] = {};
    unsigned clock = 0;
    bool     no_mem = false;

    // The page's fonts by their resource names
    struct Named {
        char name[24];
        int  obj;
        int  slot;                        // into font[] (-1: couldn't load)
    };
    Named    named[kFonts];
    int      n_named = 0;

    // The lexer
    enum Lex { L_None, L_Word, L_Name, L_Str, L_Hex, L_Comment, L_Skip } lex = L_None;
    char     tok[kTok];
    int      tl = 0;
    int      paren = 0;
    bool     esc = false;
    int      oct = 0, oct_n = 0;
    int      dict_depth = 0, arr_depth = 0;
    uint8_t  skip_hist[3] = {};           // an inline image: past "EI"
    bool     lt = false, gt = false;      // a '<' / '>' seen (for "<<" / ">>")

    // Operands
    float    num[8];
    int      nn = 0;
    char     last_name[24];

    // The state
    Mat      ctm;
    struct Saved {             // q / Q: the graphics state, the text state in it
        Mat ctm;
        Font* font;
        float fs, tc, tw, th, tl;
    } stack[16];
    int      sp = 0;
    Mat      tm, tlm;
    float    fs = 0, tc = 0, tw = 0, th = 1, tleading = 0;
    Font*    cur = nullptr;
    bool     in_bt = false;

    // The line being gathered
    char     line[kLine];
    int      ll = 0;
    float    lx = 0, ly = 0, lx_end = 0, lsize = 0;
    float    alt = 0;                     // the line's sizes: lsize's letters (n_main), another's (n_alt)
    int      n_main = 0, n_alt = 0;
    float    main_size() const { return n_alt > n_main ? alt : lsize; }
    bool     have_line = false;
    TextSink* sink = nullptr;
    bool     stop = false;
    int      lines = 0;

    ~Impl()
    {
        delete a;
        delete b;
        free(window);
        for (Font* f : font) delete f;
    }

    // ---- fonts
    static void set_simple(void* ctx, uint32_t code, uint16_t u)
    {
        Font* f = static_cast<Font*>(ctx);
        if (code < 256) f->u[code] = u;
    }
    struct CidBuild {
        Cid* c = nullptr;
        int  n = 0, cap = 0;
        bool full = false;
    };
    static void set_cid(void* ctx, uint32_t code, uint16_t u)
    {
        CidBuild* b = static_cast<CidBuild*>(ctx);
        if (code > 0xFFFF) return;
        if (b->n == b->cap) {
            if (b->cap >= kMaxCids) {
                b->full = true;
                return;
            }
            const int nc = b->cap ? b->cap * 2 : 128;
            Cid* m = static_cast<Cid*>(realloc(b->c, sizeof(Cid) * static_cast<size_t>(nc)));
            if (!m) {
                b->full = true;
                return;
            }
            b->c = m;
            b->cap = nc;
        }
        b->c[b->n++] = Cid{static_cast<uint16_t>(code), u, 0};
    }

    bool to_unicode(dax::ByteSource& src, const Doc& doc, int ref, void (*set)(void*, uint32_t, uint16_t), void* ctx)
    {
        if (ref <= 0 || !load(src, doc, ref, *a) || !a->stream_at) return false;
        CMap cm;
        cm.set = set;
        cm.ctx = ctx;
        return stream(src, doc, *a, *b, cm, window);
    }

    Font* load_font(dax::ByteSource& src, const Doc& doc, int obj)
    {
        Font* f = new (std::nothrow) Font;
        if (!f) {
            no_mem = true;
            return nullptr;
        }
        f->obj = obj;
        if (!load(src, doc, obj, *a)) {
            delete f;
            return nullptr;
        }
        const int st = get(*a, a->root, "Subtype");
        {
            // (a dingbat or ornament font: check boxes and flourishes, not text)
            const int bf = get(*a, a->root, "BaseFont");
            if (bf >= 0 && a->node[bf].t == T::Name) {
                char nm[64];
                const size_t k = a->node[bf].n < sizeof nm - 1 ? a->node[bf].n : sizeof nm - 1;
                memcpy(nm, a->buf + a->node[bf].s, k);
                nm[k] = 0;
                f->marks = strstr(nm, "Dingbat") || strstr(nm, "Ornament") || strstr(nm, "Wingding");
            }
        }
        int tu = get(*a, a->root, "ToUnicode");
        const int tu_ref = tu >= 0 && a->node[tu].t == T::Ref ? a->node[tu].v : 0;
        if (name_is(*a, st, "Type0")) {
            f->two = true;
            // The descendant: its /DW and /W
            const int dfn = get(*a, a->root, "DescendantFonts");
            int dref = 0;
            if (dfn >= 0 && a->node[dfn].t == T::Array && a->node[dfn].first >= 0 && a->node[a->node[dfn].first].t == T::Ref)
                dref = a->node[a->node[dfn].first].v;
            else if (dfn >= 0 && a->node[dfn].t == T::Ref && load(src, doc, a->node[dfn].v, *b) &&
                     b->node[b->root].t == T::Array && b->node[b->root].first >= 0 &&
                     b->node[b->node[b->root].first].t == T::Ref)
                dref = b->node[b->node[b->root].first].v;
            // Its codes from the ToUnicode map first (sorted), then the
            // descendant's /DW and /W for those codes only (no list of
            // every width: a /W range can name thousands)
            CidBuild cb;
            if (tu_ref) to_unicode(src, doc, tu_ref, set_cid, &cb);
            if (cb.full && !cb.c) no_mem = true;
            for (int i = 1; i < cb.n; ++i) {
                const Cid x = cb.c[i];
                int k = i;
                for (; k > 0 && cb.c[k - 1].code > x.code; --k) cb.c[k] = cb.c[k - 1];
                cb.c[k] = x;
            }
            if (dref && load(src, doc, dref, *a)) {
                const int dw = get(*a, a->root, "DW");
                if (dw >= 0 && a->node[dw].t == T::Num) f->dw = static_cast<uint16_t>(a->node[dw].v);
                for (int i = 0; i < cb.n; ++i) cb.c[i].w = f->dw;
                const int w = get(*a, a->root, "W");
                const char* t = nullptr;
                size_t tn = 0;
                if (w >= 0 && array_span(src, doc, *a, w, *b, &t, &tn)) {
                    struct WScan {
                        Cid* c;
                        int n;
                        double pend[3];
                        int np = 0;
                        int list_at = -1;             // in "c [w ...]": the next code
                        void set(uint32_t lo, uint32_t hi, uint16_t w)
                        {
                            // (the codes in [lo, hi]: from the first not below lo)
                            int a0 = 0, a1 = n;
                            while (a0 < a1) {
                                const int m = (a0 + a1) / 2;
                                if (c[m].code < lo) a0 = m + 1;
                                else a1 = m;
                            }
                            for (int i = a0; i < n && c[i].code <= hi; ++i) c[i].w = w;
                        }
                        void open()
                        {
                            if (np >= 1) list_at = static_cast<int>(pend[np - 1]);
                            np = 0;
                        }
                        void close() { list_at = -1; }
                        void name(const char*, size_t) {}
                        void number(double v)
                        {
                            const uint16_t w = static_cast<uint16_t>(v < 0 ? 0 : v > 60000 ? 60000 : v);
                            if (list_at >= 0) {
                                if (list_at <= 0xFFFF) set(static_cast<uint32_t>(list_at), static_cast<uint32_t>(list_at), w);
                                ++list_at;
                                return;
                            }
                            pend[np++] = v;
                            if (np == 3) {
                                // c1 c2 w
                                if (pend[0] >= 0 && pend[1] >= pend[0])
                                    set(static_cast<uint32_t>(pend[0]), static_cast<uint32_t>(pend[1] > 65535 ? 65535 : pend[1]), w);
                                np = 0;
                            }
                        }
                    } ws{cb.c, cb.n};
                    if (cb.n) scan_span(t, tn, ws);
                }
            } else {
                for (int i = 0; i < cb.n; ++i) cb.c[i].w = f->dw;
            }
            f->cid = cb.c;
            f->n_cid = cb.n;
            return f;
        }
        // A simple font: WinAnsi to start with (also for Type1's Standard
        // Encoding: the same for letters, digits and the usual marks)
        for (int c = 0; c < 256; ++c) f->u[c] = win_ansi(c);
        if (name_is(*a, st, "Type1") || name_is(*a, st, "MMType1")) {
            f->u[0x27] = 0x2019;                               // (StandardEncoding's quoteright)
            f->u[0x60] = 0x2018;
        }
        const int fc = get(*a, a->root, "FirstChar");
        const int first = fc >= 0 && a->node[fc].t == T::Num ? a->node[fc].v : 0;
        struct EncScan {
            Font* f;
            int code = 0;
            void open() {}
            void close() {}
            void number(double v) { code = static_cast<int>(v); }
            void name(const char* s, size_t n)
            {
                if (code >= 0 && code < 256) {
                    const uint16_t u = glyph_unicode(s, n);
                    f->u[code] = u;
                }
                ++code;
            }
        };
        struct WidthScan {
            Font* f;
            int code;
            void open() {}
            void close() {}
            void name(const char*, size_t) {}
            void number(double v)
            {
                if (code >= 0 && code < 256) f->w[code] = static_cast<uint16_t>(v < 0 ? 0 : v > 60000 ? 60000 : v);
                ++code;
            }
        };
        // /Encoding: a name, or a dictionary (inline or not) with /Differences
        const int enc = get(*a, a->root, "Encoding");
        int enc_ref = 0, wid_ref = 0;
        auto encoding = [&](const Obj& o, int d) {
            if (d < 0 || o.node[d].t != T::Dict) return;
            const int df = get(o, d, "Differences");
            if (df >= 0 && o.node[df].t == T::Array) {
                EncScan es{f};
                scan_span(o.buf + o.node[df].s, o.node[df].n, es);
            }
        };
        if (enc >= 0 && a->node[enc].t == T::Ref) enc_ref = a->node[enc].v;
        else encoding(*a, enc);
        const int wd = get(*a, a->root, "Widths");
        if (wd >= 0 && a->node[wd].t == T::Array) {
            WidthScan ws{f, first};
            scan_span(a->buf + a->node[wd].s, a->node[wd].n, ws);
        } else if (wd >= 0 && a->node[wd].t == T::Ref) {
            wid_ref = a->node[wd].v;
        }
        if (enc_ref && load(src, doc, enc_ref, *b)) encoding(*b, b->root);
        if (wid_ref && load(src, doc, wid_ref, *b) && b->node[b->root].t == T::Array) {
            WidthScan ws{f, first};
            scan_span(b->buf + b->node[b->root].s, b->node[b->root].n, ws);
        }
        if (tu_ref) to_unicode(src, doc, tu_ref, set_simple, f);
        return f;
    }

    // The font for object obj: cached, or loaded (the oldest one not on
    // this page making room)
    int font_slot(dax::ByteSource& src, const Doc& doc, int obj)
    {
        for (int i = 0; i < kFonts; ++i)
            if (font[i] && font[i]->obj == obj) {
                font[i]->used = ++clock;
                return i;
            }
        int slot = -1;
        for (int i = 0; i < kFonts && slot < 0; ++i)
            if (!font[i]) slot = i;
        if (slot < 0) {
            unsigned best = ~0u;
            for (int i = 0; i < kFonts; ++i) {
                bool on_page = false;
                for (int k = 0; k < n_named; ++k) on_page = on_page || named[k].slot == i;
                if (!on_page && font[i]->used < best) {
                    best = font[i]->used;
                    slot = i;
                }
            }
            if (slot < 0) return -1;
            delete font[slot];
            font[slot] = nullptr;
        }
        font[slot] = load_font(src, doc, obj);
        if (!font[slot]) return -1;
        font[slot]->used = ++clock;
        return slot;
    }

    // ---- lines
    void flush_line()
    {
        if (!have_line) return;
        have_line = false;
        while (ll > 0 && line[ll - 1] == ' ') --ll;
        int s = 0;
        while (s < ll && line[s] == ' ') ++s;
        if (s >= ll || stop || !sink) return;
        TextLine t;
        t.text = line + s;
        t.len = ll - s;
        t.x = lx;
        t.x_end = lx_end;
        t.y = ly;
        t.size = main_size();       // (the size most of its letters have: a small-caps word doesn't decide)
        ++lines;
        if (!sink->line(t)) stop = true;
    }
    void glyph(uint16_t u, float x0, float x1, float y, float size)
    {
        char ch[4];
        const int n = fold(u, ch);
        if (n == 0) return;
        const bool space = n == 1 && ch[0] == ' ';
        const float em = size > 0.5f ? size : 0.5f;
        if (have_line) {
            const float dy = std::fabs(y - ly);
            const bool new_line = dy > 0.4f * (em > lsize ? em : lsize) || x0 < lx_end - 1.5f * em;
            if (new_line) flush_line();
        }
        if (!have_line) {
            if (space) return;
            have_line = true;
            ll = 0;
            lx = x0;
            ly = y;
            lsize = size;
            n_main = n_alt = 0;
            lx_end = x1;
        } else if (!space && x0 - lx_end > 0.2f * em && ll > 0 && line[ll - 1] != ' ' && ll < kLine) {
            line[ll++] = ' ';               // (a gap: a word space drawn as a move)
        }
        if (space && ll > 0 && line[ll - 1] == ' ') {
            if (x1 > lx_end) lx_end = x1;
            return;
        }
        for (int i = 0; i < n && ll < kLine; ++i) line[ll++] = ch[i];
        if (x1 > lx_end || space) lx_end = x1 > lx_end ? x1 : lx_end;
        if (!space) {
            if (std::fabs(size - lsize) < 0.05f) {
                ++n_main;
            } else {
                if (std::fabs(size - alt) >= 0.05f) {
                    alt = size;
                    n_alt = 0;
                }
                ++n_alt;
            }
        }
    }

    // Moves the text matrix along by tx (text space units)
    void advance(float tx)
    {
        tm.e += tx * tm.a;
        tm.f += tx * tm.b;
    }

    void show(const uint8_t* s, int n)
    {
        if (!cur || !in_bt) return;
        const Mat m = mul(tm, ctm);
        const float scale_y = std::sqrt(m.c * m.c + m.d * m.d);
        const float size = fs * scale_y;
        const float th_k = th;
        for (int i = 0; i < n;) {
            uint32_t code;
            uint16_t u = 0, w = 0;
            if (cur->two) {
                if (i + 1 >= n) break;
                code = static_cast<uint32_t>(s[i]) << 8 | s[i + 1];
                i += 2;
                int lo = 0, hi = cur->n_cid - 1;
                w = cur->dw;
                while (lo <= hi) {
                    const int mid = (lo + hi) / 2;
                    if (cur->cid[mid].code == code) {
                        u = cur->cid[mid].u;
                        w = cur->cid[mid].w;
                        break;
                    }
                    if (cur->cid[mid].code < code) lo = mid + 1;
                    else hi = mid - 1;
                }
            } else {
                code = s[i++];
                u = cur->u[code];
                w = cur->w[code];
            }
            const Mat g = mul(tm, ctm);
            const float x0 = g.e, y0 = g.f;
            float tx = (static_cast<float>(w) / 1000.0f * fs + tc + (!cur->two && code == 32 ? tw : 0)) * th_k;
            const float x1 = x0 + tx * g.a;
            if (!cur->marks) glyph(u, x0, x1, y0, size);
            advance(tx);
        }
    }

    // ---- operators
    float arg(int k) const { return k < nn ? num[k] : 0; }       // k-th of the operands
    void op(const char* o, int n)
    {
        auto is = [&](const char* w) { return static_cast<int>(strlen(w)) == n && memcmp(o, w, static_cast<size_t>(n)) == 0; };
        const float* v = nn >= 6 ? num + nn - 6 : num;
        if (is("q")) {
            if (sp < 16) stack[sp++] = Saved{ctm, cur, fs, tc, tw, th, tleading};
        } else if (is("Q")) {
            if (sp > 0) {
                const Saved& s = stack[--sp];
                ctm = s.ctm;
                cur = s.font;
                fs = s.fs;
                tc = s.tc;
                tw = s.tw;
                th = s.th;
                tleading = s.tl;
            }
        } else if (is("cm") && nn >= 6) {
            Mat m{v[0], v[1], v[2], v[3], v[4], v[5]};
            ctm = mul(m, ctm);
        } else if (is("BT")) {
            in_bt = true;
            tm = tlm = Mat{};
        } else if (is("ET")) {
            in_bt = false;
        } else if (is("Tf") && nn >= 1) {
            fs = num[nn - 1];
            cur = nullptr;
            for (int k = 0; k < n_named; ++k)
                if (strcmp(named[k].name, last_name) == 0 && named[k].slot >= 0) cur = font[named[k].slot];
        } else if (is("Tm") && nn >= 6) {
            tm = tlm = Mat{v[0], v[1], v[2], v[3], v[4], v[5]};
        } else if ((is("Td") || is("TD")) && nn >= 2) {
            const float tx = num[nn - 2], ty = num[nn - 1];
            if (is("TD")) tleading = -ty;
            Mat t{1, 0, 0, 1, tx, ty};
            tm = tlm = mul(t, tlm);
        } else if (is("T*")) {
            Mat t{1, 0, 0, 1, 0, -tleading};
            tm = tlm = mul(t, tlm);
        } else if (is("TL") && nn >= 1) {
            tleading = num[nn - 1];
        } else if (is("Tc") && nn >= 1) {
            tc = num[nn - 1];
        } else if (is("Tw") && nn >= 1) {
            tw = num[nn - 1];
        } else if (is("Tz") && nn >= 1) {
            th = num[nn - 1] / 100.0f;
        } else if (is("ID")) {
            lex = L_Skip;                                         // an inline image's data
            skip_hist[0] = skip_hist[1] = skip_hist[2] = 0;
        }
        nn = 0;
    }

    void number_token()
    {
        tok[tl] = 0;
        const float x = static_cast<float>(strtod(tok, nullptr));
        if (dict_depth > 0) return;
        if (arr_depth > 0) {
            // TJ's adjustment: thousandths of the size, leftwards
            if (in_bt && cur) advance(-x / 1000.0f * fs * th);
            return;
        }
        if (nn == 8) {
            memmove(num, num + 1, sizeof num - sizeof num[0]);
            --nn;
        }
        num[nn++] = x;
    }
    bool pending_string = false;          // a string waits for its operator

    void word_token()
    {
        tok[tl] = 0;
        const char c = tok[0];
        if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.') {
            number_token();
            return;
        }
        if (dict_depth > 0) return;
        if (strcmp(tok, "Tj") == 0) {
            if (pending_string) show(reinterpret_cast<const uint8_t*>(str_buf), str_len);
            pending_string = false;
            nn = 0;
            return;
        }
        if (strcmp(tok, "'") == 0 || strcmp(tok, "\"") == 0) {
            // next line, then the string ('"' sets the spacing first)
            if (tok[0] == '"' && nn >= 2) {
                tw = num[nn - 2];
                tc = num[nn - 1];
            }
            Mat t{1, 0, 0, 1, 0, -tleading};
            tm = tlm = mul(t, tlm);
            if (pending_string) show(reinterpret_cast<const uint8_t*>(str_buf), str_len);
            pending_string = false;
            nn = 0;
            return;
        }
        pending_string = false;
        op(tok, tl);
    }
    char str_buf[kTok];
    int  str_len = 0;

    void put_str_byte(uint8_t b)
    {
        if (tl < kTok) tok[tl++] = static_cast<char>(b);
    }
    void end_string()
    {
        if (dict_depth > 0) return;
        // In an array: TJ's pieces, shown as they come; else Tj / ' / "
        // (shown at the operator)
        if (arr_depth > 0) {
            show(reinterpret_cast<const uint8_t*>(tok), tl);
        } else {
            memcpy(str_buf, tok, static_cast<size_t>(tl));
            str_len = tl;
            pending_string = true;
        }
    }

    bool put(uint8_t b) override
    {
        if (stop) return false;
        const char c = static_cast<char>(b);
        switch (lex) {
        case L_Skip:
            skip_hist[0] = skip_hist[1];
            skip_hist[1] = skip_hist[2];
            skip_hist[2] = b;
            if (is_ws(static_cast<char>(skip_hist[0])) && skip_hist[1] == 'E' && skip_hist[2] == 'I') lex = L_None;
            return true;
        case L_Comment:
            if (c == '\n' || c == '\r') lex = L_None;
            return true;
        case L_Str:
            if (oct_n > 0) {
                if (c >= '0' && c <= '7' && oct_n < 3) {
                    oct = oct * 8 + (c - '0');
                    ++oct_n;
                    if (oct_n == 3) {
                        put_str_byte(static_cast<uint8_t>(oct));
                        oct_n = 0;
                    }
                    return true;
                }
                put_str_byte(static_cast<uint8_t>(oct));
                oct_n = 0;
            }
            if (esc) {
                esc = false;
                switch (c) {
                case 'n': put_str_byte('\n'); break;
                case 'r': put_str_byte('\r'); break;
                case 't': put_str_byte('\t'); break;
                case 'b': put_str_byte('\b'); break;
                case 'f': put_str_byte('\f'); break;
                case '\r': case '\n': break;              // (a line continued)
                default:
                    if (c >= '0' && c <= '7') {
                        oct = c - '0';
                        oct_n = 1;
                    } else {
                        put_str_byte(b);
                    }
                }
                return true;
            }
            if (c == '\\') {
                esc = true;
            } else if (c == '(') {
                ++paren;
                put_str_byte(b);
            } else if (c == ')') {
                if (--paren == 0) {
                    lex = L_None;
                    end_string();
                } else {
                    put_str_byte(b);
                }
            } else {
                put_str_byte(b);
            }
            return true;
        case L_Hex:
            if (c == '>') {
                // pairs of digits -> bytes
                int k = 0;
                for (int i = 0; i < tl; i += 2) {
                    const int hi = hexval(tok[i]), lo = i + 1 < tl ? hexval(tok[i + 1]) : 0;
                    tok[k++] = static_cast<char>(hi << 4 | lo);
                }
                tl = k;
                lex = L_None;
                end_string();
            } else if (hexval(c) >= 0 && tl < kTok) {
                tok[tl++] = c;
            }
            return true;
        case L_Word:
        case L_Name:
            if (!is_ws(c) && !is_delim(c)) {
                if (tl < 63) tok[tl++] = c;
                return true;
            }
            if (lex == L_Name) {
                lex = L_None;
                tok[tl] = 0;
                if (dict_depth == 0) {
                    const size_t k = static_cast<size_t>(tl) < sizeof last_name - 1 ? static_cast<size_t>(tl) : sizeof last_name - 1;
                    memcpy(last_name, tok, k);
                    last_name[k] = 0;
                }
            } else {
                lex = L_None;
                word_token();            // (may start an inline image's skip)
            }
            break;
        case L_None:
            break;
        }
        if (lex == L_Skip) return true;
        // Between tokens
        if (lt) {
            lt = false;
            if (c == '<') {
                ++dict_depth;
                return true;
            }
            lex = L_Hex;
            tl = 0;
            if (hexval(c) >= 0) tok[tl++] = c;
            if (c == '>') {
                lex = L_None;
                end_string();
            }
            return true;
        }
        if (gt) {
            gt = false;
            if (c == '>') {
                if (dict_depth > 0) --dict_depth;
                return true;
            }
        }
        if (is_ws(c)) return true;
        switch (c) {
        case '%': lex = L_Comment; break;
        case '(': lex = L_Str; tl = 0; paren = 1; esc = false; oct_n = 0; break;
        case '<': lt = true; break;
        case '>': gt = true; break;
        case '[': ++arr_depth; break;
        case ']': if (arr_depth > 0) --arr_depth; break;
        case '/': lex = L_Name; tl = 0; break;
        case '{': case '}': break;
        default: lex = L_Word; tl = 0; tok[tl++] = c; break;
        }
        return true;
    }

    void reset_page()
    {
        lex = L_None;
        tl = 0;
        nn = 0;
        dict_depth = arr_depth = 0;
        lt = gt = false;
        ctm = Mat{};
        sp = 0;
        tm = tlm = Mat{};
        fs = tc = tw = tleading = 0;
        th = 1;
        cur = nullptr;
        in_bt = false;
        have_line = false;
        ll = 0;
        stop = false;
        lines = 0;
        pending_string = false;
        last_name[0] = 0;
    }
};

TextReader::TextReader() : d_(new (std::nothrow) Impl)
{
    if (!d_) return;
    d_->a = new (std::nothrow) Obj;
    d_->b = new (std::nothrow) Obj;
    d_->window = static_cast<uint8_t*>(malloc(inflate::kWindow));
}

TextReader::~TextReader() { delete d_; }

bool TextReader::ok() const { return d_ && d_->a && d_->b && d_->window; }
bool TextReader::no_memory() const { return !ok() || d_->no_mem; }
uint8_t* TextReader::window() const { return d_ ? d_->window : nullptr; }

int TextReader::page(dax::ByteSource& src, const Doc& doc, int page_obj, TextSink& out)
{
    if (!ok()) return -1;
    Impl& d = *d_;
    Obj& a = *d.a;
    Obj& b = *d.b;
    d.no_mem = false;
    if (!load(src, doc, page_obj, a)) return -1;
    // The contents: one stream or a list of them
    constexpr int kParts = 32;
    int parts[kParts], n_parts = 0;
    const int c = get(a, a.root, "Contents");
    if (c >= 0 && a.node[c].t == T::Ref) {
        // (a reference to a stream, or to an array of them)
        parts[n_parts++] = a.node[c].v;
    } else if (c >= 0 && a.node[c].t == T::Array) {
        for (int k = a.node[c].first; k >= 0 && n_parts < kParts; k = a.node[k].next)
            if (a.node[k].t == T::Ref) parts[n_parts++] = a.node[k].v;
    }
    // The fonts by name: /Resources here or inherited
    int res = get(a, a.root, "Resources");
    for (int up = 0; res < 0 && up < 8; ++up) {
        const int parent = get(a, a.root, "Parent");
        if (parent < 0 || a.node[parent].t != T::Ref || !load(src, doc, a.node[parent].v, a)) break;
        res = get(a, a.root, "Resources");
    }
    d.n_named = 0;
    const Obj* ro = nullptr;
    int rn = -1;
    if (res >= 0 && deref(src, doc, a, res, b, &ro, &rn)) {
        Obj* holder = ro == &b ? &b : &a;
        Obj* spare = holder == &a ? &b : &a;
        const int fo = get(*ro, rn, "Font");
        const Obj* fo_o = nullptr;
        int fo_n = -1;
        if (fo >= 0 && deref(src, doc, *holder, fo, *spare, &fo_o, &fo_n) && fo_n >= 0 && fo_o->node[fo_n].t == T::Dict) {
            for (int k = fo_o->node[fo_n].first; k >= 0 && d.n_named < kFonts;) {
                const int v = fo_o->node[k].next;
                if (v < 0) break;
                if (fo_o->node[v].t == T::Ref) {
                    Impl::Named& nm = d.named[d.n_named++];
                    const size_t len = fo_o->node[k].n < sizeof nm.name - 1 ? fo_o->node[k].n : sizeof nm.name - 1;
                    memcpy(nm.name, fo_o->buf + fo_o->node[k].s, len);
                    nm.name[len] = 0;
                    nm.obj = fo_o->node[v].v;
                    nm.slot = -1;
                }
                k = fo_o->node[v].next;
            }
        }
    }
    // A /Contents reference may be to an array of streams
    if (n_parts == 1 && load(src, doc, parts[0], a) && !a.stream_at && a.node[a.root].t == T::Array) {
        n_parts = 0;
        for (int k = a.node[a.root].first; k >= 0 && n_parts < kParts; k = a.node[k].next)
            if (a.node[k].t == T::Ref) parts[n_parts++] = a.node[k].v;
    }
    // The fonts loaded before the content (their maps need the window);
    // the ones this page doesn't use let go (memory: only the page's own -
    // a book's pages share theirs, read again when they come back)
    for (int i = 0; i < d.n_named; ++i) d.named[i].slot = -1;
    for (int k = 0; k < kFonts; ++k) {
        bool used = false;
        for (int i = 0; i < d.n_named && d.font[k] && !used; ++i) used = d.named[i].obj == d.font[k]->obj;
        if (d.font[k] && !used) {
            delete d.font[k];
            d.font[k] = nullptr;
        }
    }
    for (int i = 0; i < d.n_named; ++i) {
        int same = -1;
        for (int k = 0; k < i; ++k)
            if (d.named[k].obj == d.named[i].obj) same = d.named[k].slot;
        d.named[i].slot = same >= 0 ? same : d.font_slot(src, doc, d.named[i].obj);
    }
    d.reset_page();
    d.sink = &out;
    bool read = n_parts > 0;
    for (int i = 0; i < n_parts && !d.stop; ++i) {
        if (!load(src, doc, parts[i], a) || !a.stream_at) {
            read = false;
            continue;
        }
        // (a stream ends between tokens: the parts are one stream)
        if (!stream(src, doc, a, b, d, d.window) && !d.stop) read = false;
        d.put(' ');
    }
    d.put('\n');
    d.flush_line();
    d.sink = nullptr;
    return read || d.lines > 0 ? d.lines : -1;
}

} // namespace pdf
