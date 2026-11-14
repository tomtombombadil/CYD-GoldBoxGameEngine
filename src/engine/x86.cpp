// The 8086 interpreter (x86.h). Own code from the instruction set reference.
#include "x86.h"

namespace x86 {

namespace {
bool parity(uint8_t v) {
    v ^= v >> 4;
    v ^= v >> 2;
    v ^= v >> 1;
    return (v & 1) == 0;
}
}  // namespace

void Cpu::szp8(uint8_t v) {
    set_flag(ZF, v == 0);
    set_flag(SF, (v & 0x80) != 0);
    set_flag(PF, parity(v));
}

void Cpu::szp16(uint16_t v) {
    set_flag(ZF, v == 0);
    set_flag(SF, (v & 0x8000) != 0);
    set_flag(PF, parity(static_cast<uint8_t>(v)));
}

// ---- addressing -------------------------------------------------------------

void Cpu::decode_modrm() {
    modrm_ = fetch8();
    const int mod = modrm_ >> 6, rm = modrm_ & 7;
    if (mod == 3) {
        ea_reg_ = true;
        return;
    }
    ea_reg_ = false;
    uint16_t off = 0;
    int def = DS;
    switch (rm) {
    case 0: off = static_cast<uint16_t>(r[BX] + r[SI]); break;
    case 1: off = static_cast<uint16_t>(r[BX] + r[DI]); break;
    case 2: off = static_cast<uint16_t>(r[BP] + r[SI]); def = SS; break;
    case 3: off = static_cast<uint16_t>(r[BP] + r[DI]); def = SS; break;
    case 4: off = r[SI]; break;
    case 5: off = r[DI]; break;
    case 6:
        if (mod == 0) off = fetch16();
        else { off = r[BP]; def = SS; }
        break;
    case 7: off = r[BX]; break;
    }
    if (mod == 1) off = static_cast<uint16_t>(off + static_cast<int8_t>(fetch8()));
    else if (mod == 2) off = static_cast<uint16_t>(off + fetch16());
    ea_off_ = off;
    ea_seg_ = data_seg(def);
}

uint8_t Cpu::get_rm8() { return ea_reg_ ? get_r8(modrm_ & 7) : rb(ea_seg_, ea_off_); }
uint16_t Cpu::get_rm16() { return ea_reg_ ? r[modrm_ & 7] : rw(ea_seg_, ea_off_); }
void Cpu::set_rm8(uint8_t v) {
    if (ea_reg_) set_r8(modrm_ & 7, v);
    else wb(ea_seg_, ea_off_, v);
}
void Cpu::set_rm16(uint16_t v) {
    if (ea_reg_) r[modrm_ & 7] = v;
    else ww(ea_seg_, ea_off_, v);
}

// ---- arithmetic -------------------------------------------------------------

// Returns the result in the low bits; flags set (CMP's result is dropped by
// the caller).
uint32_t Cpu::alu8(int op, uint8_t a, uint8_t b) {
    uint32_t res = 0;
    const unsigned c = flag(CF) ? 1u : 0u;
    switch (op) {
    case 0:                                         // ADD
    case 2: {                                       // ADC
        const unsigned cin = op == 2 ? c : 0;
        res = a + b + cin;
        set_flag(CF, res > 0xFF);
        set_flag(OF, ((a ^ res) & (b ^ res) & 0x80) != 0);
        set_flag(AF, ((a ^ b ^ res) & 0x10) != 0);
        break;
    }
    case 3:                                         // SBB
    case 5:                                         // SUB
    case 7: {                                       // CMP
        const unsigned cin = op == 3 ? c : 0;
        res = a - b - cin;
        set_flag(CF, a < b + cin);
        set_flag(OF, ((a ^ b) & (a ^ res) & 0x80) != 0);
        set_flag(AF, ((a ^ b ^ res) & 0x10) != 0);
        break;
    }
    case 1: res = a | b; set_flag(CF, false); set_flag(OF, false); set_flag(AF, false); break;
    case 4: res = a & b; set_flag(CF, false); set_flag(OF, false); set_flag(AF, false); break;
    case 6: res = a ^ b; set_flag(CF, false); set_flag(OF, false); set_flag(AF, false); break;
    }
    szp8(static_cast<uint8_t>(res));
    return res & 0xFF;
}

uint32_t Cpu::alu16(int op, uint16_t a, uint16_t b) {
    uint32_t res = 0;
    const uint32_t c = flag(CF) ? 1u : 0u;
    switch (op) {
    case 0:
    case 2: {
        const uint32_t cin = op == 2 ? c : 0;
        res = static_cast<uint32_t>(a) + b + cin;
        set_flag(CF, res > 0xFFFF);
        set_flag(OF, ((a ^ res) & (b ^ res) & 0x8000) != 0);
        set_flag(AF, ((a ^ b ^ res) & 0x10) != 0);
        break;
    }
    case 3:
    case 5:
    case 7: {
        const uint32_t cin = op == 3 ? c : 0;
        res = static_cast<uint32_t>(a) - b - cin;
        set_flag(CF, static_cast<uint32_t>(a) < static_cast<uint32_t>(b) + cin);
        set_flag(OF, ((a ^ b) & (a ^ res) & 0x8000) != 0);
        set_flag(AF, ((a ^ b ^ res) & 0x10) != 0);
        break;
    }
    case 1: res = a | b; set_flag(CF, false); set_flag(OF, false); set_flag(AF, false); break;
    case 4: res = a & b; set_flag(CF, false); set_flag(OF, false); set_flag(AF, false); break;
    case 6: res = a ^ b; set_flag(CF, false); set_flag(OF, false); set_flag(AF, false); break;
    }
    szp16(static_cast<uint16_t>(res));
    return res & 0xFFFF;
}

uint8_t Cpu::inc8(uint8_t v, int d) {                // INC / DEC: CF kept
    const uint8_t res = static_cast<uint8_t>(v + d);
    set_flag(OF, d > 0 ? v == 0x7F : v == 0x80);
    set_flag(AF, ((v ^ res) & 0x10) != 0);
    szp8(res);
    return res;
}

uint16_t Cpu::inc16(uint16_t v, int d) {
    const uint16_t res = static_cast<uint16_t>(v + d);
    set_flag(OF, d > 0 ? v == 0x7FFF : v == 0x8000);
    set_flag(AF, ((v ^ res) & 0x10) != 0);
    szp16(res);
    return res;
}

// ROL ROR RCL RCR SHL SHR SAL SAR (op 0-7); count as the 8086 takes it (the
// 186 masks it to 5 bits - the drivers never shift by more than 15)
uint16_t Cpu::shift(int op, uint16_t v, int count, bool word) {
    count &= 0x1F;
    if (count == 0) return v;
    const int bits = word ? 16 : 8;
    const uint32_t mask = word ? 0xFFFF : 0xFF, top = word ? 0x8000 : 0x80;
    uint32_t x = v & mask;
    bool cf = flag(CF);
    for (int i = 0; i < count; ++i) {
        switch (op) {
        case 0: cf = (x & top) != 0; x = ((x << 1) | (cf ? 1 : 0)) & mask; break;
        case 1: cf = (x & 1) != 0; x = (x >> 1) | (cf ? top : 0); break;
        case 2: { const bool o = (x & top) != 0; x = ((x << 1) | (cf ? 1 : 0)) & mask; cf = o; break; }
        case 3: { const bool o = (x & 1) != 0; x = (x >> 1) | (cf ? top : 0); cf = o; break; }
        case 4:
        case 6: cf = (x & top) != 0; x = (x << 1) & mask; break;
        case 5: cf = (x & 1) != 0; x >>= 1; break;
        case 7: cf = (x & 1) != 0; x = (x >> 1) | (x & top); break;
        }
    }
    set_flag(CF, cf);
    const bool msb = (x & top) != 0;
    switch (op) {
    case 0: case 2: case 4: case 6: set_flag(OF, msb != cf); break;
    case 1: case 3: set_flag(OF, msb != ((x & (top >> 1)) != 0)); break;
    case 5: set_flag(OF, (v & top) != 0 && count == 1); break;
    case 7: set_flag(OF, false); break;
    }
    if (op >= 4) {
        if (word) szp16(static_cast<uint16_t>(x));
        else szp8(static_cast<uint8_t>(x));
    }
    (void)bits;
    return static_cast<uint16_t>(x);
}

bool Cpu::cond(int c) const {
    switch (c & 15) {
    case 0: return flag(OF);
    case 1: return !flag(OF);
    case 2: return flag(CF);
    case 3: return !flag(CF);
    case 4: return flag(ZF);
    case 5: return !flag(ZF);
    case 6: return flag(CF) || flag(ZF);
    case 7: return !flag(CF) && !flag(ZF);
    case 8: return flag(SF);
    case 9: return !flag(SF);
    case 10: return flag(PF);
    case 11: return !flag(PF);
    case 12: return flag(SF) != flag(OF);
    case 13: return flag(SF) == flag(OF);
    case 14: return flag(ZF) || flag(SF) != flag(OF);
    default: return !flag(ZF) && flag(SF) == flag(OF);
    }
}

// F6 / F7: TEST NOT NEG MUL IMUL DIV IDIV
bool Cpu::group3(bool word) {
    const int op = (modrm_ >> 3) & 7;
    if (!word) {
        const uint8_t v = get_rm8();
        switch (op) {
        case 0:
        case 1: alu8(4, v, fetch8()); break;
        case 2: set_rm8(static_cast<uint8_t>(~v)); break;
        case 3: { const uint8_t res = static_cast<uint8_t>(alu8(5, 0, v)); set_flag(CF, v != 0); set_rm8(res); break; }
        case 4: {
            const uint16_t res = static_cast<uint16_t>((r[AX] & 0xFF) * v);
            r[AX] = res;
            set_flag(CF, (res >> 8) != 0); set_flag(OF, (res >> 8) != 0);
            break;
        }
        case 5: {
            const int16_t res = static_cast<int16_t>(static_cast<int8_t>(r[AX]) * static_cast<int8_t>(v));
            r[AX] = static_cast<uint16_t>(res);
            const bool ext = res != static_cast<int8_t>(res);
            set_flag(CF, ext); set_flag(OF, ext);
            break;
        }
        case 6: {
            if (v == 0) return false;
            const uint16_t q = r[AX] / v;
            if (q > 0xFF) return false;
            r[AX] = static_cast<uint16_t>((r[AX] % v) << 8 | q);
            break;
        }
        case 7: {
            if (v == 0) return false;
            const int16_t a = static_cast<int16_t>(r[AX]);
            const int q = a / static_cast<int8_t>(v);
            if (q > 127 || q < -128) return false;
            const int rem = a % static_cast<int8_t>(v);
            r[AX] = static_cast<uint16_t>((rem & 0xFF) << 8 | (q & 0xFF));
            break;
        }
        }
        return true;
    }
    const uint16_t v = get_rm16();
    switch (op) {
    case 0:
    case 1: alu16(4, v, fetch16()); break;
    case 2: set_rm16(static_cast<uint16_t>(~v)); break;
    case 3: { const uint16_t res = static_cast<uint16_t>(alu16(5, 0, v)); set_flag(CF, v != 0); set_rm16(res); break; }
    case 4: {
        const uint32_t res = static_cast<uint32_t>(r[AX]) * v;
        r[AX] = static_cast<uint16_t>(res);
        r[DX] = static_cast<uint16_t>(res >> 16);
        set_flag(CF, r[DX] != 0); set_flag(OF, r[DX] != 0);
        break;
    }
    case 5: {
        const int32_t res = static_cast<int32_t>(static_cast<int16_t>(r[AX])) * static_cast<int16_t>(v);
        r[AX] = static_cast<uint16_t>(res);
        r[DX] = static_cast<uint16_t>(static_cast<uint32_t>(res) >> 16);
        const bool ext = res != static_cast<int16_t>(res);
        set_flag(CF, ext); set_flag(OF, ext);
        break;
    }
    case 6: {
        if (v == 0) return false;
        const uint32_t a = static_cast<uint32_t>(r[DX]) << 16 | r[AX];
        const uint32_t q = a / v;
        if (q > 0xFFFF) return false;
        r[AX] = static_cast<uint16_t>(q);
        r[DX] = static_cast<uint16_t>(a % v);
        break;
    }
    case 7: {
        if (v == 0) return false;
        const int32_t a = static_cast<int32_t>(static_cast<uint32_t>(r[DX]) << 16 | r[AX]);
        const int32_t d = static_cast<int16_t>(v);
        if (a == INT32_MIN && d == -1) return false;
        const int32_t q = a / d;
        if (q > 32767 || q < -32768) return false;
        r[AX] = static_cast<uint16_t>(q);
        r[DX] = static_cast<uint16_t>(a % d);
        break;
    }
    }
    return true;
}

// MOVS CMPS STOS LODS SCAS (A4-AF), with REP / REPE (rep 1) / REPNE (rep 2)
void Cpu::string_op(uint8_t op, int rep) {
    const bool word = (op & 1) != 0;
    const int d = (flag(DF) ? -1 : 1) * (word ? 2 : 1);
    const uint16_t src = data_seg(DS);
    for (;;) {
        if (rep && r[CX] == 0) break;
        switch (op) {
        case 0xA4: wb(s[ES], r[DI], rb(src, r[SI])); r[SI] += d; r[DI] += d; break;
        case 0xA5: ww(s[ES], r[DI], rw(src, r[SI])); r[SI] += d; r[DI] += d; break;
        case 0xA6: alu8(7, rb(src, r[SI]), rb(s[ES], r[DI])); r[SI] += d; r[DI] += d; break;
        case 0xA7: alu16(7, rw(src, r[SI]), rw(s[ES], r[DI])); r[SI] += d; r[DI] += d; break;
        case 0xAA: wb(s[ES], r[DI], static_cast<uint8_t>(r[AX])); r[DI] += d; break;
        case 0xAB: ww(s[ES], r[DI], r[AX]); r[DI] += d; break;
        case 0xAC: set_r8(0, rb(src, r[SI])); r[SI] += d; break;
        case 0xAD: r[AX] = rw(src, r[SI]); r[SI] += d; break;
        case 0xAE: alu8(7, static_cast<uint8_t>(r[AX]), rb(s[ES], r[DI])); r[DI] += d; break;
        case 0xAF: alu16(7, r[AX], rw(s[ES], r[DI])); r[DI] += d; break;
        }
        if (!rep) break;
        --r[CX];
        const bool cmp = op == 0xA6 || op == 0xA7 || op == 0xAE || op == 0xAF;
        if (cmp && rep == 1 && !flag(ZF)) break;
        if (cmp && rep == 2 && flag(ZF)) break;
    }
}

void Cpu::do_int(uint8_t n) {
    if (int_hook && int_hook(*this, n, int_user)) return;
    push(flags);
    push(s[CS]);
    push(ip);
    set_flag(IF, false);
    set_flag(TF, false);
    ip = rw(0, static_cast<uint16_t>(n * 4));
    s[CS] = rw(0, static_cast<uint16_t>(n * 4 + 2));
}

// ---- one instruction --------------------------------------------------------

bool Cpu::step() {
    seg_over_ = -1;
    int rep = 0;
    uint8_t op;
    for (;;) {
        op = fetch8();
        if (op == 0x26) seg_over_ = ES;
        else if (op == 0x2E) seg_over_ = CS;
        else if (op == 0x36) seg_over_ = SS;
        else if (op == 0x3E) seg_over_ = DS;
        else if (op == 0xF3) rep = 1;
        else if (op == 0xF2) rep = 2;
        else if (op == 0xF0) {}
        else break;
    }

    if (op < 0x40 && (op & 7) < 6) {                 // ALU ops in their six forms
        const int alu = op >> 3;
        switch (op & 7) {
        case 0: { decode_modrm(); const uint32_t v = alu8(alu, get_rm8(), get_r8((modrm_ >> 3) & 7)); if (alu != 7) set_rm8(static_cast<uint8_t>(v)); break; }
        case 1: { decode_modrm(); const uint32_t v = alu16(alu, get_rm16(), r[(modrm_ >> 3) & 7]); if (alu != 7) set_rm16(static_cast<uint16_t>(v)); break; }
        case 2: { decode_modrm(); const int g = (modrm_ >> 3) & 7; const uint32_t v = alu8(alu, get_r8(g), get_rm8()); if (alu != 7) set_r8(g, static_cast<uint8_t>(v)); break; }
        case 3: { decode_modrm(); const int g = (modrm_ >> 3) & 7; const uint32_t v = alu16(alu, r[g], get_rm16()); if (alu != 7) r[g] = static_cast<uint16_t>(v); break; }
        case 4: { const uint32_t v = alu8(alu, static_cast<uint8_t>(r[AX]), fetch8()); if (alu != 7) set_r8(0, static_cast<uint8_t>(v)); break; }
        case 5: { const uint32_t v = alu16(alu, r[AX], fetch16()); if (alu != 7) r[AX] = static_cast<uint16_t>(v); break; }
        }
        return true;
    }

    switch (op) {
    case 0x06: push(s[ES]); break;
    case 0x07: s[ES] = pop(); break;
    case 0x0E: push(s[CS]); break;
    case 0x16: push(s[SS]); break;
    case 0x17: s[SS] = pop(); break;
    case 0x1E: push(s[DS]); break;
    case 0x1F: s[DS] = pop(); break;
    case 0x27: {                                     // DAA
        uint8_t al = static_cast<uint8_t>(r[AX]);
        const bool cf = flag(CF);
        const uint8_t old = al;
        if ((al & 0x0F) > 9 || flag(AF)) { al = static_cast<uint8_t>(al + 6); set_flag(AF, true); }
        else set_flag(AF, false);
        if (old > 0x99 || cf) { al = static_cast<uint8_t>(al + 0x60); set_flag(CF, true); }
        else set_flag(CF, false);
        set_r8(0, al); szp8(al);
        break;
    }
    case 0x2F: {                                     // DAS
        uint8_t al = static_cast<uint8_t>(r[AX]);
        const bool cf = flag(CF);
        const uint8_t old = al;
        if ((al & 0x0F) > 9 || flag(AF)) { al = static_cast<uint8_t>(al - 6); set_flag(AF, true); }
        else set_flag(AF, false);
        if (old > 0x99 || cf) { al = static_cast<uint8_t>(al - 0x60); set_flag(CF, true); }
        else set_flag(CF, false);
        set_r8(0, al); szp8(al);
        break;
    }
    case 0x37:                                       // AAA
    case 0x3F: {                                     // AAS
        if ((r[AX] & 0x0F) > 9 || flag(AF)) {
            if (op == 0x37) { r[AX] = static_cast<uint16_t>(r[AX] + 0x106); }
            else { r[AX] = static_cast<uint16_t>(r[AX] - 6); r[AX] = static_cast<uint16_t>(r[AX] - 0x100); }
            set_flag(AF, true); set_flag(CF, true);
        } else {
            set_flag(AF, false); set_flag(CF, false);
        }
        r[AX] &= 0xFF0F;
        break;
    }
    case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
        r[op & 7] = inc16(r[op & 7], 1); break;
    case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
        r[op & 7] = inc16(r[op & 7], -1); break;
    case 0x50: case 0x51: case 0x52: case 0x53: case 0x55: case 0x56: case 0x57:
        push(r[op & 7]); break;
    case 0x54: push(static_cast<uint16_t>(r[SP] - 2)); break;   // (8086: SP after the decrement)
    case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
        r[op & 7] = pop(); break;
    case 0x60: {                                     // PUSHA (186)
        const uint16_t sp = r[SP];
        for (int i = 0; i < 8; ++i) push(i == SP ? sp : r[i]);
        break;
    }
    case 0x61:                                       // POPA
        for (int i = 7; i >= 0; --i) { const uint16_t v = pop(); if (i != SP) r[i] = v; }
        break;
    case 0x68: push(fetch16()); break;               // PUSH imm (186)
    case 0x6A: push(static_cast<uint16_t>(static_cast<int8_t>(fetch8()))); break;
    case 0x69:
    case 0x6B: {                                     // IMUL r, r/m, imm (186)
        decode_modrm();
        const int32_t a = static_cast<int16_t>(get_rm16());
        const int32_t b = op == 0x69 ? static_cast<int16_t>(fetch16()) : static_cast<int8_t>(fetch8());
        const int32_t res = a * b;
        r[(modrm_ >> 3) & 7] = static_cast<uint16_t>(res);
        const bool ext = res != static_cast<int16_t>(res);
        set_flag(CF, ext); set_flag(OF, ext);
        break;
    }
    case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
    case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F: {
        const int8_t d = static_cast<int8_t>(fetch8());
        if (cond(op)) ip = static_cast<uint16_t>(ip + d);
        break;
    }
    case 0x80: case 0x82: {
        decode_modrm();
        const int alu = (modrm_ >> 3) & 7;
        const uint8_t a = get_rm8(), b = fetch8();
        const uint32_t v = alu8(alu, a, b);
        if (alu != 7) set_rm8(static_cast<uint8_t>(v));
        break;
    }
    case 0x81: case 0x83: {
        decode_modrm();
        const int alu = (modrm_ >> 3) & 7;
        const uint16_t a = get_rm16();
        const uint16_t b = op == 0x81 ? fetch16() : static_cast<uint16_t>(static_cast<int8_t>(fetch8()));
        const uint32_t v = alu16(alu, a, b);
        if (alu != 7) set_rm16(static_cast<uint16_t>(v));
        break;
    }
    case 0x84: decode_modrm(); alu8(4, get_rm8(), get_r8((modrm_ >> 3) & 7)); break;
    case 0x85: decode_modrm(); alu16(4, get_rm16(), r[(modrm_ >> 3) & 7]); break;
    case 0x86: { decode_modrm(); const int g = (modrm_ >> 3) & 7; const uint8_t a = get_rm8(); set_rm8(get_r8(g)); set_r8(g, a); break; }
    case 0x87: { decode_modrm(); const int g = (modrm_ >> 3) & 7; const uint16_t a = get_rm16(); set_rm16(r[g]); r[g] = a; break; }
    case 0x88: decode_modrm(); set_rm8(get_r8((modrm_ >> 3) & 7)); break;
    case 0x89: decode_modrm(); set_rm16(r[(modrm_ >> 3) & 7]); break;
    case 0x8A: decode_modrm(); set_r8((modrm_ >> 3) & 7, get_rm8()); break;
    case 0x8B: decode_modrm(); r[(modrm_ >> 3) & 7] = get_rm16(); break;
    case 0x8C: decode_modrm(); set_rm16(s[(modrm_ >> 3) & 3]); break;
    case 0x8D: decode_modrm(); if (ea_reg_) return false; r[(modrm_ >> 3) & 7] = ea_off_; break;
    case 0x8E: decode_modrm(); s[(modrm_ >> 3) & 3] = get_rm16(); break;
    case 0x8F: { decode_modrm(); const uint16_t v = pop(); set_rm16(v); break; }
    case 0x90: break;
    case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: {
        const uint16_t t = r[AX]; r[AX] = r[op & 7]; r[op & 7] = t; break;
    }
    case 0x98: r[AX] = static_cast<uint16_t>(static_cast<int8_t>(r[AX])); break;
    case 0x99: r[DX] = (r[AX] & 0x8000) ? 0xFFFF : 0; break;
    case 0x9A: {
        const uint16_t off = fetch16(), seg = fetch16();
        push(s[CS]); push(ip);
        s[CS] = seg; ip = off;
        break;
    }
    case 0x9B: break;                                // WAIT
    case 0x9C: push(static_cast<uint16_t>(flags | 0xF002)); break;
    case 0x9D: flags = static_cast<uint16_t>((pop() & 0x0FD5) | 0x0002); break;
    case 0x9E: flags = static_cast<uint16_t>((flags & 0xFF00) | ((r[AX] >> 8) & 0xD5) | 0x02); break;
    case 0x9F: r[AX] = static_cast<uint16_t>((r[AX] & 0x00FF) | (flags & 0xFF) << 8); break;
    case 0xA0: set_r8(0, rb(data_seg(DS), fetch16())); break;
    case 0xA1: r[AX] = rw(data_seg(DS), fetch16()); break;
    case 0xA2: wb(data_seg(DS), fetch16(), static_cast<uint8_t>(r[AX])); break;
    case 0xA3: ww(data_seg(DS), fetch16(), r[AX]); break;
    case 0xA4: case 0xA5: case 0xA6: case 0xA7: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF:
        string_op(op, rep); break;
    case 0xA8: alu8(4, static_cast<uint8_t>(r[AX]), fetch8()); break;
    case 0xA9: alu16(4, r[AX], fetch16()); break;
    case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB7:
        set_r8(op & 7, fetch8()); break;
    case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF:
        r[op & 7] = fetch16(); break;
    case 0xC0: case 0xC1: {                          // shifts by imm8 (186)
        decode_modrm();
        const int sop = (modrm_ >> 3) & 7;
        if (op == 0xC0) { const uint8_t v = get_rm8(); const int n = fetch8(); set_rm8(static_cast<uint8_t>(shift(sop, v, n, false))); }
        else { const uint16_t v = get_rm16(); const int n = fetch8(); set_rm16(shift(sop, v, n, true)); }
        break;
    }
    case 0xC2: { const uint16_t n = fetch16(); ip = pop(); r[SP] = static_cast<uint16_t>(r[SP] + n); break; }
    case 0xC3: ip = pop(); break;
    case 0xC4: case 0xC5: {
        decode_modrm();
        if (ea_reg_) return false;
        r[(modrm_ >> 3) & 7] = rw(ea_seg_, ea_off_);
        s[op == 0xC4 ? ES : DS] = rw(ea_seg_, static_cast<uint16_t>(ea_off_ + 2));
        break;
    }
    case 0xC6: decode_modrm(); set_rm8(fetch8()); break;
    case 0xC7: decode_modrm(); set_rm16(fetch16()); break;
    case 0xC8: {                                     // ENTER (186)
        const uint16_t size = fetch16();
        const uint8_t level = fetch8() & 0x1F;
        push(r[BP]);
        const uint16_t frame = r[SP];
        for (int i = 1; i < level; ++i) { r[BP] = static_cast<uint16_t>(r[BP] - 2); push(rw(s[SS], r[BP])); }
        if (level) push(frame);
        r[BP] = frame;
        r[SP] = static_cast<uint16_t>(r[SP] - size);
        break;
    }
    case 0xC9: r[SP] = r[BP]; r[BP] = pop(); break;  // LEAVE
    case 0xCA: { const uint16_t n = fetch16(); ip = pop(); s[CS] = pop(); r[SP] = static_cast<uint16_t>(r[SP] + n); break; }
    case 0xCB: ip = pop(); s[CS] = pop(); break;
    case 0xCC: do_int(3); break;
    case 0xCD: do_int(fetch8()); break;
    case 0xCE: if (flag(OF)) do_int(4); break;
    case 0xCF: ip = pop(); s[CS] = pop(); flags = static_cast<uint16_t>((pop() & 0x0FD5) | 0x0002); break;
    case 0xD0: case 0xD1: case 0xD2: case 0xD3: {
        decode_modrm();
        const int sop = (modrm_ >> 3) & 7;
        const int n = op < 0xD2 ? 1 : (r[CX] & 0xFF);
        if (op & 1) set_rm16(shift(sop, get_rm16(), n, true));
        else set_rm8(static_cast<uint8_t>(shift(sop, get_rm8(), n, false)));
        break;
    }
    case 0xD4: {                                     // AAM
        const uint8_t b = fetch8();
        if (b == 0) return false;
        const uint8_t al = static_cast<uint8_t>(r[AX]);
        r[AX] = static_cast<uint16_t>((al / b) << 8 | (al % b));
        szp8(static_cast<uint8_t>(r[AX]));
        break;
    }
    case 0xD5: {                                     // AAD
        const uint8_t b = fetch8();
        const uint8_t al = static_cast<uint8_t>((r[AX] & 0xFF) + (r[AX] >> 8) * b);
        r[AX] = al;
        szp8(al);
        break;
    }
    case 0xD7: set_r8(0, rb(data_seg(DS), static_cast<uint16_t>(r[BX] + (r[AX] & 0xFF)))); break;
    case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDE: case 0xDF:
        decode_modrm(); break;                       // ESC (no coprocessor)
    case 0xE0: case 0xE1: case 0xE2: {
        const int8_t d = static_cast<int8_t>(fetch8());
        --r[CX];
        const bool go = r[CX] != 0 && (op == 0xE2 || (op == 0xE1 ? flag(ZF) : !flag(ZF)));
        if (go) ip = static_cast<uint16_t>(ip + d);
        break;
    }
    case 0xE3: { const int8_t d = static_cast<int8_t>(fetch8()); if (r[CX] == 0) ip = static_cast<uint16_t>(ip + d); break; }
    case 0xE4: set_r8(0, bus_.in(fetch8())); break;
    case 0xE5: { const uint8_t p = fetch8(); r[AX] = static_cast<uint16_t>(bus_.in(p) | bus_.in(static_cast<uint16_t>(p + 1)) << 8); break; }
    case 0xE6: bus_.out(fetch8(), static_cast<uint8_t>(r[AX])); break;
    case 0xE7: { const uint8_t p = fetch8(); bus_.out(p, static_cast<uint8_t>(r[AX])); bus_.out(static_cast<uint16_t>(p + 1), static_cast<uint8_t>(r[AX] >> 8)); break; }
    case 0xE8: { const uint16_t d = fetch16(); push(ip); ip = static_cast<uint16_t>(ip + d); break; }
    case 0xE9: { const uint16_t d = fetch16(); ip = static_cast<uint16_t>(ip + d); break; }
    case 0xEA: { const uint16_t off = fetch16(), seg = fetch16(); s[CS] = seg; ip = off; break; }
    case 0xEB: { const int8_t d = static_cast<int8_t>(fetch8()); ip = static_cast<uint16_t>(ip + d); break; }
    case 0xEC: set_r8(0, bus_.in(r[DX])); break;
    case 0xED: r[AX] = static_cast<uint16_t>(bus_.in(r[DX]) | bus_.in(static_cast<uint16_t>(r[DX] + 1)) << 8); break;
    case 0xEE: bus_.out(r[DX], static_cast<uint8_t>(r[AX])); break;
    case 0xEF: bus_.out(r[DX], static_cast<uint8_t>(r[AX])); bus_.out(static_cast<uint16_t>(r[DX] + 1), static_cast<uint8_t>(r[AX] >> 8)); break;
    case 0xF4: return false;                         // HLT (run() reports it)
    case 0xF5: set_flag(CF, !flag(CF)); break;
    case 0xF6: decode_modrm(); return group3(false);
    case 0xF7: decode_modrm(); return group3(true);
    case 0xF8: set_flag(CF, false); break;
    case 0xF9: set_flag(CF, true); break;
    case 0xFA: set_flag(IF, false); break;
    case 0xFB: set_flag(IF, true); break;
    case 0xFC: set_flag(DF, false); break;
    case 0xFD: set_flag(DF, true); break;
    case 0xFE: {
        decode_modrm();
        const int g = (modrm_ >> 3) & 7;
        if (g > 1) return false;
        set_rm8(inc8(get_rm8(), g == 0 ? 1 : -1));
        break;
    }
    case 0xFF: {
        decode_modrm();
        const int g = (modrm_ >> 3) & 7;
        switch (g) {
        case 0: set_rm16(inc16(get_rm16(), 1)); break;
        case 1: set_rm16(inc16(get_rm16(), -1)); break;
        case 2: { const uint16_t t = get_rm16(); push(ip); ip = t; break; }
        case 3: {
            if (ea_reg_) return false;
            const uint16_t off = rw(ea_seg_, ea_off_), seg = rw(ea_seg_, static_cast<uint16_t>(ea_off_ + 2));
            push(s[CS]); push(ip);
            s[CS] = seg; ip = off;
            break;
        }
        case 4: ip = get_rm16(); break;
        case 5: {
            if (ea_reg_) return false;
            const uint16_t off = rw(ea_seg_, ea_off_), seg = rw(ea_seg_, static_cast<uint16_t>(ea_off_ + 2));
            s[CS] = seg; ip = off;
            break;
        }
        case 6: push(get_rm16()); break;
        default: return false;
        }
        break;
    }
    default:
        return false;
    }
    return true;
}

Stop Cpu::run(uint16_t stop_seg, uint16_t stop_off, uint32_t max_steps) {
    for (uint32_t n = 0; n < max_steps; ++n) {
        if (s[CS] == stop_seg && ip == stop_off) return Stop::Returned;
        const uint16_t at_cs = s[CS], at_ip = ip;
        ++steps_;
        if (!step()) {
            if (rb(at_cs, at_ip) == 0xF4) return Stop::Halted;
            s[CS] = at_cs;
            ip = at_ip;
            return Stop::Bad;
        }
    }
    return Stop::Limit;
}

Stop Cpu::far_call(uint16_t seg, uint16_t off, const uint16_t* words, int n, uint32_t max_steps) {
    const uint16_t sp = r[SP];
    for (int i = 0; i < n; ++i) push(words[i]);
    push(kRetSeg);
    push(kRetOff);
    s[CS] = seg;
    ip = off;
    const Stop st = run(kRetSeg, kRetOff, max_steps);
    if (st == Stop::Returned) r[SP] = sp;
    return st;
}

Stop Cpu::interrupt(uint16_t seg, uint16_t off, uint32_t max_steps) {
    const uint16_t sp = r[SP];
    push(flags);
    push(kRetSeg);
    push(kRetOff);
    set_flag(IF, false);
    s[CS] = seg;
    ip = off;
    const Stop st = run(kRetSeg, kRetOff, max_steps);
    if (st == Stop::Returned) r[SP] = sp;
    return st;
}

}  // namespace x86
