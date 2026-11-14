// The music renderer: the x86 interpreter (src/engine/x86.*), the MicroMagic
// runner (music.*) on a synthetic driver written here in machine code, and the
// OPL2 (opl.*). Nothing from the games. CI builds it with
//   g++ -std=c++17 -O1 -Wall -Werror -fsanitize=address,undefined -Isrc
//       -o test_music tools/host_tests/test_music.cpp src/engine/*.cpp
#include "engine/dax.h"
#include "engine/music.h"
#include "engine/opl.h"
#include "engine/x86.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)

using Bytes = std::vector<uint8_t>;

// A flat megabyte with a port log
struct Flat : x86::Bus {
    std::vector<uint8_t> m = std::vector<uint8_t>(1 << 20);
    std::vector<std::pair<uint16_t, uint8_t>> outs;
    uint8_t read(uint32_t a) override { return m[a & 0xFFFFF]; }
    void write(uint32_t a, uint8_t v) override { m[a & 0xFFFFF] = v; }
    uint8_t in(uint16_t p) override { return static_cast<uint8_t>(p); }
    void out(uint16_t p, uint8_t v) override { outs.push_back({p, v}); }
};

// Runs `code` at 1000:0000 until its HLT
static x86::Stop run_code(Flat& bus, x86::Cpu& cpu, const Bytes& code) {
    for (size_t i = 0; i < code.size(); ++i) bus.m[0x10000 + i] = code[i];
    cpu.s[x86::CS] = 0x1000;
    cpu.ip = 0;
    cpu.s[x86::SS] = 0x2000;
    cpu.r[x86::SP] = 0xFFFE;
    cpu.s[x86::DS] = cpu.s[x86::ES] = 0x3000;
    return cpu.run(0xFFFF, 0xFFFF, 100000);
}

static void test_arithmetic() {
    Flat bus;
    x86::Cpu cpu(bus);
    // mov ax,0x7FFF; add ax,1 (OF SF, no CF); mov bx,ax; mov cx,0x10; sub cx,0x11 (CF SF);
    // mov dx,3; mov si,7; imul si -> dx:ax = 3*7 = 21? (imul uses ax: ax=0x8000) ...
    const Bytes code = {
        0xB8, 0xFF, 0x7F,             // mov ax,7FFF
        0x05, 0x01, 0x00,             // add ax,1
        0x9C,                         // pushf
        0x5B,                         // pop bx  (flags)
        0xB9, 0x10, 0x00,             // mov cx,10
        0x83, 0xE9, 0x11,             // sub cx,11
        0x9C, 0x5A,                   // pushf; pop dx
        0xF4,
    };
    CHECK(run_code(bus, cpu, code) == x86::Stop::Halted);
    CHECK(cpu.r[x86::AX] == 0x8000);
    CHECK((cpu.r[x86::BX] & x86::OF) && (cpu.r[x86::BX] & x86::SF) && !(cpu.r[x86::BX] & x86::CF));
    CHECK((cpu.r[x86::BX] & x86::AF));
    CHECK(cpu.r[x86::CX] == 0xFFFF);
    CHECK((cpu.r[x86::DX] & x86::CF) && (cpu.r[x86::DX] & x86::SF) && !(cpu.r[x86::DX] & x86::OF));
}

static void test_mul_div_shift() {
    Flat bus;
    x86::Cpu cpu(bus);
    const Bytes code = {
        0xB8, 0x34, 0x12,             // mov ax,1234
        0xBB, 0x00, 0x01,             // mov bx,100
        0xF7, 0xE3,                   // mul bx        -> dx:ax = 0x00123400
        0x89, 0xC6,                   // mov si,ax
        0x89, 0xD7,                   // mov di,dx
        0xB9, 0x10, 0x00,             // mov cx,10
        0xF7, 0xF1,                   // div cx        -> ax = 0x12340, too big? (dx:ax = 0x123400 / 16 = 0x12340: overflow)
        0xF4,
    };
    // 0x123400 / 16 = 0x12340 > 0xFFFF: a divide error stops it (Bad)
    CHECK(run_code(bus, cpu, code) == x86::Stop::Bad);
    CHECK(cpu.r[x86::SI] == 0x3400 && cpu.r[x86::DI] == 0x0012);

    x86::Cpu c2(bus);
    const Bytes code2 = {
        0xB8, 0x00, 0x34,             // mov ax,3400
        0xBA, 0x12, 0x00,             // mov dx,0012
        0xB9, 0x00, 0x01,             // mov cx,100
        0xF7, 0xF1,                   // div cx  -> ax = 0x1234, dx = 0
        0xB3, 0x81,                   // mov bl,81
        0xD0, 0xE3,                   // shl bl,1 -> 02, CF=1, OF=1 (msb 0 != CF 1)
        0x9C, 0x5E,                   // pushf; pop si
        0xB7, 0xF0,                   // mov bh,F0
        0xB1, 0x04,                   // mov cl,4
        0xD2, 0xFF,                   // sar bh,cl -> FF
        0xF4,
    };
    CHECK(run_code(bus, c2, code2) == x86::Stop::Halted);
    CHECK(c2.r[x86::AX] == 0x1234 && c2.r[x86::DX] == 0);
    CHECK((c2.r[x86::BX] & 0xFF) == 0x02);
    CHECK((c2.r[x86::SI] & x86::CF) && (c2.r[x86::SI] & x86::OF));
    CHECK((c2.r[x86::BX] >> 8) == 0xFF);

    // signed: idiv -7 / 2 = -3 rem -1
    x86::Cpu c3(bus);
    const Bytes code3 = {
        0xB8, 0xF9, 0xFF,             // mov ax,-7
        0x99,                         // cwd
        0xBB, 0x02, 0x00,             // mov bx,2
        0xF7, 0xFB,                   // idiv bx
        0xF4,
    };
    CHECK(run_code(bus, c3, code3) == x86::Stop::Halted);
    CHECK(c3.r[x86::AX] == 0xFFFD && c3.r[x86::DX] == 0xFFFF);
}

