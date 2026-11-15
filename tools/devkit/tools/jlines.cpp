// jlines.cpp <journal.pdf> <page>: the engine's own text lines of one PDF page
// (x, y, size, text) - what journal::make_book sees (2026-11-15)
// g++ -std=c++17 -O1 -Isrc -o jlines tools/devkit/tools/jlines.cpp src/engine/*.cpp
#include "engine/pdf_text.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
int main(int argc, char** argv)
{
    if (argc < 3) return 1;
    FILE* f = fopen(argv[1], "rb");
    if (!f) return 1;
    std::vector<uint8_t> d;
    fseek(f, 0, SEEK_END);
    d.resize(ftell(f));
    fseek(f, 0, SEEK_SET);
    if (fread(d.data(), 1, d.size(), f) != d.size()) return 1;
    dax::MemorySource src(d.data(), d.size());
    static pdf::Doc doc;
    uint32_t* more = nullptr;
    if (!pdf::open_large(src, doc, &more)) return 2;
    static int pages[2000];
    const int n = pdf::pages(src, doc, pages, 2000), p = atoi(argv[2]);
    if (p < 1 || p > n) return 3;
    struct Out : pdf::TextSink {
        bool line(const pdf::TextLine& l) override
        {
            printf("x %6.1f-%6.1f y %6.1f s %4.1f |%.*s|\n", l.x, l.x_end, l.y, l.size, l.len, l.text);
            return true;
        }
    } out;
    pdf::TextReader tr;
    tr.page(src, doc, pages[p - 1], out);
    free(more);
}
