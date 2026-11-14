// ea_wav <MIDI.DAX> <driver block> <SOUND.DAX> <song block> <pit divisor> <out.wav> [log] : the engine's
// Electronic Arts renderer end to end (Champions' title: the player's driver in the x86 interpreter ->
// ymfm OPL2 -> 16-bit WAV). "log" prints the chip writes as ealog.py does ("T tick", "reg val").
// Champions (AdLib): MIDI.DAX 3, SOUND.DAX 1, pit 21845 (0x5555). Output stays in scratch.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "engine/dax.h"
#include "engine/music.h"
#include "engine/opl.h"
uint32_t g_now = 0;
static std::vector<uint8_t> block(const char* path, int id) {
    std::vector<uint8_t> f, out;
    FILE* fp = fopen(path, "rb");
    if (!fp) return out;
    fseek(fp, 0, SEEK_END); f.resize(ftell(fp)); fseek(fp, 0, SEEK_SET);
    if (fread(f.data(), 1, f.size(), fp) != f.size()) f.clear();
    fclose(fp);
    dax::MemorySource src(f.data(), f.size());
    static dax::Index index;
    if (dax::read_index(src, index) != dax::Status::Ok) return out;
    for (int i = 0; i < index.count; ++i)
        if (index.entries[i].id == id) {
            out.resize(index.entries[i].raw_size);
            out.resize(dax::load_block(src, index, index.entries[i], out.data()));
        }
    return out;
}
struct Log : music::Sink {
    music::Sink* next; long tick = 0; long shown = -1; bool print;
    void opl(uint8_t r, uint8_t v) override {
        if (print) { if (tick != shown) { printf("T %ld\n", tick); shown = tick; } printf("%02X %02X\n", r, v); }
        next->opl(r, v);
    }
    void advance(double s) override { ++tick; next->advance(s); }
};
static void put(const int16_t* s, uint32_t n, void* u) { static_cast<std::vector<int16_t>*>(u)->insert(static_cast<std::vector<int16_t>*>(u)->end(), s, s + n); }
int main(int argc, char** argv) {
    if (argc < 7) return 2;
    auto drv = block(argv[1], atoi(argv[2])), song = block(argv[3], atoi(argv[4]));
    std::vector<int16_t> pcm;
    music::Opl2Sink chip(music::kRate, put, &pcm);
    Log log; log.next = &chip; log.print = argc > 7;
    double secs = 0;
    const music::Result r = music::render_ea(drv.data(), drv.size(), song.data(), song.size(),
                                             static_cast<uint16_t>(atoi(argv[5])), log, 600, 1.5, &secs);
    chip.flush();
    uint32_t n = chip.last_sound() + music::kRate / 2;
    if (n > pcm.size()) n = pcm.size();
    FILE* o = fopen(argv[6], "wb");
    auto w32 = [&](uint32_t x) { fwrite(&x, 4, 1, o); };
    auto w16 = [&](uint16_t x) { fwrite(&x, 2, 1, o); };
    fwrite("RIFF", 1, 4, o); w32(36 + 2 * n); fwrite("WAVEfmt ", 1, 8, o); w32(16); w16(1); w16(1);
    w32(music::kRate); w32(2 * music::kRate); w16(2); w16(16); fwrite("data", 1, 4, o); w32(2 * n);
    fwrite(pcm.data(), 2, n, o);
    fclose(o);
    fprintf(stderr, "%s: %.1f s played, %.1f s kept\n", music::result_text(r), secs, n / double(music::kRate));
    return r == music::Result::Ok ? 0 : 1;
}