static void test_memory_strings_calls() {
    Flat bus;
    x86::Cpu cpu(bus);
    // copy 5 bytes with rep movsb (DS:0 -> ES:10), call a near routine that
    // adds [bp+4] to ax, a far call through memory, a loop
    const Bytes code = {
        0xBE, 0x00, 0x00,             // 00 mov si,0
        0xBF, 0x10, 0x00,             // 03 mov di,10
        0xB9, 0x05, 0x00,             // 06 mov cx,5
        0xFC,                         // 09 cld
        0xF3, 0xA4,                   // 0A rep movsb
        0x31, 0xC0,                   // 0C xor ax,ax
        0x6A, 0x07,                   // 0E push 7     (186)
        0xE8, 0x0B, 0x00,             // 10 call 1E
        0x83, 0xC4, 0x02,             // 13 add sp,2
        0xB9, 0x04, 0x00,             // 16 mov cx,4
        0x40,                         // 19 inc ax
        0xE2, 0xFD,                   // 1A loop 19
        0xF4,                         // 1C hlt
        0x90,                         // 1D
        0x55,                         // 1E push bp
        0x8B, 0xEC,                   // 1F mov bp,sp
        0x03, 0x46, 0x04,             // 21 add ax,[bp+4]
        0x5D,                         // 24 pop bp
        0xC3,                         // 25 ret
    };
    for (int i = 0; i < 5; ++i) bus.m[0x30000 + i] = static_cast<uint8_t>('a' + i);
    CHECK(run_code(bus, cpu, code) == x86::Stop::Halted);
    CHECK(memcmp(&bus.m[0x30010], "abcde", 5) == 0);
    CHECK(cpu.r[x86::AX] == 11);
    CHECK(cpu.r[x86::CX] == 0);
    CHECK(cpu.r[x86::SP] == 0xFFFE);

    // far_call + interrupt: a routine that returns ax = first arg * 2 (retf),
    // and an interrupt handler that bumps a byte (iret)
    const Bytes fn = {0x55, 0x8B, 0xEC, 0x8B, 0x46, 0x06, 0x01, 0xC0, 0x5D, 0xCB,    // 4000:0000
                      0x2E, 0xFE, 0x06, 0x20, 0x00, 0xCF};                              // 4000:000A
    for (size_t i = 0; i < fn.size(); ++i) bus.m[0x40000 + i] = fn[i];
    const uint16_t args[2] = {99, 21};
    CHECK(cpu.far_call(0x4000, 0, args, 2, 1000) == x86::Stop::Returned);
    CHECK(cpu.r[x86::AX] == 42);
    CHECK(cpu.r[x86::SP] == 0xFFFE);
    CHECK(cpu.interrupt(0x4000, 0x0A, 1000) == x86::Stop::Returned);
    CHECK(cpu.interrupt(0x4000, 0x0A, 1000) == x86::Stop::Returned);
    CHECK(bus.m[0x40020] == 2);

    // ports
    x86::Cpu c2(bus);
    const Bytes io = {0xBA, 0x88, 0x03, 0xB0, 0x20, 0xEE, 0x42, 0xB0, 0x31, 0xEE, 0xE4, 0x61, 0xF4};
    CHECK(run_code(bus, c2, io) == x86::Stop::Halted);
    CHECK(bus.outs.size() == 2 && bus.outs[0].first == 0x388 && bus.outs[0].second == 0x20 &&
          bus.outs[1].first == 0x389 && bus.outs[1].second == 0x31);
    CHECK((c2.r[x86::AX] & 0xFF) == 0x61);

    // an opcode it doesn't know stops it where it is
    x86::Cpu c3(bus);
    const Bytes bad = {0x90, 0x0F, 0x0B};
    CHECK(run_code(bus, c3, bad) == x86::Stop::Bad);
    CHECK(c3.ip == 1);
}

