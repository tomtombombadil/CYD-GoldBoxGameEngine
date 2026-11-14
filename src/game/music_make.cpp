// The scan's music files (music_make.h). Own code.
#include "music_make.h"

#include <cstdio>
#include <cstring>
#include <new>

#include "engine/games.h"
#include "engine/music.h"
#include "engine/opl.h"
#include "platform/file.h"
#include "platform/memlog.h"
#include "platform/platform.h"

namespace musicfiles {

namespace {

constexpr const char* kFile = "MUSIC.DAX";

bool open_dax(const char* data_dir, plat::File& f) {
    char p[200];
    games::path_of(data_dir, kFile, p, sizeof p);
    f = plat::open(p, "r");
    return static_cast<bool>(f);
}

void le32(uint8_t* p, uint32_t v) { for (int i = 0; i < 4; ++i) p[i] = static_cast<uint8_t>(v >> (8 * i)); }

void wav_header(uint8_t* h, uint32_t n) {   // (n: the data's bytes)
    memcpy(h, "RIFF", 4); le32(h + 4, 36 + n);
    memcpy(h + 8, "WAVEfmt ", 8); le32(h + 16, 16);
    h[20] = 1; h[21] = 0; h[22] = 1; h[23] = 0;               // PCM, mono
    le32(h + 24, music::kRate); le32(h + 28, music::kRate * 2);
    h[32] = 2; h[33] = 0; h[34] = 16; h[35] = 0;              // 2 bytes a sample, 16 bits
    memcpy(h + 36, "data", 4); le32(h + 40, n);
}

// The renderer's samples go to the file - a quiet stretch held back (as a
// count: it's written as silence when sound follows, dropped at the end
// past a short tail) so the file stops where the music does
struct Writer {
    plat::File f;
    uint32_t written = 0;       // samples in the file
    uint32_t quiet = 0;         // quiet samples held back
    bool ok = true;
    uint8_t buf[512];
    int nbuf = 0;
    void flush() {
        if (nbuf && f.write(buf, static_cast<size_t>(nbuf)) != static_cast<size_t>(nbuf)) ok = false;
        nbuf = 0;
    }
    void put(int16_t v) {                       // (little-endian, as WAV)
        buf[nbuf++] = static_cast<uint8_t>(v);
        buf[nbuf++] = static_cast<uint8_t>(static_cast<uint16_t>(v) >> 8);
        ++written;
        if (nbuf == static_cast<int>(sizeof buf)) flush();
    }
    static void take(const int16_t* s, uint32_t n, void* u) {
        Writer& w = *static_cast<Writer*>(u);
        for (uint32_t i = 0; i < n; ++i) {
            const int16_t v = s[i];
            if (v >= -512 && v <= 512) { ++w.quiet; continue; }
            for (; w.quiet; --w.quiet) w.put(0);
            w.put(v);
        }
    }
    void finish(uint32_t tail) {
        for (uint32_t k = quiet < tail ? quiet : tail; k; --k) put(0);
        quiet = 0;
        flush();
    }
};

}  // namespace

bool ready(const char* data_dir, const char* cache_dir) {
    const uint32_t m = songs(data_dir);
    for (int n = 1; n < 32; ++n) {
        if (!(m >> n & 1)) continue;
        char p[200];
        path(cache_dir, n, p, sizeof p);
        if (!have(p)) return false;
    }
    return true;
}

int make(const char* data_dir, const char* cache_dir, const char* game_name, bool force, Say say, void* user) {
    const uint32_t m = songs(data_dir);
    if (m && !plat::exists(cache_dir)) plat::mkdir(cache_dir);
    int failed = 0, count = 0, done = 0;
    for (int n = 1; n < 32; ++n) count += (m >> n) & 1;
    for (int n = 1; n < 32; ++n) {
        if (!(m >> n & 1)) continue;
        ++done;
        char out[200], tmp[200], line[160];
        path(cache_dir, n, out, sizeof out);
        if (!force && have(out)) continue;
        snprintf(line, sizeof line, "Rendering the %s music, song %d of %d...", game_name, done, count);
        if (say) say(line, false, user);
        snprintf(tmp, sizeof tmp, "%s/MUSIC.TMP", cache_dir);
        const uint32_t t0 = plat::now_ms();

        plat::File dax;
        Writer* w = new (std::nothrow) Writer;
        music::Result r = music::Result::NoMemory;
        const char* why = nullptr;
        double secs = 0;
        if (w && !open_dax(data_dir, dax)) why = "MUSIC.DAX didn't open";
        if (w && !why) {
            w->f = plat::open(tmp, "w");
            uint8_t head[music::kWavHeader] = {};
            if (!w->f) why = "couldn't write on the card";
            if (w->f && w->f.write(head, sizeof head) == sizeof head) {
                plat::FileSource* src = new (std::nothrow) plat::FileSource(dax);
                music::Opl2Sink* chip = new (std::nothrow) music::Opl2Sink(music::kRate, Writer::take, w);
                if (src && chip && chip->ok()) {
                    r = music::render_micromagic(*src, n, *chip, 600, 1.5, &secs);
                    chip->flush();
                    w->finish(music::kRate / 2);
                }
                delete chip;
                delete src;
                wav_header(head, w->written * 2);       // (bytes: 2 a sample)
                if (!w->f.seek(0) || w->f.write(head, sizeof head) != sizeof head) w->ok = false;
            } else {
                w->ok = false;
            }
            w->f.close();
            dax.close();
        }
        const bool good = w && w->ok && (r == music::Result::Ok || r == music::Result::TooLong) && w->written > 0;
        if (good) plat::remove(out);
        if (good && plat::rename(tmp, out)) {
            snprintf(line, sizeof line, "Rendered the %s music, song %d (%.0f s of music, %lu s to make)", game_name, n,
                     w->written / static_cast<double>(music::kRate),
                     static_cast<unsigned long>((plat::now_ms() - t0) / 1000));
        } else {
            plat::remove(tmp);
            ++failed;
            snprintf(line, sizeof line, "The %s music, song %d, wasn't rendered: %s. The next start tries again.",
                     game_name, n,
                     why ? why : good ? "couldn't save it on the card" : w && !w->ok ? "the card is full?" : music::result_text(r));
            memlog::note(line);
        }
        delete w;
        if (say) say(line, true, user);
    }
    return failed;
}

}  // namespace musicfiles
