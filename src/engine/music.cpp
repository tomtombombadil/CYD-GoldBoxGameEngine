// The games' music rendered at scan time (music.h). Own code.
#include "music.h"

#include <cstdlib>
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

// ---- Stream -------------------------------------------------------------------

Stream::~Stream() { free(ring_); }

uint32_t Stream::room() const {
    return cap_ - (head_.load() - tail_.load());
}

void Stream::push(const uint8_t* p, uint32_t n) {
    uint32_t h = head_.load();
    for (uint32_t i = 0; i < n; ++i, ++h) ring_[h % cap_] = p[i];
    head_.store(h);
}

bool Stream::render(uint8_t* out, int n, int hz, int volume) {
    if (done_.load()) {
        for (int i = 0; i < n; ++i) out[i] = 128;
        return false;
    }
    const uint32_t step = static_cast<uint32_t>((static_cast<uint64_t>(kRate) << 16) / static_cast<uint32_t>(hz));
    uint32_t t = tail_.load();
    for (int i = 0; i < n; ++i) {
        const uint32_t h = head_.load();
        if (t == h) {                               // nothing read in yet / any more
            if (end_.load()) {
                done_.store(true);
                for (; i < n; ++i) out[i] = 128;
                break;
            }
            out[i] = 128;
            continue;
        }
        // between this sample and the next in a straight line
        const int a = ring_[t % cap_];
        const int b = t + 1 != h ? ring_[(t + 1) % cap_] : a;
        int s = a + (b - a) * static_cast<int>(frac_ & 0xFFFF) / 65536 - 128;
        s = s * volume / 255;
        out[i] = static_cast<uint8_t>(128 + (s > 127 ? 127 : s < -128 ? -128 : s));
        frac_ += step;
        t += frac_ >> 16;
        frac_ &= 0xFFFF;
        if (static_cast<int32_t>(t - h) > 0) t = h;
    }
    tail_.store(t);
    return !done_.load();
}

}  // namespace music
