// doorscan <GEO*.DAX ...>: each map block's locked sides (passage 2 / 3)
#include "engine/dax.h"
#include "engine/geo.h"
#include <cstdio>
#include <vector>
#include <cstring>
struct FileSrc : dax::ByteSource {
    std::vector<uint8_t> d;
    size_t read_at(uint32_t pos, uint8_t* out, size_t n) override { if (pos >= d.size()) return 0; if (pos + n > d.size()) n = d.size() - pos; memcpy(out, d.data() + pos, n); return n; }
    uint32_t size() const override { return d.size(); }
};
int main(int argc, char** argv) {
    for (int a = 1; a < argc; ++a) {
        FILE* f = fopen(argv[a], "rb"); if (!f) continue;
        FileSrc s; int c; while ((c = fgetc(f)) != EOF) s.d.push_back((uint8_t)c); fclose(f);
        dax::Index idx; if (dax::read_index(s, idx) != dax::Status::Ok) { printf("%s: no index\n", argv[a]); continue; }
        for (int i = 0; i < idx.count; ++i) {
            geo::Map m; if (!geo::load(s, idx, idx.entries[i].id, m)) continue;
            int n2 = 0, n3 = 0;
            for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x) for (int d = 0; d < 8; d += 2) { int p = geo::passage(m, x, y, d); n2 += p == 2; n3 += p == 3; }
            printf("%s #%d: locked %d unpickable %d\n", argv[a], idx.entries[i].id, n2, n3);
            geo::release(m);
        }
    }
}
