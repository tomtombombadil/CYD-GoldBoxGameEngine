// mm_log <MUSIC.DAX> <song> : the engine's music::render_micromagic (the x86
// interpreter running the player's driver) as a write log: one "reg val"
// line a write, "T <tick>" before each tick's writes (the init's writes
// first, with no T) - the same form as mmlog.py's (the driver run in Unicorn),
// so the two can be compared line for line.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "engine/music.h"
uint32_t g_now = 0;
struct Mem : dax::ByteSource {
    std::vector<uint8_t> d;
    size_t read_at(uint32_t pos, uint8_t* buf, size_t n) override {
        if (pos >= d.size()) return 0;
        if (pos + n > d.size()) n = d.size() - pos;
        for (size_t i = 0; i < n; ++i) buf[i] = d[pos + i];
        return n;
    }
    uint32_t size() const override { return d.size(); }
};
struct Log : music::Sink {
    long tick = -1; long shown = -2; double hz = 0;
    void opl(uint8_t r, uint8_t v) override {
        if (tick >= 0 && tick != shown) { printf("T %ld\n", tick); shown = tick; }
        printf("%02X %02X\n", r, v);
    }
    void advance(double s) override { if (!hz) hz = 1 / s; ++tick; }
};
int main(int argc, char** argv) {
    Mem m;
    FILE* f = fopen(argv[1], "rb");
    fseek(f, 0, SEEK_END); m.d.resize(ftell(f)); fseek(f, 0, SEEK_SET);
    if (fread(m.d.data(), 1, m.d.size(), f) != m.d.size()) return 1;
    Log log;
    log.tick = 0;            // (the init writes go out before tick 0 - see below)
    log.shown = 0;
    double secs = 0;
    const music::Result r = music::render_micromagic(m, atoi(argv[2]), log, 600, 0, &secs);
    fprintf(stderr, "%s, %.1f s, %ld ticks at %.2f Hz\n", music::result_text(r), secs, log.tick, log.hz);
    return r == music::Result::Ok ? 0 : 1;
}
