#include "engine/view3d.h"
#include "engine/dax.h"
#include <cstdio>
#include <cstring>
struct FileSrc : dax::ByteSource { FILE* f; uint32_t n;
    explicit FileSrc(const char* p) { f = fopen(p, "rb"); fseek(f, 0, SEEK_END); n = (uint32_t)ftell(f); }
    uint32_t size() const override { return n; }
    size_t read_at(uint32_t off, uint8_t* d, size_t l) override { fseek(f, off, SEEK_SET); return fread(d, 1, l, f); } };
static const int G[11]={0,2,6,10,22,38,54,110,132,154,156};
int main(int argc, char** argv) {
    for (int a = 1; a < argc; ++a) {
        FileSrc src(argv[a]); dax::Index idx; if (dax::read_index(src, idx) != dax::Status::Ok) continue;
        for (int k = 0; k < idx.count; ++k) {
            static view3d::World w; w = view3d::World{}; int n = 0;
            if (!view3d::load_walls(src, idx, 1, idx.entries[k].id, w, &n)) continue;
            for (int s = 0; s < n && s < 3; ++s) {
                printf("%s b%d set%d:", argv[a]+strlen(argv[a])-12, idx.entries[k].id, s+1);
                for (int p = 0; p < 5; ++p) for (int q = p+1; q < 5; ++q) {
                    // which of the 10 views (groups) are the same
                    char same[11]={0}; int ns=0;
                    for (int g = 0; g < 10; ++g) { bool eq=true; for (int i=G[g];i<G[g+1];++i) if (w.walls[s].id[p][i]!=w.walls[s].id[q][i]) eq=false; same[g]=eq?'='+0:'.'; ns+=eq; }
                    if (ns >= 3) printf(" p%d~p%d %s", p, q, same);
                }
                printf("\n");
            }
        }
    }
}
