// mm_wav <MUSIC.DAX> <song> <out.wav> : the engine's renderer end to end (the
// player's driver in the x86 interpreter -> ymfm OPL2 -> 8-bit 11,025 Hz WAV,
// the trailing silence cut as the scan does). Output stays in scratch.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "engine/music.h"
#include "engine/opl.h"
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
static void put(const int16_t* s, uint32_t n, void* u) { static_cast<std::vector<int16_t>*>(u)->insert(static_cast<std::vector<int16_t>*>(u)->end(), s, s + n); }
int main(int argc, char** argv) {
    if (argc < 4) return 2;
    Mem m;
    FILE* f = fopen(argv[1], "rb");
    fseek(f, 0, SEEK_END); m.d.resize(ftell(f)); fseek(f, 0, SEEK_SET);
    if (fread(m.d.data(), 1, m.d.size(), f) != m.d.size()) return 1;
    std::vector<int16_t> pcm;
    music::Opl2Sink sink(music::kRate, put, &pcm);
    double secs = 0;
    const music::Result r = music::render_micromagic(m, atoi(argv[2]), sink, 600, 1.5, &secs);
    sink.flush();
    uint32_t n = sink.last_sound() + music::kRate / 2;
    if (n > pcm.size()) n = pcm.size();
    FILE* o = fopen(argv[3], "wb");
    auto w32 = [&](uint32_t x) { fwrite(&x, 4, 1, o); };
    auto w16 = [&](uint16_t x) { fwrite(&x, 2, 1, o); };
    fwrite("RIFF", 1, 4, o); w32(36 + 2 * n); fwrite("WAVEfmt ", 1, 8, o); w32(16); w16(1); w16(1);
    w32(music::kRate); w32(2 * music::kRate); w16(2); w16(16); fwrite("data", 1, 4, o); w32(2 * n);
    fwrite(pcm.data(), 2, n, o);
    fclose(o);
    int lo = 32767, hi = -32768;
    for (uint32_t i = 0; i < n; ++i) { if (pcm[i] < lo) lo = pcm[i]; if (pcm[i] > hi) hi = pcm[i]; }
    fprintf(stderr, "%s: %.1f s played, %.1f s kept, samples %d..%d\n", music::result_text(r), secs, n / double(music::kRate), lo, hi);
    return r == music::Result::Ok ? 0 : 1;
}
