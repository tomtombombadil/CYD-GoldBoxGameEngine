#include "engine/view3d.cpp"
#include "engine/dax.h"
#include "engine/picture.h"
#include <cstdio>
#include <cstring>
#include <string>
struct FileSrc : dax::ByteSource { FILE* f; uint32_t n;
    explicit FileSrc(const char* p) { f = fopen(p, "rb"); if (!f) { n=0; return; } fseek(f, 0, SEEK_END); n = (uint32_t)ftell(f); }
    uint32_t size() const override { return n; }
    size_t read_at(uint32_t off, uint8_t* d, size_t l) override { fseek(f, off, SEEK_SET); return fread(d, 1, l, f); } };
// a WALLDEF block's 5 pieces seen head-on near (group 6) and middle (group 3), into a PPM
int main(int argc, char** argv) {
    std::string dir = argv[1]; int file = atoi(argv[2]), block = atoi(argv[3]);
    static view3d::World w;
    FileSrc ws((dir + "/WALLDEF" + std::to_string(file) + ".DAX").c_str()); dax::Index wi; dax::read_index(ws, wi);
    int n = 0; if (!view3d::load_walls(ws, wi, 1, block, w, &n)) { puts("no walls"); return 1; }
    FileSrc c1((dir + "/8X8D1.DAX").c_str()); dax::Index ci; dax::read_index(c1, ci);
    view3d::load_tiles(c1, ci, 203, w.common);
    FileSrc ts((dir + "/8X8D" + std::to_string(file) + ".DAX").c_str()); dax::Index ti; dax::read_index(ts, ti);
    if (!view3d::load_tiles(ts, ti, view3d::tiles_block(block, n, 0), w.sets[0])) puts("no tiles");
    const int VW = 112, VH = 112;
    static uint8_t buf[5][VW*VH];
    for (int p = 0; p < 5; ++p) {
        memset(buf[p], 1, sizeof buf[p]);
        pic::Canvas c{buf[p], VW, VH};
        view3d::piece(c, w, 6, p + 1, 1, 2);
    }
    static const uint8_t ega[16][3]={{0,0,0},{0,0,170},{0,170,0},{0,170,170},{170,0,0},{170,0,170},{170,85,0},{170,170,170},{85,85,85},{85,85,255},{85,255,85},{85,255,255},{255,85,85},{255,85,255},{255,255,85},{255,255,255}};
    FILE* o = fopen(argv[4], "wb"); fprintf(o, "P6 %d %d 255\n", VW*5+20, VH);
    for (int y = 0; y < VH; ++y) for (int p = 0; p < 5; ++p) { for (int x = 0; x < VW; ++x) fwrite(ega[buf[p][y*VW+x]&15], 1, 3, o); if (p<4) for (int k=0;k<5;++k) fwrite(ega[15],1,3,o); }
    fclose(o);
    // and whether the fronts differ pixel by pixel
    for (int p = 0; p < 5; ++p) { printf("p%d:", p); for (int q = 0; q < 5; ++q) { int d=0; for (int i=0;i<VW*VH;++i) d += buf[p][i]!=buf[q][i]; printf(" %5d", d); } printf("\n"); }
}
