#include "engine/ecl.h"
#include "engine/dax.h"
#include "engine/profile.h"
#include "engine/games.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
struct FileSrc : dax::ByteSource { FILE* f; uint32_t n;
    explicit FileSrc(const char* p) { f = fopen(p, "rb"); fseek(f, 0, SEEK_END); n = (uint32_t)ftell(f); }
    uint32_t size() const override { return n; }
    size_t read_at(uint32_t off, uint8_t* d, size_t l) override { fseek(f, off, SEEK_SET); return fread(d, 1, l, f); } };
// argv[1]: game number (games::Game), then ECL*.DAX files: each block's map and wall sets
int main(int argc, char** argv) {
    const profile::Profile* pr = profile::first(static_cast<games::Game>(atoi(argv[1])));
    if (!pr || !pr->ecl_ops) { puts("no profile"); return 1; }
    for (int a = 2; a < argc; ++a) {
        FileSrc src(argv[a]); dax::Index idx; if (dax::read_index(src, idx) != dax::Status::Ok) continue;
        for (int k = 0; k < idx.count; ++k) {
            const dax::Entry& e = idx.entries[k]; std::vector<uint8_t> b(e.raw_size);
            dax::RleReader r(src, idx, e); r.read(b.data(), b.size());
            ecl::MapLoad ml = ecl::find_map_load(b.data(), b.size(), *pr->ecl_ops);
            printf("%s ecl %d geo %d walls %d %d %d\n", argv[a], e.id, ml.geo, ml.walls[0], ml.walls[1], ml.walls[2]);
        }
    }
}
