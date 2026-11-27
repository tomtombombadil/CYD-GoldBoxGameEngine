#include "engine/geo.h"
#include "engine/dax.h"
#include <cstdio>
struct FileSrc : dax::ByteSource { FILE* f; uint32_t n;
    explicit FileSrc(const char* p) { f = fopen(p, "rb"); fseek(f, 0, SEEK_END); n = (uint32_t)ftell(f); }
    uint32_t size() const override { return n; }
    size_t read_at(uint32_t off, uint8_t* d, size_t l) override { fseek(f, off, SEEK_SET); return fread(d, 1, l, f); } };
// per block: "B id" then for each y,x: 4 numbers per square: wall,door for dirs 0,2,4,6 ; plus plain bits
int main(int argc, char** argv) {
    for (int a = 1; a < argc; ++a) {
        FileSrc src(argv[a]); dax::Index idx; if (dax::read_index(src, idx) != dax::Status::Ok) continue;
        for (int k = 0; k < idx.count; ++k) { const int id = idx.entries[k].id; geo::Map m; if (!geo::load(src, idx, id, m)) continue;
            printf("B %s %d %u\n", argv[a], id, geo::plain_walls(m));
            for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x) { for (int d = 0; d < 8; d += 2) printf("%d %d %d ", geo::wall(m,x,y,d), geo::door(m,x,y,d), geo::passage(m,x,y,d)); printf("\n"); } }
    }
}
