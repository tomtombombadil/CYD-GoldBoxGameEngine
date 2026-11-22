// vace_run <ACF> <BMA> <out dir> [ega] [nosound] [every]: plays a VACE movie with the
// engine's player (src/engine/vace.*) on a simulated clock and writes, as the
// DOSBox recorder (claude/devkit_dosbox_autodos_patch.md, AUTODOS_FRAMES +
// the DAC in its hash) does, every screen change seen at a 20 ms poll as
// <out>/NNNNN.ppm with "frame N at T ms" lines on stdout - vace_cmp.py then
// matches them with DOSBox's ("every": each change at 1 ms - a fade's every step). Nothing from the games here: it reads the
// player's files. Build: g++ -std=c++17 -O2 -Isrc tools/devkit/vace/vace_run.cpp
//   src/engine/vace.cpp src/engine/dax.cpp -o /tmp/claude-0/ag7/bin/vace_run
#include "engine/vace.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

struct Src : dax::ByteSource {
    std::vector<uint8_t> d;
    size_t read_at(uint32_t pos, uint8_t* out, size_t n) override
    {
        if (pos >= d.size()) return 0;
        if (pos + n > d.size()) n = d.size() - pos;
        memcpy(out, d.data() + pos, n);
        return n;
    }
    uint32_t size() const override { return static_cast<uint32_t>(d.size()); }
};

static const uint8_t kEga8[16][3] = {{0, 0, 0}, {0, 0, 170}, {0, 170, 0}, {0, 170, 170}, {170, 0, 0}, {170, 0, 170},
    {170, 85, 0}, {170, 170, 170}, {85, 85, 85}, {85, 85, 255}, {85, 255, 85}, {85, 255, 255}, {255, 85, 85},
    {255, 85, 255}, {255, 255, 85}, {255, 255, 255}};

struct Scr : vace::Screen {
    uint8_t px[320 * 200] = {};
    uint8_t dac[256][3] = {};
    bool ega = false;
    int song_n = -1;
    void put(int x, int y, uint8_t v) override { px[y * 320 + x] = v; }
    uint8_t get(int x, int y) const override { return px[y * 320 + x]; }
    void colour(int i, uint8_t r, uint8_t g, uint8_t b) override { dac[i][0] = r; dac[i][1] = g; dac[i][2] = b; }
    void song(int n) override { song_n = n; }
    void rgb(int v, uint8_t* o) const
    {
        if (ega) { memcpy(o, kEga8[v & 15], 3); return; }
        for (int c = 0; c < 3; ++c) o[c] = static_cast<uint8_t>(dac[v][c] << 2 | dac[v][c] >> 4);
    }
    unsigned long hash() const
    {
        unsigned long h = 1469598103UL;
        for (int i = 0; i < 64000; ++i) h = (h ^ px[i]) * 1099511628211UL;
        for (int i = 0; i < 256; ++i) h = (h ^ (dac[i][0] | dac[i][1] << 6 | dac[i][2] << 12)) * 1099511628211UL;
        return h;
    }
};

static bool load(const char* fn, std::vector<uint8_t>& out)
{
    FILE* f = fopen(fn, "rb");
    if (!f) return false;
    int c;
    while ((c = fgetc(f)) != EOF) out.push_back(static_cast<uint8_t>(c));
    fclose(f);
    return true;
}

int main(int argc, char** argv)
{
    if (argc < 4) { fprintf(stderr, "vace_run <ACF> <BMA> <out dir> [ega] [nosound]\n"); return 1; }
    std::vector<uint8_t> acf;
    Src bma;
    if (!load(argv[1], acf) || !load(argv[2], bma.d)) { fprintf(stderr, "can't read the files\n"); return 1; }
    bool ega = false, sound = true, every = false;
    for (int i = 4; i < argc; ++i) {
        if (!strcmp(argv[i], "ega")) ega = true;
        if (!strcmp(argv[i], "nosound")) sound = false;
        if (!strcmp(argv[i], "every")) every = true;
    }
    static vace::Player p;
    static Scr s;
    s.ega = ega;
    if (!vace::open(p, acf.data(), static_cast<uint32_t>(acf.size()), bma, !ega, sound)) { fprintf(stderr, "not a VACE movie\n"); return 1; }
    printf("chunks %d\n", p.chunks);
    unsigned long last = s.hash();
    int n = 0, song = -1;
    uint32_t next = 0, t = 0;
    bool on = true;
    for (t = 0; t < 600000 && on; ++t) {
        if (t >= next) on = vace::run(p, t, bma, s, &next);
        if (s.song_n != song) { song = s.song_n; printf("song %d at %u ms\n", song, t); }
        if (every || t % 20 == 0) {
            const unsigned long h = s.hash();
            if (h == last) continue;
            last = h;
            char fn[300];
            snprintf(fn, sizeof fn, "%s/%05d.ppm", argv[3], n);
            FILE* f = fopen(fn, "wb");
            if (!f) continue;
            fprintf(f, "P6 320 200 255\n");
            uint8_t o[3];
            for (int i = 0; i < 64000; ++i) { s.rgb(s.px[i], o); fwrite(o, 1, 3, f); }
            fclose(f);
            printf("frame %d at %u ms\n", n++, t);
        }
    }
    printf("ended at %u ms\n", t);
    return 0;
}
