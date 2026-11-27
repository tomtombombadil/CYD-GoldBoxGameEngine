#include "engine/qimport.h"

#include <cstring>

#include "engine/x86.h"

namespace qimport {
namespace {

// The program window at its own file offsets, a small stack, nothing else
class Bus : public x86::Bus {
public:
    Bus(const uint8_t* w, size_t n, uint32_t lo) : w_(w), n_(n), lo_(lo) {}
    uint8_t read(uint32_t a) override
    {
        if (a >= lo_ && a - lo_ < n_) return w_[a - lo_];
        if (a >= kStack && a < kStack + sizeof stack_) return stack_[a - kStack];
        return 0;
    }
    void write(uint32_t a, uint8_t v) override
    {
        if (a >= kStack && a < kStack + sizeof stack_) stack_[a - kStack] = v;
    }
    static constexpr uint32_t kStack = 0xE0000;
private:
    const uint8_t* w_;
    size_t n_;
    uint32_t lo_;
    uint8_t stack_[256] = {};
};

bool run(x86::Cpu& cpu, uint16_t cs, uint16_t off, int v, uint8_t& out)
{
    cpu.s[x86::SS] = static_cast<uint16_t>(Bus::kStack >> 4);
    cpu.r[x86::SP] = 0xF0;
    cpu.s[x86::DS] = cpu.s[x86::ES] = 0;
    const uint16_t w[1] = {static_cast<uint16_t>(v)};
    if (cpu.far_call(cs, off, w, 1, 2000) != x86::Stop::Returned) return false;
    out = static_cast<uint8_t>(cpu.r[x86::AX]);
    return true;
}

} // namespace

bool make(const uint8_t* window, size_t n, const profile::QImport& q, Maps& m)
{
    if (!q.base || (q.base & 15) || q.hi <= q.lo || n < q.hi - q.lo) return false;
    Bus bus(window, q.hi - q.lo, q.lo);
    x86::Cpu cpu(bus);
    const uint16_t cs = static_cast<uint16_t>(q.base >> 4);
    // (each takes its argument as a C int: a byte the caller widened with CBW - 0x80 up negative)
    for (int v = 0; v < 256; ++v) {
        if (!run(cpu, cs, q.words, v < 128 ? v : 0xFF00 | v, m.word[v])) return false;
        if (!run(cpu, cs, q.effects, v < 128 ? v : 0xFF00 | v, m.effect[v])) return false;
    }
    for (int v = 0; v < 128; ++v)
        if (!run(cpu, cs, q.spells, v, m.spell[v])) return false;
    return true;
}

void convert(party::Character& c, const Maps& m)
{
    for (int k = 0; k < c.n_items; ++k) {
        uint8_t* it = c.items[k];
        // (the Curse-layout item: type 0x2E, words 0x2F-0x31, the Queen's last three bytes 0x3C-0x3E)
        for (int j = 1; j <= 3; ++j) it[0x2E + j] = m.word[it[0x2E + j]];
        const uint8_t t = it[0x2E];
        if (t == 0x27 || t == 0x28 || t == 0x49) {             // (scrolls and their bundle: three spells)
            for (int j = 0; j < 3; ++j) it[0x3C + j] = static_cast<uint8_t>((it[0x3C + j] & 0x80) | m.spell[it[0x3C + j] & 0x7F]);
        } else if (it[0x3E] == 0x80) {                           // (an effect it gives)
            it[0x3D] = m.effect[it[0x3D]];
        } else {                                                 // (the spell it casts)
            it[0x3D] = static_cast<uint8_t>((it[0x3D] & 0x80) | m.spell[it[0x3D] & 0x7F]);
        }
        if (t == 0x34) {
            it[0x2E] = 0x33;
            it[0x3E] = 0;
        }
    }
    for (int k = 0; k < c.n_affects; ++k) c.affects[k][0] = m.effect[c.affects[k][0]];
    // Its spells (DQK.EXE 0x35181, the .WIZ): the 74 memorized places through the spell switch (one being
    // learnt - 0x80 up, negative as the C int it takes - as it is), the places past them empty; the spells
    // known, 1-114, each as the switch's number
    for (int i = 0; i < 74; ++i) {
        uint8_t& v = c.rec[0x1E + i];
        if (v < 0x80) v = m.spell[v];
    }
    for (int i = 74; i < 84; ++i) c.rec[0x1E + i] = 0;
    memset(c.ext + party::kExtMemorized, 0, 137 - 84);
    bool known[115] = {};
    for (int s = 1; s <= 114; ++s) known[s] = s <= 100 ? c.rec[0x79 + s - 1] != 0 : c.ext[party::kExtBook + s - 101] != 0;
    memset(c.rec + 0x79, 0, 100);
    memset(c.ext + party::kExtBook, 0, 17);
    memset(c.ext + party::kExtBook2, 0, 9);
    for (int s = 1; s <= 114; ++s) {
        if (!known[s]) continue;
        const int t = m.spell[s];
        if (t >= 1 && t <= 100) c.rec[0x79 + t - 1] = 1;
        else if (t >= 101 && t <= 117) c.ext[party::kExtBook + t - 101] = 1;
        else if (t >= 118 && t <= 126) c.ext[party::kExtBook2 + t - 118] = 1;
    }
}

}  // namespace qimport