// ---- the MicroMagic protocol on a synthetic driver ---------------------------

static void put16(Bytes& b, uint32_t v) { b.push_back(v & 0xFF); b.push_back((v >> 8) & 0xFF); }
static void put32(Bytes& b, uint32_t v) { put16(b, v & 0xFFFF); put16(b, v >> 16); }

static Bytes make_dax(const std::vector<std::pair<uint8_t, Bytes>>& blocks) {
    Bytes index, data;
    for (const auto& [id, raw] : blocks) {
        Bytes comp(raw.size() + raw.size() / 128 + 2);
        comp.resize(dax::compress(raw.data(), raw.size(), comp.data()));
        index.push_back(id);
        put32(index, static_cast<uint32_t>(data.size()));
        put16(index, static_cast<uint32_t>(raw.size()));
        put16(index, static_cast<uint32_t>(comp.size()));
        data.insert(data.end(), comp.begin(), comp.end());
    }
    Bytes file;
    put16(file, static_cast<uint32_t>(index.size()));
    file.insert(file.end(), index.begin(), index.end());
    file.insert(file.end(), data.begin(), data.end());
    return file;
}

// A tiny driver in the MicroMagic shape (offsets as loaded, from 0x100):
// cmd 0 hooks INT 1Ch at 0x150 and sets the PIT to 0x4000; cmd 2 returns the
// notes left (word 0x200); the handler bumps its clock (word 0x202) and, while
// notes are left, writes register B0 = notes left, one a tick.
static Bytes synthetic_driver() {
    Bytes d(0x400 - 0x100 + 0x100, 0x90);
    auto at = [&](int off, std::initializer_list<int> bytes) {
        int i = off - 0x100;
        for (int b : bytes) d[i++] = static_cast<uint8_t>(b);
    };
    at(0x100, {0x55, 0x8B, 0xEC, 0x8B, 0x46, 0x06, 0x3D, 0x00, 0x00, 0x74, 0x09, 0x3D, 0x02, 0x00, 0x74, 0x2B,
               0x31, 0xC0, 0x5D, 0xCB});
    at(0x114, {0x31, 0xC0, 0x1E, 0x8E, 0xD8, 0xC7, 0x06, 0x70, 0x00, 0x50, 0x01, 0x8C, 0x0E, 0x72, 0x00, 0x1F,
               0xB0, 0x36, 0xE6, 0x43, 0xB0, 0x00, 0xE6, 0x40, 0xB0, 0x40, 0xE6, 0x40,
               0x2E, 0xC7, 0x06, 0x00, 0x02, 0x03, 0x00, 0x31, 0xC0, 0x5D, 0xCB});
    at(0x13B, {0x2E, 0xA1, 0x00, 0x02, 0x5D, 0xCB});
    at(0x150, {0x2E, 0xFF, 0x06, 0x02, 0x02, 0x50, 0x52, 0x2E, 0x83, 0x3E, 0x00, 0x02, 0x00, 0x74, 0x11,
               0xBA, 0x88, 0x03, 0xB0, 0xB0, 0xEE, 0x42, 0x2E, 0xA0, 0x00, 0x02, 0xEE,
               0x2E, 0xFF, 0x0E, 0x00, 0x02, 0x5A, 0x58, 0xCF});
    return d;
}

struct Log : music::Sink {
    std::vector<std::pair<uint8_t, uint8_t>> w;
    double t = 0;
    void opl(uint8_t r, uint8_t v) override { w.push_back({r, v}); }
    void advance(double s) override { t += s; }
};

struct Mem : dax::ByteSource {
    Bytes d;
    size_t read_at(uint32_t pos, uint8_t* buf, size_t n) override {
        if (pos >= d.size()) return 0;
        if (pos + n > d.size()) n = d.size() - pos;
        memcpy(buf, d.data() + pos, n);
        return n;
    }
    uint32_t size() const override { return static_cast<uint32_t>(d.size()); }
};

