// The games' music rendered at scan time (music.h). Own code.
#include "music.h"

#include <cstdlib>
#include <initializer_list>
#include <cstring>
#include <new>

#include "x86.h"

namespace music {

const char* result_text(Result r) {
    switch (r) {
    case Result::Ok: return "ok";
    case Result::NoDriver: return "no music driver";
    case Result::NoSong: return "no such song";
    case Result::NoMemory: return "not enough memory";
    case Result::DriverFailed: return "the music driver stopped";
    case Result::TooLong: return "cut off at the time limit";
    }
    return "?";
}

namespace {

// Where things sit in the driver's 1 MB
constexpr uint16_t kDrvSeg = 0x1000;                 // the driver: kDrvSeg:0100
constexpr uint16_t kSongSeg = 0x3000;
constexpr uint16_t kStackSeg = 0x7000;
constexpr uint32_t kStackLo = 0x7F800, kStackHi = 0x80000;
constexpr uint32_t kLowHi = 0x600;                   // vectors, BIOS data, the IRET stub
constexpr uint32_t kIretAt = 0x500;                  // the old INT 1Ch: an IRET
constexpr uint32_t kDrvSlack = 0x1000;               // room past the image for its data

class Bus : public x86::Bus {
public:
    uint8_t* drv = nullptr;   uint32_t drv_len = 0;  // from kDrvSeg:0000
    uint8_t* song = nullptr;  uint32_t song_len = 0;
    uint8_t  low[kLowHi] = {};
    uint8_t  stack[kStackHi - kStackLo] = {};
    uint8_t  index = 0;
    Sink*    sink = nullptr;
    uint16_t pit = 0;
    int      pit_bytes = 0;
    uint32_t stray = 0;                              // accesses outside the windows

