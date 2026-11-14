// A small 8086 (+ the 80186's few extra opcodes) interpreter: runs a music
// driver from the PLAYER'S OWN game files at card-scan time, to render the
// games' AdLib music to sound files (Tom, 2026-11-14 - the music is turned
// into WAV files during the scan and played from the card; the original
// drivers do the music exactly as they did in DOS, the OPL2 chip writes they
// make go to an emulated chip). Nothing of the games is in this code: the
// driver's bytes are read from the card, run, and thrown away.
//
// Own code, written from the Intel 8086 / 80186 instruction set reference.
// Plain C++ (no Arduino), host-tested in tools/host_tests/test_x86.cpp and
// checked against Unicorn on the real drivers (tools/devkit/music).
//
// Memory and ports go through a Bus, so a board can give the driver a few
// small windows of RAM (its code and data, the song, a stack) instead of a
// megabyte: addresses outside them read 0 and drop writes.
#pragma once

#include <cstdint>

namespace x86 {

class Bus {
public:
    virtual ~Bus() = default;
    virtual uint8_t read(uint32_t linear) = 0;
    virtual void write(uint32_t linear, uint8_t v) = 0;
    virtual uint8_t in(uint16_t port) { (void)port; return 0xFF; }
    virtual void out(uint16_t port, uint8_t v) { (void)port; (void)v; }
};

enum Reg16 { AX, CX, DX, BX, SP, BP, SI, DI };
enum Seg { ES, CS, SS, DS };

enum Flag : uint16_t {
    CF = 1 << 0, PF = 1 << 2, AF = 1 << 4, ZF = 1 << 6, SF = 1 << 7,
    TF = 1 << 8, IF = 1 << 9, DF = 1 << 10, OF = 1 << 11,
};

class Cpu;

// A software interrupt (INT n) the caller handles itself (BIOS / DOS
// services a driver asks for); return false to run the vector in memory.
using IntHook = bool (*)(Cpu& cpu, uint8_t n, void* user);

enum class Stop : uint8_t {
    Returned,     // reached the stop address (a call's return)
    Halted,       // HLT
    Limit,        // ran the step budget out
    Bad,          // an opcode this interpreter doesn't know, or a divide error
};

class Cpu {
public:
    explicit Cpu(Bus& bus) : bus_(bus) {}

    uint16_t r[8] = {};          // AX CX DX BX SP BP SI DI
    uint16_t s[4] = {};          // ES CS SS DS
    uint16_t ip = 0;
    uint16_t flags = 0x0002;

    IntHook int_hook = nullptr;
    void*   int_user = nullptr;

    // Runs from CS:IP until CS:IP == stop_seg:stop_off, HLT, a bad opcode
    // or max_steps instructions.
    Stop run(uint16_t stop_seg, uint16_t stop_off, uint32_t max_steps);

    // Far-calls seg:off with the words given pushed first to last (so the
    // last is nearest the return address - a C function's first argument),
    // the return address being a sentinel; runs until it comes back. The
    // stack pointer is put back as before (a C caller pops the arguments).
    Stop far_call(uint16_t seg, uint16_t off, const uint16_t* words, int n, uint32_t max_steps);
    // As an interrupt: FLAGS, CS, IP pushed, until its IRET.
    Stop interrupt(uint16_t seg, uint16_t off, uint32_t max_steps);

    uint8_t  rb(uint16_t seg, uint16_t off) { return bus_.read(lin(seg, off)); }
    uint16_t rw(uint16_t seg, uint16_t off) {
        return static_cast<uint16_t>(rb(seg, off) | rb(seg, static_cast<uint16_t>(off + 1)) << 8);
    }
    void wb(uint16_t seg, uint16_t off, uint8_t v) { bus_.write(lin(seg, off), v); }
    void ww(uint16_t seg, uint16_t off, uint16_t v) {
        wb(seg, off, static_cast<uint8_t>(v));
        wb(seg, static_cast<uint16_t>(off + 1), static_cast<uint8_t>(v >> 8));
    }
    void push(uint16_t v) { r[SP] = static_cast<uint16_t>(r[SP] - 2); ww(s[SS], r[SP], v); }
    uint16_t pop() { const uint16_t v = rw(s[SS], r[SP]); r[SP] = static_cast<uint16_t>(r[SP] + 2); return v; }

    static uint32_t lin(uint16_t seg, uint16_t off) { return ((static_cast<uint32_t>(seg) << 4) + off) & 0xFFFFF; }

    uint32_t steps() const { return steps_; }

    // The sentinel return address far_call / interrupt use (a HLT is never
    // needed there: run() stops when CS:IP reaches it).
    static constexpr uint16_t kRetSeg = 0xF000, kRetOff = 0xFFF0;

private:
    Bus& bus_;
    uint32_t steps_ = 0;

    // decoding state of the instruction in hand
    int      seg_over_ = -1;
    uint8_t  modrm_ = 0;
    uint16_t ea_off_ = 0;
    uint16_t ea_seg_ = 0;
    bool     ea_reg_ = false;

    bool step();                       // false: a bad opcode / divide error

    uint8_t  fetch8() { const uint8_t v = rb(s[CS], ip); ip = static_cast<uint16_t>(ip + 1); return v; }
    uint16_t fetch16() { const uint16_t v = rw(s[CS], ip); ip = static_cast<uint16_t>(ip + 2); return v; }

    void     decode_modrm();
    uint8_t  get_rm8();
    uint16_t get_rm16();
    void     set_rm8(uint8_t v);
    void     set_rm16(uint16_t v);
    uint8_t  get_r8(int i) const { return i < 4 ? static_cast<uint8_t>(r[i]) : static_cast<uint8_t>(r[i - 4] >> 8); }
    void     set_r8(int i, uint8_t v) {
        if (i < 4) r[i] = static_cast<uint16_t>((r[i] & 0xFF00) | v);
        else r[i - 4] = static_cast<uint16_t>((r[i - 4] & 0x00FF) | v << 8);
    }
    uint16_t data_seg(int def) const { return s[seg_over_ >= 0 ? seg_over_ : def]; }

    void set_flag(uint16_t f, bool on) { flags = on ? (flags | f) : (flags & ~f); }
    bool flag(uint16_t f) const { return (flags & f) != 0; }
    void szp8(uint8_t v);
    void szp16(uint16_t v);

    uint32_t alu8(int op, uint8_t a, uint8_t b);     // op 0-7: add or adc sbb and sub xor cmp
    uint32_t alu16(int op, uint16_t a, uint16_t b);
    uint8_t  inc8(uint8_t v, int d);
    uint16_t inc16(uint16_t v, int d);
    uint16_t shift(int op, uint16_t v, int count, bool word);
    bool     group3(bool word);
    bool     cond(int c) const;
    void     string_op(uint8_t op, int rep);
    void     do_int(uint8_t n);
};

}  // namespace x86