static void test_micromagic() {
    Bytes song = {'G', 'L', 'B', 'R'};
    song.resize(32, 0);
    Mem m;
    m.d = make_dax({{0, synthetic_driver()}, {1, song}, {3, song}});
    CHECK(music::micromagic_songs(m) == ((1u << 1) | (1u << 3)));
    Log log;
    double secs = 0;
    CHECK(music::render_micromagic(m, 1, log, 60, 0.5, &secs) == music::Result::Ok);
    CHECK(log.w.size() == 3);
    if (log.w.size() == 3) CHECK(log.w[0].first == 0xB0 && log.w[0].second == 3 && log.w[2].second == 1);
    // three ticks at 1193182 / 0x4000 Hz, then the tail
    const double tick = 16384.0 / 1193182.0;
    CHECK(secs > 3 * tick + 0.49 && secs < 3 * tick + 0.51);
    CHECK(log.t > 3 * tick + 0.49 && log.t < 3 * tick + 0.51);
    Log l2;
    CHECK(music::render_micromagic(m, 2, l2) == music::Result::NoSong);
    Mem none;
    none.d = make_dax({{1, song}});
    CHECK(music::render_micromagic(none, 1, l2) == music::Result::NoDriver);
}

static void test_opl() {
    std::vector<int16_t> out;
    music::Opl2Sink chip(music::kRate, [](const int16_t* s, uint32_t n, void* u) {
        auto* v = static_cast<std::vector<int16_t>*>(u);
        v->insert(v->end(), s, s + n);
    }, &out);
    CHECK(chip.ok());
    chip.advance(0.1);                       // silence
    // a plain sine voice on channel 0: carrier full volume, A4-ish, key on
    const uint8_t regs[][2] = {{0x20, 0x01}, {0x23, 0x01}, {0x40, 0x3F}, {0x43, 0x00}, {0x60, 0xF0}, {0x63, 0xF0},
                               {0x80, 0x00}, {0x83, 0x00}, {0xC0, 0x01}, {0xA0, 0x44}, {0xB0, 0x32}};
    for (auto& r : regs) chip.opl(r[0], r[1]);
    chip.advance(0.2);
    chip.flush();
    CHECK(out.size() >= 6610 && out.size() <= 6620);   // 0.3 s at 22,050
    int quiet_lo = 32767, quiet_hi = -32768, lo = 32767, hi = -32768;
    for (size_t i = 0; i < 2000 && i < out.size(); ++i) { quiet_lo = std::min<int>(quiet_lo, out[i]); quiet_hi = std::max<int>(quiet_hi, out[i]); }
    for (size_t i = 3000; i < out.size(); ++i) { lo = std::min<int>(lo, out[i]); hi = std::max<int>(hi, out[i]); }
    CHECK(quiet_hi - quiet_lo <= 2);
    CHECK(hi - lo > 10000);
    CHECK(chip.last_sound() > 6000);
}

// The ring: 16-bit kept as it is, 8-bit rounded down to a byte; volume;
// silence while empty, then over once ended
static void test_stream() {
    for (int wide = 0; wide < 2; ++wide) {
        music::Stream s(static_cast<uint8_t*>(malloc(64)), 64, wide != 0);
        CHECK(s.room() == (wide ? 32u : 64u));
        const int16_t in[4] = {0, 1000, -1000, 32767};
        s.push(in, 4);
        CHECK(s.room() == (wide ? 28u : 60u));
        int16_t o16[6];
        uint8_t o8[6];
        bool more = wide ? s.render16(o16, 6, 255) : s.render8(o8, 6, 255);
        CHECK(more);
        if (wide) {
            CHECK(o16[0] == 0 && o16[1] == 1000 && o16[2] == -1000 && o16[3] == 32767 && o16[4] == 0);
        } else {
            CHECK(o8[0] == 128 && o8[1] == 131 && o8[2] == 124 && o8[3] == 255 && o8[4] == 128);
        }
        s.push(in, 2);
        s.end();
        more = wide ? s.render16(o16, 6, 128) : s.render8(o8, 6, 128);
        CHECK(!more && s.done());
        if (wide) CHECK(o16[1] == 1000 * 128 / 255 && o16[2] == 0);
    }
    uint8_t h[44] = {};
    memcpy(h, "RIFF", 4); memcpy(h + 8, "WAVEfmt ", 8); h[20] = 1; h[22] = 1;
    h[24] = music::kRate & 0xFF; h[25] = music::kRate >> 8; h[34] = 16; memcpy(h + 36, "data", 4); h[40] = 0x10;
    CHECK(music::wav_ours(h) && music::wav_data(h) == 16);
    h[34] = 8;
    CHECK(!music::wav_ours(h));
}

int main() {
    test_arithmetic();
    test_mul_div_shift();
    test_memory_strings_calls();
    test_micromagic();
    test_opl();
    test_stream();
    if (failures) { printf("%d failures\n", failures); return 1; }
    printf("test_music: all passed\n");
    return 0;
}
