// VACE movies (vace.h)
#include "vace.h"

#include <cstring>

namespace vace {

namespace {

// The music driver's timer (PIT divisor 0x4000) and the BIOS's (0x10000),
// in microseconds a tick; a retrace in mode 13h / 0Dh (70.086 Hz)
constexpr uint32_t kTickSound = 13731, kTickPlain = 54925, kRetrace = 14268;   // (us)
// The EGA colours in 6-bit values (the hardware's standard 16)
constexpr uint8_t kEga6[16][3] = {{0, 0, 0},    {0, 0, 42},   {0, 42, 0},   {0, 42, 42},  {42, 0, 0},   {42, 0, 42},
                                  {42, 21, 0},  {42, 42, 42}, {21, 21, 21}, {21, 21, 63}, {21, 63, 21}, {21, 63, 63},
                                  {63, 21, 21}, {63, 21, 63}, {63, 63, 21}, {63, 63, 63}};

uint32_t rd32(dax::ByteSource& src, uint32_t at, bool* ok)
{
    uint8_t b[4] = {};
    if (src.read_at(at, b, 4) != 4) *ok = false;
    return b[0] | b[1] << 8 | b[2] << 16 | static_cast<uint32_t>(b[3]) << 24;
}

uint32_t tick_us(const Player& p) { return p.sound ? kTickSound : kTickPlain; }

// A deadline met at `now`: the time the frame counts from (the original
// reads the tick when it notices - just past the deadline, or much later
// after a fade's work)
uint32_t met(const Player& p, uint32_t deadline, uint32_t now)
{
    return now - deadline < tick_us(p) + 1000 ? deadline : now;
}

// A time in us (from the movie's start) as the caller's ms (rounded up: not before it's due)
uint32_t g_epoch = 0;
uint32_t ms_of(uint32_t us) { return g_epoch + (us + 999) / 1000; }

void clear(Screen& s)
{
    for (int y = 0; y < kH; ++y)
        for (int x = 0; x < kW; ++x) s.put(x, y, 0);
}

// A signed byte's square (EGA.DRV's weights: a difference that wrapped
// below 0 counts as its negative)
int sq(int v)
{
    v &= 0xFF;
    if (v > 128) v = 256 - v;
    return v * v;
}

void fade_start(Player& p, Screen& s, uint32_t at)
{
    p.fade_at = at;
    p.step = 0;
    if (!p.vga) {
        p.fading = 2;
        p.lfsr = 0;                             // (0, 0 first, then the register from 1)
        p.left = static_cast<uint32_t>(kW) * kH;
        p.batch = 0;
        return;
    }
    p.fading = 1;
    bool on[256] = {};
    for (int y = 0; y < kH; ++y)
        for (int x = 0; x < kW; ++x) on[s.get(x, y)] = true;
    p.n_used = 0;
    for (int i = 255; i >= 0; --i)
        if (on[i]) p.used[p.n_used++] = static_cast<uint8_t>(i);
    p.waits0 = p.n_used ? 1 : 0;
    for (int i = 0; i < p.n_used; ++i)
        if (!(i & 16)) ++p.waits0;
    int m = 0;
    for (int k = 0; k < p.n_used; ++k)
        for (int c = 0; c < 3; ++c) {
            const int j = k * 3 + c, v = p.dac[p.used[k]][c], t = p.dac[0][c];
            p.cur[j] = static_cast<uint8_t>(v);
            p.sign[j] = static_cast<int8_t>(v >= t ? -1 : 1);
            p.dist[j] = static_cast<uint8_t>(v >= t ? v - t : t - v);
            if (p.dist[j] > m) m = p.dist[j];
        }
    p.steps = p.n_used ? m : 0;
    for (int j = 0; j < p.n_used * 3; ++j) p.acc[j] = static_cast<uint8_t>(m >> 1);
}

// The fade's work due by now; true when it's done (the screen cleared)
bool fade_run(Player& p, Screen& s, uint32_t now, uint32_t* next)
{
    if (p.fading == 1) {
        while (p.step < p.steps) {
            const uint32_t at = p.fade_at + static_cast<uint32_t>(p.waits0 + p.step + 1) * kRetrace;
            if (static_cast<int32_t>(now - at) < 0) {
                *next = ms_of(at);
                return false;
            }
            for (int j = 0; j < p.n_used * 3; ++j) {
                int a = p.acc[j] + p.dist[j];
                if (a > p.steps) {
                    a -= p.steps;
                    p.cur[j] = static_cast<uint8_t>(p.cur[j] + p.sign[j]);
                }
                p.acc[j] = static_cast<uint8_t>(a);
            }
            for (int k = 0; k < p.n_used; ++k) {
                uint8_t* d = p.dac[p.used[k]];
                d[0] = p.cur[k * 3];
                d[1] = p.cur[k * 3 + 1];
                d[2] = p.cur[k * 3 + 2];
                s.colour(p.used[k], d[0], d[1], d[2]);
            }
            ++p.step;
        }
        const uint32_t end = p.fade_at + static_cast<uint32_t>(p.waits0) * kRetrace;
        if (static_cast<int32_t>(now - end) < 0) {
            *next = ms_of(end);
            return false;
        }
    } else {
        constexpr uint32_t kBatch = static_cast<uint32_t>(kW) * kH / 60;
        while (p.left) {
            const uint32_t at = p.fade_at + p.batch * kRetrace;
            if (static_cast<int32_t>(now - at) < 0) {
                *next = ms_of(at);
                return false;
            }
            for (uint32_t n = 0; n < kBatch && p.left;) {
                int x = 0, y = 0;
                if (p.lfsr) {
                    p.lfsr = lfsr_next(p.lfsr);
                    x = static_cast<int>(p.lfsr >> 8);
                    y = static_cast<int>(p.lfsr & 0xFF);
                    if (x >= kW || y >= kH) continue;
                } else {
                    p.lfsr = 1;
                }
                s.put(x, y, 0);
                --p.left;
                ++n;
            }
            ++p.batch;
        }
    }
    clear(s);
    p.fading = 0;
    p.resend = true;
    return true;
}

}  // namespace

uint32_t lfsr_next(uint32_t s)
{
    const bool out = s & 1;
    s >>= 1;
    if (out) s ^= 0x12000;                      // (x^17 + x^14 + 1)
    return s;
}

void ega_pairs(const uint8_t (*pal)[3], uint8_t (*out)[2])
{
    // The 136 pairs a, b (a >= b), in EGA.DRV's order: a from 15 down, b from a down
    struct Pair { uint8_t sum[3], a, b; uint16_t spread; };
    Pair pr[136];
    int n = 0;
    for (int a = 15; a >= 0; --a)
        for (int b = a; b >= 0; --b) {
            Pair& q = pr[n++];
            int sp = 0;
            for (int c = 0; c < 3; ++c) {
                q.sum[c] = static_cast<uint8_t>(kEga6[a][c] + kEga6[b][c]);
                sp += sq(kEga6[a][c] - kEga6[b][c]);
            }
            q.spread = static_cast<uint16_t>(a == 15 && b == 14 ? 0 : sp >> 1);
            q.a = static_cast<uint8_t>(a);
            q.b = static_cast<uint8_t>(b);
        }
    for (int i = 0; i < 256; ++i) {
        int best = 0x7FFFFFFF, at = 0;
        for (int k = n - 1; k >= 0; --k) {       // (from the last pair: the first lowest kept)
            int cost = pr[k].spread;
            for (int c = 0; c < 3; ++c) cost += sq(pr[k].sum[c] - 2 * pal[i][c]);
            if (cost < best) {
                best = cost;
                at = k;
            }
        }
        out[i][0] = pr[at].b;
        out[i][1] = pr[at].a;
    }
}

bool draw_frame(dax::ByteSource& bma, uint32_t chunk_pos, int f, Screen& s, const uint8_t (*pair)[2])
{
    bool ok = true;
    uint8_t h[2];
    if (bma.read_at(chunk_pos + 4, h, 2) != 2) return false;
    if (f < 0 || f >= (h[0] | h[1] << 8)) return false;
    const uint32_t size = rd32(bma, chunk_pos, &ok);
    const uint32_t off = rd32(bma, chunk_pos + 6 + 4u * static_cast<uint32_t>(f), &ok);
    if (!ok || off >= size) return false;
    uint32_t at = chunk_pos + off + 4;          // (past its width and height)
    const uint32_t end = chunk_pos + size;
    uint8_t b[4];
    while (at < end) {
        if (bma.read_at(at++, b, 1) != 1) return false;
        const int y = b[0];
        if (y == 0xFF) return true;
        for (;;) {
            if (bma.read_at(at, b, 4) != 4) return false;
            at += 4;
            int x = b[0] + ((b[1] & 1) << 8);
            const bool last = b[1] & 0x80;
            const uint32_t stop = at + b[3];
            while (at < stop) {
                uint8_t c, v;
                if (bma.read_at(at++, &c, 1) != 1) return false;
                const int n = (c >> 1) + 1;
                for (int k = 0; k < n; ++k) {
                    if (k == 0 || !(c & 1))
                        if (bma.read_at(at++, &v, 1) != 1) return false;
                    if (x < kW && y < kH) s.put(x, y, pair ? pair[v][(x + y) & 1] : v);
                    ++x;
                }
            }
            at = stop;
            if (last) break;
        }
    }
    return true;
}

bool open(Player& p, const uint8_t* acf, uint32_t acf_len, dax::ByteSource& bma, bool vga, bool sound)
{
    p = Player{};
    p.vga = vga;
    p.sound = sound;
    p.acf = acf;
    p.acf_len = acf_len;
    if (!acf || acf_len < 2 || acf[0] < 2) return false;
    uint32_t pos = 0;
    const uint32_t total = bma.size();
    while (pos + 6 <= total && p.chunks < kMaxChunks) {
        bool ok = true;
        const uint32_t size = rd32(bma, pos, &ok);
        uint8_t h[2];
        if (!ok || bma.read_at(pos + 4, h, 2) != 2) break;
        const uint16_t n = static_cast<uint16_t>(h[0] | h[1] << 8);
        if (!n || size < 6u + 4u * n || pos + size > total) break;
        p.chunk[p.chunks] = pos;
        p.frames[p.chunks++] = n;
        pos += size;
    }
    return p.chunks > 0;
}

void stop(Player& p, Screen& s)
{
    if (p.ended) return;
    p.ended = true;
    if (p.sound) s.song(0);
}

bool run(Player& p, uint32_t now_ms, dax::ByteSource& bma, Screen& s, uint32_t* next)
{
    *next = now_ms;
    if (!p.started) {                           // (a microsecond clock from the movie's start: ms x 1000
        p.started = true;                       // wraps after 71 minutes of uptime)
        p.epoch = now_ms;
    }
    g_epoch = p.epoch;
    const uint32_t now = (now_ms - p.epoch) * 1000u;
    while (!p.ended) {
        if (p.fading && !fade_run(p, s, now, next)) return true;
        if (p.pc + 2 > p.acf_len || p.acf[p.pc] < 2 || p.pc + p.acf[p.pc] > p.acf_len) break;
        const uint8_t* r = p.acf + p.pc;
        const uint32_t len = r[0];
        const uint32_t at = p.last + p.step_us;     // (a step's deadline)
        switch (r[1]) {
        case 0:
            if (len >= 4) {
                const int rate = r[2] | r[3] << 8;
                const uint32_t ticks = rate ? (p.sound ? 261u : 62u) / static_cast<uint32_t>(rate) : 0;
                p.step_us = ticks * tick_us(p);
            }
            break;
        case 1:
            stop(p, s);
            return false;
        case 2:
            if (len < 6) break;
            if (p.resend) {                         // (after a fade: the whole palette again)
                p.resend = false;
                for (int i = 0; i < 256; ++i) {
                    memcpy(p.dac[i], p.pal[i], 3);
                    if (p.vga) s.colour(i, p.pal[i][0], p.pal[i][1], p.pal[i][2]);
                }
            }
            p.pal[r[3]][0] = r[2] & 0x3F;
            p.pal[r[3]][1] = r[5] & 0x3F;
            p.pal[r[3]][2] = r[4] & 0x3F;
            memcpy(p.dac[r[3]], p.pal[r[3]], 3);
            if (p.vga) s.colour(r[3], p.pal[r[3]][0], p.pal[r[3]][1], p.pal[r[3]][2]);
            break;
        case 3:
            if (p.seq >= p.chunks) break;
            if (p.drawn) {
                if (static_cast<int32_t>(now - at) < 0) {
                    *next = ms_of(at);
                    return true;
                }
                p.last = met(p, at, now);
            } else {
                p.drawn = true;
                p.last = now;
            }
            if (!p.vga && p.fr == 0) ega_pairs(p.pal, p.pair);
            draw_frame(bma, p.chunk[p.seq], p.fr, s, p.vga ? nullptr : p.pair);
            if (++p.fr >= p.frames[p.seq]) {
                ++p.seq;
                p.fr = 0;
            }
            break;
        case 8: {                                   // the song file: "P<n>" is MUSIC.DAX block n
            char name[9] = {};
            int k = 0;
            for (uint32_t w = 4; w + 1 < len && k < 8; w += 2) {
                name[k++] = static_cast<char>(r[w + 1]);
                name[k++] = static_cast<char>(r[w]);
            }
            p.song = 0;
            if (name[0] == 'P' || name[0] == 'p')
                for (int i = 1; name[i] >= '0' && name[i] <= '9'; ++i) p.song = p.song * 10 + name[i] - '0';
            break;
        }
        case 9:
            if (p.sound && p.song) s.song(p.song);
            break;
        case 10:
            if (p.sound) s.song(0);
            break;
        case 11:
            if (static_cast<int32_t>(now - at) < 0) {
                *next = ms_of(at);
                return true;
            }
            p.pc += len;
            fade_start(p, s, met(p, at, now));
            continue;
        default:
            break;
        }
        p.pc += len;
    }
    stop(p, s);
    return false;
}

}  // namespace vace