    uint8_t* at(uint32_t a) {
        if (a < kLowHi) return &low[a];
        const uint32_t d = a - static_cast<uint32_t>(kDrvSeg) * 16;
        if (d < drv_len) return &drv[d];
        const uint32_t s = a - static_cast<uint32_t>(kSongSeg) * 16;
        if (s < song_len) return &song[s];
        if (a >= kStackLo && a < kStackHi) return &stack[a - kStackLo];
        return nullptr;
    }
    uint8_t read(uint32_t a) override {
        if (uint8_t* p = at(a)) return *p;
        ++stray;
        return 0;
    }
    void write(uint32_t a, uint8_t v) override {
        if (uint8_t* p = at(a)) *p = v;
        else ++stray;
    }
    // The AdLib status port reads 0 (no timer flags), as the original's
    // checks met it when the songs were compared; the rest 0 too.
    uint8_t in(uint16_t) override { return 0; }
    void out(uint16_t port, uint8_t v) override {
        if (port == 0x388) index = v;
        else if (port == 0x389) { if (sink) sink->opl(index, v); }
        else if (port == 0x40) { pit = static_cast<uint16_t>(pit_bytes++ & 1 ? (pit & 0xFF) | v << 8 : v); }
        else if (port == 0x43) pit_bytes = 0;
    }
};

// BIOS services a driver asks for: the clock (INT 1Ah) reads 0
bool int_hook(x86::Cpu& cpu, uint8_t n, void*) {
    if (n == 0x1A) {
        cpu.r[x86::AX] = 0;
        cpu.r[x86::CX] = 0;
        cpu.r[x86::DX] = 0;
        return true;
    }
    return false;
}

bool load_block(dax::ByteSource& src, uint8_t id, uint8_t* out, uint32_t cap, uint32_t* len) {
    dax::Entry e;
    uint32_t start = 0;
    if (!dax::find_entry(src, id, e, start) || e.raw_size > cap) return false;
    dax::RleReader rd(src, start, e);
    *len = static_cast<uint32_t>(rd.read(out, e.raw_size));
    return *len == e.raw_size && !rd.error();
}

uint32_t block_size(dax::ByteSource& src, uint8_t id) {
    dax::Entry e;
    uint32_t start = 0;
    return dax::find_entry(src, id, e, start) ? e.raw_size : 0;
}

constexpr uint32_t kSteps = 2000000;                 // a call's instruction budget

}  // namespace

uint32_t micromagic_songs(dax::ByteSource& music_dax) {
    uint32_t mask = 0;
    for (int i = 1; i < 32; ++i)
        if (block_size(music_dax, static_cast<uint8_t>(i))) mask |= 1u << i;
    return mask;
}

Result render_micromagic(dax::ByteSource& src, int song, Sink& sink, double max_seconds, double tail,
                         double* seconds) {
    if (seconds) *seconds = 0;
    const uint32_t dsize = block_size(src, 0);
    if (dsize < 0x400 || dsize > 0x8000) return Result::NoDriver;
    const uint32_t ssize = song > 0 && song < 256 ? block_size(src, static_cast<uint8_t>(song)) : 0;
    if (ssize < 16) return Result::NoSong;

    Bus* bus = static_cast<Bus*>(calloc(1, sizeof(Bus)));
    if (!bus) return Result::NoMemory;
    new (bus) Bus();
    Result res = Result::Ok;
    bus->drv_len = 0x100 + dsize + kDrvSlack;
    bus->drv = static_cast<uint8_t*>(calloc(1, bus->drv_len));
    bus->song = static_cast<uint8_t*>(calloc(1, ssize));
    x86::Cpu* cpu = nullptr;
    do {
        if (!bus->drv || !bus->song) { res = Result::NoMemory; break; }
        uint32_t got = 0;
        if (!load_block(src, 0, bus->drv + 0x100, dsize, &got)) { res = Result::NoDriver; break; }
        if (!load_block(src, static_cast<uint8_t>(song), bus->song, ssize, &got)) { res = Result::NoSong; break; }
        bus->song_len = got;
        if (memcmp(bus->song, "GLBR", 4) != 0) { res = Result::NoSong; break; }
        // the old INT 1Ch: an IRET; vector 1Ch -> it
        bus->low[kIretAt] = 0xCF;
        bus->low[0x70] = 0; bus->low[0x71] = 0;
        bus->low[0x72] = static_cast<uint8_t>(kIretAt >> 4); bus->low[0x73] = 0;
        bus->sink = &sink;

        cpu = static_cast<x86::Cpu*>(malloc(sizeof(x86::Cpu)));
        if (!cpu) { res = Result::NoMemory; break; }
        new (cpu) x86::Cpu(*bus);
        cpu->int_hook = int_hook;
        cpu->s[x86::SS] = kStackSeg;
        cpu->r[x86::SP] = 0xFFF0;
        cpu->s[x86::DS] = cpu->s[x86::ES] = kSongSeg;

        auto call = [&](uint16_t cmd, uint16_t a, uint16_t b) {
            const uint16_t w[3] = {b, a, cmd};
            return cpu->far_call(kDrvSeg, 0x100, w, 3, kSteps);
        };
        if (call(0, 0, kSongSeg) != x86::Stop::Returned) { res = Result::DriverFailed; break; }
        // its tick handler, now in vector 1Ch, and the counter it increments
        const uint16_t hoff = static_cast<uint16_t>(bus->low[0x70] | bus->low[0x71] << 8);
        const uint16_t hseg = static_cast<uint16_t>(bus->low[0x72] | bus->low[0x73] << 8);
        if (hseg != kDrvSeg) { res = Result::NoDriver; break; }
        int clock = -1;
        for (int i = 0; i < 24 && clock < 0; ++i)
            if (cpu->rb(hseg, static_cast<uint16_t>(hoff + i)) == 0x2E &&
                cpu->rb(hseg, static_cast<uint16_t>(hoff + i + 1)) == 0xFF &&
                cpu->rb(hseg, static_cast<uint16_t>(hoff + i + 2)) == 0x06)
                clock = cpu->rw(hseg, static_cast<uint16_t>(hoff + i + 3));
        if (clock < 0) { res = Result::NoDriver; break; }
        const double hz = 1193182.0 / (bus->pit ? bus->pit : 65536);
        const double dt = 1.0 / hz;
        cpu->ww(hseg, static_cast<uint16_t>(clock), 1);   // the clock running
        if (call(1, 0, 0) != x86::Stop::Returned) { res = Result::DriverFailed; break; }

        double t = 0;
        for (;;) {
            if (cpu->interrupt(hseg, hoff, kSteps) != x86::Stop::Returned) { res = Result::DriverFailed; break; }
            sink.advance(dt);
            t += dt;
            if (call(2, 0, 0) != x86::Stop::Returned) { res = Result::DriverFailed; break; }
            if (cpu->r[x86::AX] == 0) break;
            if (t >= max_seconds) { res = Result::TooLong; break; }
        }
        if (res == Result::TooLong) call(3, 0, 0);
        if (res == Result::Ok || res == Result::TooLong) { sink.advance(tail); t += tail; }
        if (seconds) *seconds = t;
    } while (false);
    if (cpu) { cpu->~Cpu(); free(cpu); }
    free(bus->drv);
    free(bus->song);
    bus->~Bus();
    free(bus);
    return res;
}

// ---- Miles' Audio Interface Library (the Dark Queen) ------------------------
//
// The Dark Queen's music: XMIDI files (ADDQ1-3.XMI) played by Miles' AIL 2
// AdLib driver (ADLIB.ADV) with its timbres from INSTR.AD (a Global Timbre
// Library) - all the player's own files, the driver run as DQK.EXE runs it
// (its AIL API, from the AIL 2 documentation's API - facts, our own code):
//   - the driver at a segment's offset 0; word 0 = the offset of its
//     function table: (function number, offset) word pairs to number 0xFFFF;
//     each a far cdecl function whose first argument is the driver handle
//   - 100 describe (DX:AX -> its description: +12 I/O, +14 IRQ, +16 DMA,
//     +18 DRQ, +20 service rate in Hz), 102 init(h, io, irq, dma, drq),
//     103 serve() (no arguments - the API's timer calls it at the service
//     rate), 150 state table size, 151 register sequence(h, file far,
//     sequence number, state far, controllers far NULL) -> handle or -1,
//     153 timbre cache size, 154 define cache(h, far, size), 155 timbre
//     request(h, seq) -> bank << 8 | patch, 0xFFFF none, 156 install
//     timbre(h, bank, patch, far timbre), 170 start(h, seq), 174
//     status(h, seq): 2 done
//   - the GTL: 6-byte entries (patch, bank, u32 offset) to bank 0xFF; at the
//     offset a timbre: u16 length (itself included), the rest
namespace {

class WinBus : public x86::Bus {
public:
    struct Win { uint32_t base, size; uint8_t* p; };
    Win w[8] = {};
    int n = 0;
    uint8_t index = 0;
    Sink* sink = nullptr;
    uint32_t stray = 0;
    bool add(uint32_t base, uint32_t size) {
        if (n == 8) return false;
        uint8_t* p = static_cast<uint8_t*>(calloc(1, size));
        if (!p) return false;
        w[n++] = {base, size, p};
        return true;
    }
    ~WinBus() override { for (int i = 0; i < n; ++i) free(w[i].p); }
    uint8_t* at(uint32_t a) {
        for (int i = 0; i < n; ++i)
            if (a - w[i].base < w[i].size) return w[i].p + (a - w[i].base);
        return nullptr;
    }
    uint8_t read(uint32_t a) override {
        if (uint8_t* q = at(a)) return *q;
        ++stray;
        return 0;
    }
    void write(uint32_t a, uint8_t v) override {
        if (uint8_t* q = at(a)) *q = v;
        else ++stray;
    }
    uint8_t in(uint16_t) override { return 0; }
    void out(uint16_t port, uint8_t v) override {
        if (port == 0x388) index = v;
        else if (port == 0x389 && sink) sink->opl(index, v);
    }
};

bool read_all(dax::ByteSource& src, uint8_t* to, uint32_t n) {
    return src.read_at(0, to, n) == n;
}

}  // namespace

Result render_ail(dax::ByteSource& adv, dax::ByteSource& xmi, dax::ByteSource& gtl, int seq, Sink& sink,
                  double max_seconds, double tail, double* seconds) {
    if (seconds) *seconds = 0;
    const uint32_t asize = adv.size(), xsize = xmi.size(), gsize = gtl.size();
    if (asize < 0x100 || asize > 0xC000) return Result::NoDriver;
    if (xsize < 16 || xsize > 0xF000 || gsize < 6 || gsize > 0xF000) return Result::NoSong;
    constexpr uint16_t kDrv = 0x1000, kXmi = 0x2000, kGtl = 0x3000, kState = 0x4000, kCache = 0x5000,
                       kStack = 0x7000;
    constexpr uint32_t kStateMax = 0x2000, kCacheMax = 0x4000;
    WinBus* bus = new (std::nothrow) WinBus();
    if (!bus) return Result::NoMemory;
    bus->sink = &sink;
    x86::Cpu* cpu = nullptr;
    Result res = Result::Ok;
    do {
        if (!bus->add(0, 0x600) || !bus->add(kDrv * 16u, asize + 0x1000) || !bus->add(kXmi * 16u, xsize) ||
            !bus->add(kGtl * 16u, gsize) || !bus->add(0x7F800, 0x800)) {
            res = Result::NoMemory;
            break;
        }
        if (!read_all(adv, bus->at(kDrv * 16u), asize)) { res = Result::NoDriver; break; }
        if (!read_all(xmi, bus->at(kXmi * 16u), xsize) || !read_all(gtl, bus->at(kGtl * 16u), gsize)) {
            res = Result::NoSong;
            break;
        }
        const uint8_t* drv = bus->at(kDrv * 16u);
        if (memcmp(drv + 2, "Copyright", 9) != 0) { res = Result::NoDriver; break; }
        cpu = new (std::nothrow) x86::Cpu(*bus);
        if (!cpu) { res = Result::NoMemory; break; }
        cpu->int_hook = int_hook;
        cpu->s[x86::SS] = kStack;
        cpu->r[x86::SP] = 0xFFF0;
        cpu->s[x86::DS] = cpu->s[x86::ES] = kXmi;
        // the function table
        auto fn = [&](uint16_t num) -> int {
            uint32_t at = drv[0] | drv[1] << 8;
            for (int k = 0; k < 64 && at + 4 <= asize; ++k, at += 4) {
                const uint16_t f = static_cast<uint16_t>(drv[at] | drv[at + 1] << 8);
                if (f == 0xFFFF) break;
                if (f == num) return drv[at + 2] | drv[at + 3] << 8;
            }
            return -1;
        };
        bool ok = true;
        auto call = [&](uint16_t num, std::initializer_list<uint16_t> args) -> uint16_t {
            const int off = fn(num);
            if (off < 0 || !ok) { ok = false; return 0; }
            uint16_t w[10];
            int k = 0;
            for (auto it = args.end(); it != args.begin();) w[k++] = *--it;    // (the last pushed first)
            if (cpu->far_call(kDrv, static_cast<uint16_t>(off), w, k, kSteps) != x86::Stop::Returned) ok = false;
            return cpu->r[x86::AX];
        };
        const uint16_t h = 0;
        call(100, {h});
        const uint16_t dseg = cpu->r[x86::DX], doff = cpu->r[x86::AX];
        if (!ok) { res = Result::DriverFailed; break; }
        const uint16_t io = cpu->rw(dseg, static_cast<uint16_t>(doff + 12));
        const uint16_t rate = cpu->rw(dseg, static_cast<uint16_t>(doff + 20));
        if (rate == 0 || rate == 0xFFFF || rate > 1000) { res = Result::NoDriver; break; }
        call(102, {h, io, cpu->rw(dseg, static_cast<uint16_t>(doff + 14)), cpu->rw(dseg, static_cast<uint16_t>(doff + 16)),
                   cpu->rw(dseg, static_cast<uint16_t>(doff + 18))});
        const uint16_t state = call(150, {h});
        const uint16_t cache = fn(153) >= 0 ? call(153, {h}) : 0;
        if (!ok || state > kStateMax || cache > kCacheMax) { res = Result::DriverFailed; break; }
        if (!bus->add(kState * 16u, state ? state : 16) || (cache && !bus->add(kCache * 16u, cache))) {
            res = Result::NoMemory;
            break;
        }
        if (cache) call(154, {h, 0, kCache, cache});
        const uint16_t hs = call(151, {h, 0, kXmi, static_cast<uint16_t>(seq), 0, kState, 0, 0});
        if (!ok) { res = Result::DriverFailed; break; }
        if (hs == 0xFFFF) { res = Result::NoSong; break; }
        // its timbres from the library
        const uint8_t* lib = bus->at(kGtl * 16u);
        for (int guard = 0; guard < 256; ++guard) {
            const uint16_t t = call(155, {h, hs});
            if (!ok || t == 0xFFFF) break;
            const uint8_t bank = static_cast<uint8_t>(t >> 8), patch = static_cast<uint8_t>(t);
            uint32_t found = 0;
            for (uint32_t e = 0; e + 6 <= gsize && lib[e + 1] != 0xFF; e += 6)
                if (lib[e] == patch && lib[e + 1] == bank) {
                    found = static_cast<uint32_t>(lib[e + 2] | lib[e + 3] << 8 | lib[e + 4] << 16) |
                            static_cast<uint32_t>(lib[e + 5]) << 24;
                    break;
                }
            if (!found || found >= gsize) { ok = false; break; }   // (the original stops there too)
            call(156, {h, bank, patch, static_cast<uint16_t>(found), kGtl});
        }
        if (!ok) { res = Result::DriverFailed; break; }
        call(170, {h, hs});
        const int serve = fn(103);
        if (!ok || serve < 0) { res = Result::DriverFailed; break; }
        const double dt = 1.0 / rate;
        double t = 0;
        for (;;) {
            if (cpu->far_call(kDrv, static_cast<uint16_t>(serve), nullptr, 0, kSteps) != x86::Stop::Returned) {
                res = Result::DriverFailed;
                break;
            }
            sink.advance(dt);
            t += dt;
            const uint16_t st = call(174, {h, hs});
            if (!ok) { res = Result::DriverFailed; break; }
            if (st != 1) break;                                    // (1 playing; 2 done, 0 stopped)
            if (t >= max_seconds) { res = Result::TooLong; call(171, {h, hs}); break; }
        }
        if (res == Result::Ok || res == Result::TooLong) { sink.advance(tail); t += tail; }
        if (seconds) *seconds = t;
    } while (false);
    delete cpu;
    delete bus;
    return res;
}

// ---- Electronic Arts' sound drivers (Champions of Krynn) -----------------------
//
// Champions' title tune: SOUND.DAX's song played by a MIDI.DAX driver (the
// sound type's - block type + 1; AdLib = 3). How START.EXE's sound unit
// uses it (its image 0x661A-0x69BC, our words): the driver is copied to a
// paragraph (offset 0) and called far at offset 0 with AH = the command -
// 0 a tick, 1 play the song at CX:0 (also paragraph-aligned), 2 stop; the
// unit's own INT 8 handler sets the PIT to its divisor and calls the tick
// at every timer interrupt. The song stops itself (or loops - then the
// title's end stops it).
Result render_ea(const uint8_t* drv, uint32_t drv_len, const uint8_t* song, uint32_t song_len, uint16_t pit,
                 Sink& sink, double max_seconds, double tail, double* seconds) {
    if (seconds) *seconds = 0;
    if (!drv || drv_len < 16 || drv_len > 0x8000 || drv[0] != 0xE9) return Result::NoDriver;
    if (!song || song_len < 4 || song_len > 0xF000) return Result::NoSong;
    constexpr uint16_t kDrv = 0x1000, kSong = 0x2000, kStack = 0x7000;
    WinBus* bus = new (std::nothrow) WinBus();
    if (!bus) return Result::NoMemory;
    bus->sink = &sink;
    x86::Cpu* cpu = nullptr;
    Result res = Result::Ok;
    do {
        if (!bus->add(0, 0x600) || !bus->add(kDrv * 16u, drv_len + 0x1000) || !bus->add(kSong * 16u, song_len) ||
            !bus->add(0x7F800, 0x800)) {
            res = Result::NoMemory;
            break;
        }
        memcpy(bus->at(kDrv * 16u), drv, drv_len);
        memcpy(bus->at(kSong * 16u), song, song_len);
        cpu = new (std::nothrow) x86::Cpu(*bus);
        if (!cpu) { res = Result::NoMemory; break; }
        cpu->int_hook = int_hook;
        cpu->s[x86::SS] = kStack;
        cpu->r[x86::SP] = 0xFFF0;
        auto call = [&](uint8_t ah, uint16_t cx) {
            cpu->s[x86::DS] = cpu->s[x86::ES] = kSong;
            cpu->r[x86::AX] = static_cast<uint16_t>(ah << 8);
            cpu->r[x86::CX] = cx;
            return cpu->far_call(kDrv, 0, nullptr, 0, kSteps) == x86::Stop::Returned;
        };
        struct Watch : Sink {               // (the time of the last chip write)
            Sink* next;
            bool wrote = false;
            void opl(uint8_t r, uint8_t v) override { wrote = true; next->opl(r, v); }
            void advance(double s) override { next->advance(s); }
        } watch;
        watch.next = &sink;
        bus->sink = &watch;
        if (!call(1, kSong)) { res = Result::DriverFailed; break; }
        const double dt = (pit ? pit : 65536) / 1193182.0;
        double t = 0, last = 0;
        for (;;) {
            watch.wrote = false;
            if (!call(0, 0)) { res = Result::DriverFailed; break; }
            if (watch.wrote) last = t;
            sink.advance(dt);
            t += dt;
            if (t - last > 8) break;                       // (silent 8 s: it has stopped)
            if (t >= max_seconds) { res = Result::TooLong; call(2, 0); break; }
        }
        if (res == Result::Ok || res == Result::TooLong) {
            sink.advance(tail);
            t += tail;
        }
        if (seconds) *seconds = t;
    } while (false);
    delete cpu;
    delete bus;
    return res;
}

// ---- Stream -------------------------------------------------------------------

namespace {
uint32_t le32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0] | p[1] << 8 | p[2] << 16) | static_cast<uint32_t>(p[3]) << 24;
}
}  // namespace

