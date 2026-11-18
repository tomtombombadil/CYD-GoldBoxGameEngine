// The journal book view's columns (ui/pdfview.cpp find_columns), worked out
// on the PC for a page picture: the same profile, the same journal::
// clear_strips / find_columns - to check the Column zoom on a real journal.
// Prints the page's size and each column's x range (scan pixels).
//   book_columns <page.jpg> ...
// Build: g++ -O1 -std=gnu++17 -Isrc -Itools/host -o bc tools/devkit/src/book_columns.cpp \
//   src/engine/{journal,journal_tables,pdf,jpeg,dax,inflate,pdf_text}.cpp
// (The pictures are the player's journal: scratch only.)
#include <cstdio>
#include <cstring>
#include <vector>
#include "engine/dax.h"
#include "engine/jpeg.h"
#include "engine/journal.h"

namespace {
struct Profile {
    int s = 0, w = 0, h = 0, top = 0, band_h = 1;
    uint16_t ink[journal::kColumnBands][424];
    uint8_t clear[424];
};
bool block(int x, int y, int w, int h, const uint8_t* rgb, void* ctx)
{
    Profile& p = *static_cast<Profile*>(ctx);
    for (int r = 0; r < h; ++r) {
        const int band = (y + r - p.top) / p.band_h;
        if (y + r < p.top || band >= journal::kColumnBands) continue;
        uint16_t* ink = p.ink[band];
        const uint8_t* s = rgb + static_cast<size_t>(r) * w * 3;
        for (int c = 0; c < w; ++c, s += 3) {
            const int px = x + c;
            if (px >= p.w || px >= 424) break;
            const int lum = (s[0] + s[1] + s[2]) / 3;
            if (lum < 240 && ink[px] < 60000) ink[px] = static_cast<uint16_t>(ink[px] + (240 - lum));
        }
    }
    return true;
}
} // namespace

int main(int argc, char** argv)
{
    static uint8_t pool[jpeg::kPoolSize];
    for (int a = 1; a < argc; ++a) {
        FILE* f = fopen(argv[a], "rb");
        if (!f) continue;
        std::vector<uint8_t> b;
        uint8_t buf[65536];
        size_t n;
        while ((n = fread(buf, 1, sizeof buf, f)) > 0) b.insert(b.end(), buf, buf + n);
        fclose(f);
        dax::MemorySource src(b.data(), b.size());
        jpeg::Info info;
        if (!jpeg::probe(src, 0, static_cast<uint32_t>(b.size()), pool, info)) continue;
        static Profile p;
        memset(&p, 0, sizeof p);
        while (p.s < 3 && (info.width >> p.s) > 420) ++p.s;
        p.w = info.width >> p.s;
        p.h = info.height >> p.s;
        p.top = p.h / 25;
        p.band_h = (p.h - 2 * p.top) / journal::kColumnBands;
        if (p.band_h < 1) p.band_h = 1;
        jpeg::decode(src, 0, static_cast<uint32_t>(b.size()), pool, nullptr, block, &p, nullptr, p.s);
        const int w = p.w < 424 ? p.w : 424;
        journal::clear_strips(&p.ink[0][0], 424, w, journal::kColumnBands, p.band_h, p.clear);
        int x0[8], x1[8];
        const int nc = journal::join_margins(x0, x1, journal::find_columns(p.clear, w, journal::kColumnBands, x0, x1, 8));
        printf("%s %dx%d:", argv[a], info.width, info.height);
        for (int i = 0; i < nc; ++i) printf(" [%d-%d]", x0[i] << p.s, x1[i] << p.s);
        printf("\n");
    }
    return 0;
}
