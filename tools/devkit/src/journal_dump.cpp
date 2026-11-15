// Makes JOURNAL.DAT from a journal PDF with the firmware's own table (as the
// card scan does) and writes one entry's pieces as PPM pictures, to check a
// table's rectangles by eye. The pictures show the journal: scratch only.
//   journal_dump <journal.pdf> <J|T|I|P> <number> <out prefix>
// Build: g++ -O1 -std=gnu++17 -Isrc -Itools/host -o jd tools/devkit/src/journal_dump.cpp \
//   src/engine/{journal,journal_tables,pdf,jpeg,dax,inflate}.cpp
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "engine/dax.h"
#include "engine/journal.h"
#include "engine/pdf.h"

struct MemOut : journal::Output {
    std::vector<uint8_t> b;
    bool write_at(uint32_t pos, const uint8_t* d, size_t n) override
    {
        if (b.size() < pos + n) b.resize(pos + n);
        memcpy(b.data() + pos, d, n);
        return true;
    }
};

int main(int argc, char** argv)
{
    if (argc < 5) {
        fprintf(stderr, "usage: journal_dump <pdf> <kind> <number> <out prefix>\n");
        return 2;
    }
    FILE* f = fopen(argv[1], "rb");
    if (!f) return 1;
    std::vector<uint8_t> pdf_bytes;
    uint8_t buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) pdf_bytes.insert(pdf_bytes.end(), buf, buf + n);
    fclose(f);
    dax::MemorySource src(pdf_bytes.data(), pdf_bytes.size());
    static pdf::Doc doc;
    if (!pdf::open(src, doc)) { fprintf(stderr, "not a PDF it reads\n"); return 1; }
    const journal::Table* t = journal::find_table(static_cast<uint32_t>(pdf_bytes.size()), doc.id);
    if (!t) { fprintf(stderr, "no table for %u %s\n", static_cast<unsigned>(pdf_bytes.size()), doc.id); return 1; }
    MemOut out;
    if (!journal::make(src, *t, out, nullptr, nullptr)) { fprintf(stderr, "make failed\n"); return 1; }
    dax::MemorySource js(out.b.data(), out.b.size());
    static journal::Info info;
    int first = 0, count = 0;
    if (!journal::read_info(js, info) || !journal::find(js, info, argv[2][0], atoi(argv[3]), &first, &count)) {
        fprintf(stderr, "no such entry\n");
        return 1;
    }
    for (int i = 0; i < count; ++i) {
        journal::PieceInfo p;
        if (!journal::piece(js, info, first + i, p) || p.text()) continue;
        char name[512];
        snprintf(name, sizeof name, "%s_%d.ppm", argv[4], i);
        FILE* o = fopen(name, "wb");
        fprintf(o, "P6\n%d %d\n255\n", p.w, p.h);
        for (int k = 0; k < p.w * p.h; ++k) {
            uint8_t rgb[3];
            journal::palette_rgb(out.b[p.offset + k], &rgb[0], &rgb[1], &rgb[2]);
            fwrite(rgb, 1, 3, o);
        }
        fclose(o);
        printf("%s %d x %d\n", name, p.w, p.h);
    }
    return 0;
}