bool wav_ours(const uint8_t* h) {
    return memcmp(h, "RIFF", 4) == 0 && memcmp(h + 8, "WAVEfmt ", 8) == 0 && h[20] == 1 && h[22] == 1 &&
           le32(h + 24) == kRate && h[34] == 16 && memcmp(h + 36, "data", 4) == 0;
}

uint32_t wav_data(const uint8_t* h) { return le32(h + 40); }

Stream::~Stream() { free(ring_); }

uint32_t Stream::room() const {
    return cap_ - (head_.load() - tail_.load());
}

void Stream::push(const int16_t* p, uint32_t n) {
    uint32_t h = head_.load();
    if (wide_) {
        int16_t* r = reinterpret_cast<int16_t*>(ring_);
        for (uint32_t i = 0; i < n; ++i, ++h) r[h % cap_] = p[i];
    } else {
        for (uint32_t i = 0; i < n; ++i, ++h) ring_[h % cap_] = static_cast<uint8_t>((p[i] + 32768) >> 8);
    }
    head_.store(h);
}

int Stream::next(bool& have) {
    const uint32_t t = tail_.load();
    if (t == head_.load()) {
        have = false;
        return 0;
    }
    have = true;
    const int v = wide_ ? reinterpret_cast<int16_t*>(ring_)[t % cap_] : (ring_[t % cap_] - 128) * 256;
    tail_.store(t + 1);
    return v;
}

bool Stream::render8(uint8_t* out, int n, int volume) {
    for (int i = 0; i < n; ++i) {
        bool have = false;
        const int v = done_.load() ? 0 : next(have);
        if (!have && end_.load()) done_.store(true);
        const int s = v * volume / 255 / 256;
        out[i] = static_cast<uint8_t>(128 + (s > 127 ? 127 : s < -128 ? -128 : s));
    }
    return !done_.load();
}

bool Stream::render16(int16_t* out, int n, int volume) {
    for (int i = 0; i < n; ++i) {
        bool have = false;
        const int v = done_.load() ? 0 : next(have);
        if (!have && end_.load()) done_.store(true);
        out[i] = static_cast<int16_t>(v * volume / 255);
    }
    return !done_.load();
}

}  // namespace music
