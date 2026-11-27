// Each map's 15 wall types: a hash of the picture each draws head-on (near and middle), from the wall sets its area
// script loads (ecl::find_map_load) - for the secret-door rule by looks
#include "engine/view3d.cpp"
#include "engine/ecl.h"
#include "engine/profile.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <dirent.h>
struct FileSrc : dax::ByteSource { FILE* f; uint32_t n;
    explicit FileSrc(const char* p) { f = fopen(p, "rb"); n = 0; if (f) { fseek(f, 0, SEEK_END); n = (uint32_t)ftell(f); } }
    ~FileSrc() { if (f) fclose(f); }
    uint32_t size() const override { return n; }
    size_t read_at(uint32_t off, uint8_t* d, size_t l) override { if (!f) return 0; fseek(f, off, SEEK_SET); return fread(d, 1, l, f); } };
static std::string dir;
// the WALLDEF file holding block b, its number
int walldef_file(int b) {
    for (int n = 1; n <= 9; ++n) { FileSrc s((dir + "/WALLDEF" + std::to_string(n) + ".DAX").c_str()); if (!s.f) continue;
        dax::Index i; if (dax::read_index(s, i) != dax::Status::Ok) continue; if (i.find(static_cast<uint8_t>(b))) return n; }
    return 0;
}
int main(int argc, char** argv) {
    dir = argv[1]; const profile::Profile* pr = profile::first(static_cast<games::Game>(atoi(argv[2])));
    for (int en = 1; en <= 9; ++en) {
        FileSrc src((dir + "/ECL" + std::to_string(en) + ".DAX").c_str()); if (!src.f) continue;
        dax::Index idx; if (dax::read_index(src, idx) != dax::Status::Ok) continue;
        for (int k = 0; k < idx.count; ++k) {
            const dax::Entry& e = idx.entries[k]; std::vector<uint8_t> b(e.raw_size);
            dax::RleReader r(src, idx, e); r.read(b.data(), b.size());
            ecl::MapLoad ml = ecl::find_map_load(b.data(), b.size(), *pr->ecl_ops);
            if (!ml.has_geo()) continue;
            printf("G %d", ml.geo);
            for (int s = 0; s < 3; ++s) {
                const int blk = ml.walls[s];
                for (int p = 0; p < 5; ++p) {
                    unsigned h = 0;
                    if (blk != ecl::kNone) {
                        const int fn = walldef_file(blk);
                        static view3d::World w; w = view3d::World{}; int n = 0;
                        FileSrc ws((dir + "/WALLDEF" + std::to_string(fn) + ".DAX").c_str()); dax::Index wi; dax::read_index(ws, wi);
                        FileSrc c1((dir + "/8X8D1.DAX").c_str()); dax::Index ci; dax::read_index(c1, ci);
                        FileSrc ts((dir + "/8X8D" + std::to_string(fn) + ".DAX").c_str()); dax::Index ti; dax::read_index(ts, ti);
                        if (view3d::load_walls(ws, wi, 1, blk, w, &n)) {
                            view3d::load_tiles(c1, ci, 203, w.common);
                            view3d::load_tiles(ts, ti, view3d::tiles_block(blk, n, 0), w.sets[0]);
                            static uint8_t buf[112*112]; memset(buf, 0, sizeof buf);
                            pic::Canvas c{buf, 112, 112};
                            view3d::piece(c, w, 6, p + 1, 1, 2); view3d::piece(c, w, 3, p + 1, 1, 2);
                            h = 2166136261u; bool any = false;
                            for (unsigned i = 0; i < sizeof buf; ++i) { h = (h ^ buf[i]) * 16777619u; any |= buf[i] != 0; }
                            if (!any) h = 0;
                        }
                    }
                    printf(" %08x", h);
                }
            }
            printf("\n");
        }
    }
}
