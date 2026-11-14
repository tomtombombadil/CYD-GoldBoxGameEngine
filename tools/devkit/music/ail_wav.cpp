// ail_wav <ADLIB.ADV> <file.XMI> <INSTR.AD> <sequence> <out.wav> [log] : the
// engine's AIL renderer end to end (the Dark Queen's music: the player's
// driver in the x86 interpreter -> ymfm OPL2 -> 16-bit WAV). "log" prints
// every chip write with its tick (serve calls before it) (for the Unicorn oracle's compare).
// Output stays in scratch.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "engine/music.h"
#include "engine/opl.h"
uint32_t g_now = 0;
struct Mem : dax::ByteSource {
    std::vector<uint8_t> d;
    bool load(const char* p) {
        FILE* f = fopen(p, "rb");
        if (!f) return false;
        fseek(f, 0, SEEK_END); d.resize(ftell(f)); fseek(f, 0, SEEK_SET);
        bool ok = fread(d.data(), 1, d.size(), f) == d.size();
        fclose(f);
        return ok;
    }
    size_t read_at(uint32_t pos, uint8_t* buf, size_t n) override {
        if (pos >= d.size()) return 0;
        if (pos + n > d.size()) n = d.size() - pos;
        memcpy(buf, d.data() + pos, n);
        return n;
    }
    uint32_t size() const override { return d.size(); }
};
struct Log : music::Sink {
    music::Sink* next; long tick = 0; bool print;   // (a tick = one serve call)
    void opl(uint8_t r, uint8_t v) override { if (print) printf("%ld %02X %02X\n", tick, r, v); next->opl(r, v); }
    void advance(double s) override { ++tick; next->advance(s); }
};
static void put(const int16_t* s, uint32_t n, void* u) { static_cast<std::vector<int16_t>*>(u)->insert(static_cast<std::vector<int16_t>*>(u)->end(), s, s + n); }
int main(int argc, char** argv) {
    if (argc < 6) return 2;
    Mem adv, xmi, gtl;
    if (!adv.load(argv[1]) || !xmi.load(argv[2]) || !gtl.load(argv[3])) return 1;
    std::vector<int16_t> pcm;
    music::Opl2Sink chip(music::kRate, put, &pcm);
    Log log; log.next = &chip; log.print = argc > 6;
    double secs = 0;
    const music::Result r = music::render_ail(adv, xmi, gtl, atoi(argv[4]), log, 600, 1.5, &secs);
    chip.flush();
    uint32_t n = chip.last_sound() + music::kRate / 2;
    if (n > pcm.size()) n = pcm.size();
    FILE* o = fopen(argv[5], "wb");
    auto w32 = [&](uint32_t x) { fwrite(&x, 4, 1, o); };
    auto w16 = [&](uint16_t x) { fwrite(&x, 2, 1, o); };
    fwrite("RIFF", 1, 4, o); w32(36 + 2 * n); fwrite("WAVEfmt ", 1, 8, o); w32(16); w16(1); w16(1);
    w32(music::kRate); w32(2 * music::kRate); w16(2); w16(16); fwrite("data", 1, 4, o); w32(2 * n);
    fwrite(pcm.data(), 2, n, o);
    fclose(o);
    fprintf(stderr, "%s: %.1f s played, %.1f s kept\n", music::result_text(r), secs, n / double(music::kRate));
    return r == music::Result::Ok ? 0 : 1;
}
