#include "cluebook.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <new>

#include "jpeg.h"
#include "pdf.h"
#include "pdf_text.h"

namespace cluebook {

const Book* find(games::Game game, int pages, float aspect)
{
    for (int i = 0; i < kBookCount; ++i)
        if (kBooks[i].game == game && kBooks[i].pages == pages &&
            (aspect < 0 || std::fabs(kBooks[i].aspect - aspect) < 0.012f * kBooks[i].aspect))
            return &kBooks[i];
    return nullptr;
}

namespace {

float poly(const float* c, float u, float v)
{
    u /= 16.0f;
    v /= 16.0f;
    const float v2 = v * v, u2 = u * u;
    return c[0] + c[1] * v + c[2] * v2 + c[3] * v2 * v + c[4] * u + c[5] * u * v + c[6] * u * v2 + c[7] * u2 +
           c[8] * u2 * v + c[9] * u2 * u;
}

// The page picture, whole, RGB888
struct Page {
    uint8_t* rgb = nullptr;
    int      w = 0, h = 0;
};

bool page_block(int x, int y, int w, int h, const uint8_t* rgb, void* ctx)
{
    Page& p = *static_cast<Page*>(ctx);
    for (int j = 0; j < h; ++j) {
        if (y + j >= p.h) break;
        const int n = x + w > p.w ? p.w - x : w;
        if (n > 0) memcpy(p.rgb + (static_cast<size_t>(y + j) * p.w + x) * 3, rgb + static_cast<size_t>(j) * w * 3, n * 3);
    }
    return true;
}

// A name's line: the text lines whose middle is inside the box, left to right
struct NameSink : pdf::TextSink {
    const float* box;
    char text[96] = "";
    float at[8] = {};
    int n = 0;
    bool line(const pdf::TextLine& l) override
    {
        const float y = l.y + l.size * 0.35f;      // (the line's middle, from the page's foot)
        const float mid = (l.x + l.x_end) / 2;
        if (y < box[2] || y > box[3] || mid < box[0] || mid > box[1]) return true;
        // (in the order across the page)
        size_t len = strlen(text);
        if (len + l.len + 2 >= sizeof text) return true;
        if (len) text[len++] = ' ';
        memcpy(text + len, l.text, l.len);
        text[len + l.len] = 0;
        ++n;
        return true;
    }
};

} // namespace

void grid_point(const Map& m, float u, float v, float* x, float* y)
{
    *x = poly(m.cx, u, v);
    *y = poly(m.cy, u, v);
}

void tidy_name(const char* in, char* out, size_t cap)
{
    if (!cap) return;
    while (*in == ' ') ++in;
    // ("J. " / "J." / "12. " - the book's letter or number for the map)
    const char* p = in;
    while (*p && *p != ' ' && *p != '.' && p - in < 3 && isalnum(static_cast<unsigned char>(*p))) ++p;
    const char* q = p;
    while (*q == ' ') ++q;
    if (p > in && *q == '.') {
        p = q;
        in = p + 1;
        while (*in == ' ') ++in;
    }
    size_t o = 0;
    bool start = true;
    char word[24];
    size_t wl = 0;
    auto flush = [&](bool first) {
        word[wl] = 0;
        static const char* small[] = {"of", "the", "and", "to", "in", "at", "on", "a", "an", "for"};
        bool lower = false;
        if (!first)
            for (const char* s : small)
                if (strcasecmp(word, s) == 0) lower = true;
        for (size_t i = 0; i < wl && o + 1 < cap; ++i) {
            const char c = word[i];
            out[o++] = (i == 0 && !lower) ? static_cast<char>(toupper(static_cast<unsigned char>(c)))
                                           : static_cast<char>(tolower(static_cast<unsigned char>(c)));
        }
        wl = 0;
    };
    bool any = false;
    for (; *in; ++in) {
        const char c = *in;
        if (isalnum(static_cast<unsigned char>(c)) || c == '\'') {
            if (wl + 1 < sizeof word) word[wl++] = c;
            continue;
        }
        if (wl) {
            flush(!any);
            any = true;
        }
        if (c == ' ') {
            if (o && out[o - 1] != ' ' && o + 1 < cap) out[o++] = ' ';
        } else if (o + 1 < cap) {
            out[o++] = c;
        }
        start = false;
    }
    if (wl) flush(!any);
    while (o && (out[o - 1] == ' ' || out[o - 1] == ':' || out[o - 1] == '-')) --o;
    out[o] = 0;
    (void)start;
}

int make(dax::ByteSource& src, games::Game game, Sink& out)
{
    pdf::Doc* doc = new (std::nothrow) pdf::Doc;
    int* pages = new (std::nothrow) int[pdf::kMaxPages];
    uint8_t* pool = static_cast<uint8_t*>(malloc(jpeg::kPoolSize));
    pdf::TextReader* text = new (std::nothrow) pdf::TextReader;
    int made = -2;
    const Book* book = nullptr;
    int n_pages = 0;
    if (!doc || !pages || !pool || !text || !text->ok()) goto done;
    made = -1;
    doc->window = text->window();       // (object streams inflate in the reader's window)
    if (!pdf::open(src, *doc)) goto done;
    n_pages = pdf::pages(src, *doc, pages, pdf::kMaxPages);
    // (the edition: its page count, then the shape of the page its first map is on)
    for (int i = 0; i < kBookCount && !book; ++i) {
        const Book& b = kBooks[i];
        if (b.game != game || b.pages != n_pages || b.n == 0 || b.maps[0].page > n_pages) continue;
        pdf::Image im;
        if (pdf::page_image(src, *doc, pages[b.maps[0].page - 1], im) && im.height > 0)
            book = find(game, n_pages, static_cast<float>(im.width) / im.height);
    }
    if (!book) goto done;
    made = 0;
    {
        int cur = 0;        // (the page in hand)
        Page pg;
        for (int i = 0; i < book->n; ++i) {
            const Map& m = book->maps[i];
            out.progress(i, book->n);
            if (m.page < 1 || m.page > n_pages) continue;
            if (m.page != cur) {
                free(pg.rgb);
                pg = Page();
                cur = m.page;
                pdf::Image img;
                if (!pdf::page_image(src, *doc, pages[m.page - 1], img) || !img.jpeg) continue;
                pg.rgb = static_cast<uint8_t*>(malloc(static_cast<size_t>(img.width) * img.height * 3));
                if (!pg.rgb) {
                    made = -2;
                    break;
                }
                pg.w = img.width;
                pg.h = img.height;
                memset(pg.rgb, 255, static_cast<size_t>(pg.w) * pg.h * 3);
                if (!jpeg::decode(src, img.data_at, img.data_len, pool, nullptr, page_block, &pg)) {
                    free(pg.rgb);
                    pg = Page();
                    continue;
                }
            }
            if (!pg.rgb) continue;
            // The square's size in the picture: as the page drew it (a whole number of pixels)
            float x0, y0, x1, y1;
            grid_point(m, 0, 8, &x0, &y0);
            grid_point(m, 16, 8, &x1, &y1);
            int sq = static_cast<int>(std::lround((x1 - x0) * pg.w / 16.0f));
            if (sq < 12) sq = 12;
            if (sq > 64) sq = 64;
            const int n = static_cast<int>((16 + 2 * kMargin) * sq);
            uint8_t* pic = static_cast<uint8_t*>(malloc(static_cast<size_t>(n) * n * 3));
            if (!pic) {
                made = -2;
                break;
            }
            for (int oy = 0; oy < n; ++oy)
                for (int ox = 0; ox < n; ++ox) {
                    const float u = (ox + 0.5f) / sq - kMargin, v = (oy + 0.5f) / sq - kMargin;
                    float fx, fy;
                    grid_point(m, u, v, &fx, &fy);
                    const float px = fx * pg.w - 0.5f, py = fy * pg.h - 0.5f;
                    int ix = static_cast<int>(std::floor(px)), iy = static_cast<int>(std::floor(py));
                    const float ax = px - ix, ay = py - iy;
                    uint8_t* d = pic + (static_cast<size_t>(oy) * n + ox) * 3;
                    if (ix < 0 || iy < 0 || ix + 1 >= pg.w || iy + 1 >= pg.h) {
                        d[0] = d[1] = d[2] = 255;
                        continue;
                    }
                    const uint8_t* a = pg.rgb + (static_cast<size_t>(iy) * pg.w + ix) * 3;
                    const uint8_t* b = a + static_cast<size_t>(pg.w) * 3;
                    for (int c = 0; c < 3; ++c) {
                        const float t = a[c] * (1 - ax) + a[c + 3] * ax, s = b[c] * (1 - ax) + b[c + 3] * ax;
                        d[c] = static_cast<uint8_t>(t * (1 - ay) + s * ay + 0.5f);
                    }
                }
            const bool go = out.picture(m, sq, n, n, pic);
            free(pic);
            ++made;
            if (m.fix) {
                if (m.fix[0]) out.name(m, m.fix);
            } else if (m.name[1] > m.name[0]) {
                NameSink ns;
                ns.box = m.name;
                text->page(src, *doc, pages[m.page - 1], ns);
                if (ns.text[0]) {
                    char tidy[96];
                    tidy_name(ns.text, tidy, sizeof tidy);
                    if (tidy[0]) out.name(m, tidy);
                }
            }
            if (!go) break;
        }
        free(pg.rgb);
        out.progress(book->n, book->n);
    }
done:
    delete text;
    free(pool);
    delete[] pages;
    delete doc;
    return made;
}

} // namespace cluebook
