// The scan's music files (music_make.h). Own code.
#include "music_make.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include "engine/games.h"
#include "engine/music.h"
#include "engine/opl.h"
#include "engine/profile.h"
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

// The Dark Queen's song n: its program's tables give the XMIDI file and
// the sequence in it; the driver, the file and the timbres are read into
// memory (~27 KB for the GOG files) and closed - the card's 3 file slots,
// the WAV being written in one
struct AilFiles {
    uint8_t* buf[3] = {};
    uint32_t len[3] = {};
    int seq = 0;
    ~AilFiles() { for (auto* b : buf) free(b); }
    const char* open(const char* data_dir, const profile::Profile& p, int song) {
        const auto& m = p.music_ail;
        char path[200], name[64];
        games::path_of(data_dir, p.program, path, sizeof path);
        plat::File prog = plat::open(path, "r");
        uint8_t fw[2] = {}, sw[2] = {}, prefix[13] = {};
        const uint32_t at = 2u * static_cast<uint32_t>(song - 1);
        const bool got = prog && prog.seek(m.files + at) && prog.read(fw, 2) == 2 && prog.seek(m.seqs + at) &&
                         prog.read(sw, 2) == 2 && prog.seek(m.prefix) && prog.read(prefix, 12) == 12;
        prog.close();
        if (!got || !prefix[0]) return "its program's song table didn't read";
        seq = sw[0] | sw[1] << 8;
        char xname[24];
        snprintf(xname, sizeof xname, "%s%d.XMI", reinterpret_cast<const char*>(prefix), fw[0] | fw[1] << 8);
        const char* names[3] = {m.driver, xname, m.timbres};
        for (int k = 0; k < 3; ++k) {
            snprintf(name, sizeof name, "%s/%s", m.dir, names[k]);
            games::path_of(data_dir, name, path, sizeof path);
            plat::File f = plat::open(path, "r");
            if (!f) return k == 1 ? "its XMI file didn't open" : k ? "INSTR.AD didn't open" : "ADLIB.ADV didn't open";
            len[k] = f.size();
            buf[k] = len[k] && len[k] < 0x10000 ? static_cast<uint8_t*>(malloc(len[k])) : nullptr;
            const bool ok = buf[k] && f.read(buf[k], len[k]) == static_cast<int>(len[k]);
            f.close();
            if (!ok) return buf[k] ? "a music file didn't read" : "not enough memory";
        }
        return nullptr;
    }
    music::Result render(music::Sink& sink, double* secs) {
        dax::MemorySource a(buf[0], len[0]), x(buf[1], len[1]), g(buf[2], len[2]);
        return music::render_ail(a, x, g, seq, sink, 600, 1.5, secs);
    }
};

// Champions' title tune: the driver's and the song's DAX blocks read into
// memory (~16 KB)
struct EaFiles {
    uint8_t* buf[2] = {};
    uint32_t len[2] = {};
    ~EaFiles() { for (auto* b : buf) free(b); }
    const char* open(const char* data_dir, const profile::Profile& p) {
        const auto& m = p.music_ea;
        const char* files[2] = {m.driver_file, m.song_file};
        const uint8_t ids[2] = {m.driver_block, m.song_block};
        dax::Index* index = new (std::nothrow) dax::Index;
        if (!index) return "not enough memory";
        const char* why = nullptr;
        for (int k = 0; k < 2 && !why; ++k) {
            char path[200];
            games::path_of(data_dir, files[k], path, sizeof path);
            plat::File f = plat::open(path, "r");
            if (!f) { why = k ? "SOUND.DAX didn't open" : "MIDI.DAX didn't open"; break; }
            plat::FileSource src(f);
            const dax::Entry* e = nullptr;
            if (dax::read_index(src, *index) == dax::Status::Ok)
                for (int i = 0; i < index->count; ++i)
                    if (index->entries[i].id == ids[k]) e = &index->entries[i];
            if (!e) why = "its music block isn't there";
            else if (!(buf[k] = static_cast<uint8_t*>(malloc(e->raw_size ? e->raw_size : 1)))) why = "not enough memory";
            else len[k] = dax::load_block(src, *index, *e, buf[k]);
            f.close();
        }
        delete index;
        return why;
    }
    music::Result render(const profile::Profile& p, music::Sink& sink, double* secs) {
        return music::render_ea(buf[0], len[0], buf[1], len[1], p.music_ea.pit, sink, 600, 1.5, secs);
    }
};

}  // namespace

bool ready(const char* data_dir, const char* cache_dir, const profile::Profile* p) {
    const uint32_t m = songs(data_dir, p);
    for (int n = 1; n < 32; ++n) {
        if (!(m >> n & 1)) continue;
        char p[200];
        path(cache_dir, n, p, sizeof p);
        if (!have(p)) return false;
    }
    return true;
}

int make(const char* data_dir, const char* cache_dir, const profile::Profile* p, const char* game_name, bool force, Say say,
         void* user) {
    const uint32_t m = songs(data_dir, p);
    const bool ail = p && p->music_ail.count;
    const bool ea = p && p->music_ea.driver_file;
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
        AilFiles* af = nullptr;
        EaFiles* ef = nullptr;
        if (w && ea) {
            ef = new (std::nothrow) EaFiles;
            why = ef ? ef->open(data_dir, *p) : "not enough memory";
        } else if (w && ail) {
            af = new (std::nothrow) AilFiles;
            why = af ? af->open(data_dir, *p, n) : "not enough memory";
        } else if (w && !open_dax(data_dir, dax)) {
            why = "MUSIC.DAX didn't open";
        }
        if (w && !why) {
            w->f = plat::open(tmp, "w");
            uint8_t head[music::kWavHeader] = {};
            if (!w->f) why = "couldn't write on the card";
            if (w->f && w->f.write(head, sizeof head) == sizeof head) {
                plat::FileSource* src = ail || ea ? nullptr : new (std::nothrow) plat::FileSource(dax);
                music::Opl2Sink* chip = new (std::nothrow) music::Opl2Sink(music::kRate, Writer::take, w);
                if ((src || ail || ea) && chip && chip->ok()) {
                    r = ea    ? ef->render(*p, *chip, &secs)
                        : ail ? af->render(*chip, &secs)
                              : music::render_micromagic(*src, n, *chip, 600, 1.5, &secs);
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
        delete af;
        delete ef;
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
