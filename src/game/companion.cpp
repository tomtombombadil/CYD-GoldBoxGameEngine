// The Companion panel's Party and Battle views (game/companion.h).
#include "game/companion.h"

#include <cstdio>
#include <cstring>

#include "engine/combat.h"
#include "engine/party.h"
#include "game/play.h"
#include "ui/style.h"

namespace companion {

namespace {

using style::rgb;

constexpr uint16_t kFloor   = rgb(34, 34, 44);      // open ground
constexpr uint16_t kRough   = rgb(78, 78, 84);      // an obstacle that can be crossed
constexpr uint16_t kWall    = rgb(150, 150, 150);   // walls / barriers (Tom: grey)
constexpr uint16_t kFriend  = style::kParty;        // the party and allies (Tom: green)
constexpr uint16_t kFoe     = rgb(255, 48, 48);     // the enemies (Tom: red)
constexpr uint16_t kBody    = rgb(20, 96, 20);      // a dead party member's body
constexpr uint16_t kWindow  = rgb(90, 170, 255);    // the game screen's 7 x 7 window
constexpr uint16_t kTurn    = rgb(255, 255, 255);   // the one whose turn it is
constexpr uint16_t kAim     = style::kGold;         // what is aimed at
constexpr uint16_t kBarBack = rgb(50, 50, 56);
constexpr uint16_t kBarOk   = rgb(40, 200, 40);
constexpr uint16_t kBarHalf = rgb(230, 200, 40);
constexpr uint16_t kBarLow  = rgb(230, 50, 40);

// s cut to n characters
void put(Painter& p, int x, int y, const char* s, uint16_t col, int n = 64)
{
    char b[65];
    if (n > 64) n = 64;
    if (n <= 0) return;
    snprintf(b, sizeof b, "%.*s", n, s);
    p.text(x, y, b, col);
}

const char* health_word(int h)
{
    switch (h) {
    case party::Okay:        return "Okay";
    case party::Animated:    return "Animated";
    case party::TempGone:    return "Away";
    case party::Running:     return "Fled";
    case party::Unconscious: return "Unconscious";
    case party::Dying:       return "Dying";
    case party::Dead:        return "Dead";
    case party::Stoned:      return "Stoned";
    case party::Gone:        return "Gone";
    }
    return "";
}

// The words for a member's state: its health (unless Okay), then its
// effects, worst first, as many whole words as fit in n characters ("+"
// when some are left out). The line's colour: red for the dead / dying,
// gold for those out of action, cream otherwise.
uint16_t status_line(int health, uint16_t fx, char* out, int n)
{
    static const struct { uint16_t bit; const char* w; } kWords[] = {
        {play::kPxAsleep, "Asleep"},     {play::kPxParalysed, "Paralysed"}, {play::kPxHeld, "Held"},
        {play::kPxStunned, "Stunned"},   {play::kPxFleeing, "Fleeing"},     {play::kPxCharmed, "Charmed"},
        {play::kPxAfraid, "Afraid"},     {play::kPxPoisoned, "Poisoned"},   {play::kPxDiseased, "Diseased"},
        {play::kPxBlind, "Blind"},       {play::kPxCursed, "Cursed"},       {play::kPxSlowed, "Slowed"},
        {play::kPxBlessed, "Blessed"},   {play::kPxHasted, "Hasted"},       {play::kPxInvisible, "Invisible"},
        {play::kPxProtected, "Protected"},
    };
    int len = 0;
    out[0] = 0;
    bool cut = false;
    auto add = [&](const char* w) {
        const int wl = static_cast<int>(strlen(w)) + (len ? 1 : 0);
        if (cut || len + wl > n - 1) {
            cut = true;
            return;
        }
        len += snprintf(out + len, static_cast<size_t>(n + 1 - len), "%s%s", len ? " " : "", w);
    };
    if (health != party::Okay || !fx) add(health_word(health));
    for (const auto& k : kWords)
        if (fx & k.bit) add(k.w);
    if (cut && len < n) {
        out[len++] = '+';
        out[len] = 0;
    }
    if (health == party::Dead || health == party::Dying || health == party::Stoned || health == party::Gone)
        return style::kWarn;
    if (health == party::Unconscious || health == party::Running || (fx & play::kPxBlink)) return style::kGold;
    return health == party::Okay && !fx ? style::kTextMuted : style::kText;
}

// ---- Party
struct Rows {
    int y0, pitch, icon, tx, tw;
};

Rows party_rows(const Area& a, int n)
{
    Rows r;
    r.y0 = a.y + 11;
    r.pitch = n ? (a.h - 11) / n : 24;
    if (r.pitch > 24) r.pitch = 24;
    r.icon = r.pitch < 24 ? r.pitch : 24;
    r.tx = a.x + 2 + 24 + 3;
    r.tw = a.x + a.w - 2 - r.tx;
    return r;
}

void party_icon(Painter& p, int i, int x, int y, int size)
{
    uint8_t px[24 * 24], see[(24 * 24 + 7) / 8];
    int w = 0, h = 0;
    if (!play::panel_icon(i, px, see, &w, &h) || w <= 0 || h <= 0) return;
    // (24 x 24 into size x size, the nearest pixel; a smaller picture centred)
    const int dw = w * size / 24, dh = h * size / 24;
    const int ox = x + (size - dw) / 2, oy = y + (size - dh);
    for (int j = 0; j < dh; ++j) {
        const int sy = j * h / dh;
        for (int i2 = 0; i2 < dw; ++i2) {
            const int sx = i2 * w / dw, k = sy * w + sx;
            if (see[k >> 3] >> (k & 7) & 1) continue;
            // (runs of one colour as one fill)
            int run = 1;
            while (i2 + run < dw) {
                const int sx2 = (i2 + run) * w / dw, k2 = sy * w + sx2;
                if ((see[k2 >> 3] >> (k2 & 7) & 1) || px[k2] != px[k]) break;
                ++run;
            }
            p.fill(ox + i2, oy + j, run, 1, p.colour(px[k]));
            i2 += run - 1;
        }
    }
}

void party_row(Painter& p, const Rows& r, int i, const play::PanelMember& m)
{
    const int y = r.y0 + i * r.pitch, cols = r.tw / kCharW;
    char hp[16];
    snprintf(hp, sizeof hp, "%d/%d", m.hp, m.hp_max);
    const int hl = static_cast<int>(strlen(hp));
    put(p, r.tx, y, m.name, m.current ? style::kGold : style::kText, cols - hl - 1);
    put(p, r.tx + (cols - hl) * kCharW, y, hp, style::kText);
    // The bar: green, yellow under half, red under a quarter
    const int by = y + 9, bh = r.pitch >= 24 ? 4 : 3;
    p.fill(r.tx, by, r.tw, bh, kBarBack);
    const bool up = m.health == party::Okay || m.health == party::Animated;
    if (m.hp_max > 0 && m.hp > 0) {
        int fw = r.tw * (m.hp > m.hp_max ? m.hp_max : m.hp) / m.hp_max;
        if (fw < 1) fw = 1;
        const uint16_t col = !up ? style::kTextMuted
                           : m.hp * 4 < m.hp_max ? kBarLow
                           : m.hp * 2 < m.hp_max ? kBarHalf
                                                 : kBarOk;
        p.fill(r.tx, by, fw, bh, col);
    }
    char st[40];
    const uint16_t sc = status_line(m.health, m.fx, st, cols < 39 ? cols : 39);
    put(p, r.tx, by + bh + 2, st, sc);
}

// ---- Battle: the last draw's geometry
struct Geo {
    Area a{};
    int  fx = 0, fy = 0, c = 3, bx = 0, by = 0, bw = combat::kW, bh = combat::kH;
    bool ok = false;
} geo;

uint16_t square_colour(int s) { return s == 3 ? kWall : s == 2 ? kRough : s == 1 ? kFloor : style::kBackground; }

void field_square(Painter& p, int x, int y)
{
    if (x < geo.bx || y < geo.by || x >= geo.bx + geo.bw || y >= geo.by + geo.bh) return;
    p.fill(geo.fx + (x - geo.bx) * geo.c, geo.fy + (y - geo.by) * geo.c, geo.c, geo.c,
           square_colour(play::panel_square(x, y)));
}

// A ring round squares (x, y) .. + n - 1, clipped to the field
void ring(Painter& p, int x, int y, int n, uint16_t col)
{
    int x0 = geo.fx + (x - geo.bx) * geo.c - 1, y0 = geo.fy + (y - geo.by) * geo.c - 1;
    int x1 = x0 + n * geo.c + 2, y1 = y0 + n * geo.c + 2;
    const int lx = geo.fx - 1, ly = geo.fy - 1, rx = geo.fx + geo.bw * geo.c + 1, ry = geo.fy + geo.bh * geo.c + 1;
    if (x1 <= lx || y1 <= ly || x0 >= rx || y0 >= ry) return;
    const bool top = y0 >= ly, bot = y1 <= ry, left = x0 >= lx, right = x1 <= rx;
    if (x0 < lx) x0 = lx;
    if (y0 < ly) y0 = ly;
    if (x1 > rx) x1 = rx;
    if (y1 > ry) y1 = ry;
    if (top) p.fill(x0, y0, x1 - x0, 1, col);
    if (bot) p.fill(x0, y1 - 1, x1 - x0, 1, col);
    if (left) p.fill(x0, y0, 1, y1 - y0, col);
    if (right) p.fill(x1 - 1, y0, 1, y1 - y0, col);
}

bool blinks(const play::PanelDot& d) { return (d.down && !d.dead) || (d.fx & play::kPxBlink); }

void dot(Painter& p, const play::PanelDot& d, bool blink_on)
{
    const int n = d.size;
    for (int j = 0; j < n; ++j)
        for (int i = 0; i < n; ++i) field_square(p, d.x + i, d.y + j);
    if (blinks(d) && !blink_on) return;
    const uint16_t col = d.dead ? kBody : d.enemy ? kFoe : kFriend;
    const int in = geo.c >= 5 ? 1 : 0;
    int x = geo.fx + (d.x - geo.bx) * geo.c + in, y = geo.fy + (d.y - geo.by) * geo.c + in;
    int s = n * geo.c - 2 * in;
    if (d.down && geo.c >= 4) {
        // (a body: a smaller dot)
        x += geo.c / 4;
        y += geo.c / 4;
        s -= geo.c / 2;
    }
    p.fill(x, y, s, s, col);
}

// The marks over the dots: the window, the turn's ring, the aim's
void marks(Painter& p)
{
    play::PanelField f;
    if (!play::panel_field(f)) return;
    ring(p, f.vx, f.vy, 7, kWindow);
    for (int i = 0; i < f.n; ++i) {
        play::PanelDot d;
        if (play::panel_dot(i, d) && d.current) ring(p, d.x, d.y, d.size, kTurn);
    }
    if (f.tx >= 0) ring(p, f.tx, f.ty, f.ts, kAim);
}

} // namespace

void draw_tabs(Painter& p, int x, int y, View v, bool fight)
{
    static const char* const kNames[3] = {"Map", "Party", "Battle"};
    for (int i = 0; i < (fight ? 3 : 2); ++i) {
        p.text(x, y, kNames[i], static_cast<int>(v) == i ? style::kGold : style::kTextMuted);
        x += (static_cast<int>(strlen(kNames[i])) + 1) * kCharW;
    }
}

void draw_party(Painter& p, const Area& a, bool all)
{
    const int n = play::panel_members();
    const Rows r = party_rows(a, n);
    if (all) {
        p.fill(a.x, a.y, a.w, a.h, style::kBackground);
        draw_tabs(p, a.x + 2, a.y + 1, View::Party, play::fighting());
        if (!n) put(p, a.x + 2, r.y0 + 2, "No party yet.", style::kTextMuted);
    }
    for (int i = 0; i < n; ++i) {
        play::PanelMember m;
        if (!play::panel_member(i, m)) continue;
        const int y = r.y0 + i * r.pitch;
        if (all) party_icon(p, i, a.x + 2, y, r.icon);
        else p.fill(r.tx, y, r.tw, r.pitch, style::kBackground);
        party_row(p, r, i, m);
    }
}

void draw_battle(Painter& p, const Area& a, bool blink_on, bool big)
{
    p.fill(a.x, a.y, a.w, a.h, style::kBackground);
    geo.ok = false;
    play::PanelField f;
    if (!play::panel_field(f)) return;
    const int cols = (a.w - 4) / kCharW;
    char line[64];
    int ty = a.y + 1;
    if (big) {
        snprintf(line, sizeof line, "Battle - Round %d", f.round + 1);
        put(p, a.x + 2, ty, line, style::kGold, cols);
        put(p, a.x + a.w - 2 - 18 * kCharW, ty, "Tap to go back.", style::kTextMuted, 18);
    } else {
        draw_tabs(p, a.x + 2, ty, View::Battle, true);
        ty += kLineH + 1;
    }
    // Who is still up on each side
    int ours = 0, theirs = 0;
    int bx0 = combat::kW, by0 = combat::kH, bx1 = -1, by1 = -1;
    for (int i = 0; i < f.n; ++i) {
        play::PanelDot d;
        if (!play::panel_dot(i, d)) continue;
        if (!d.down) ++(d.enemy ? theirs : ours);
        if (d.x < bx0) bx0 = d.x;
        if (d.y < by0) by0 = d.y;
        if (d.x + d.size - 1 > bx1) bx1 = d.x + d.size - 1;
        if (d.y + d.size - 1 > by1) by1 = d.y + d.size - 1;
    }
    if (!big) {
        snprintf(line, sizeof line, "Round %d", f.round + 1);
        put(p, a.x + 2, ty, line, style::kTextMuted, cols);
    }
    {
        char us[12], them[12];
        snprintf(us, sizeof us, "%d", ours);
        snprintf(them, sizeof them, "%d", theirs);
        const int w = static_cast<int>(strlen(us) + strlen(them) + 3);
        const int x = big ? a.x + 2 + 18 * kCharW : a.x + a.w - 2 - w * kCharW;
        p.text(x, ty, us, kFriend);
        p.text(x + static_cast<int>(strlen(us) + 1) * kCharW, ty, "v", style::kTextMuted);
        p.text(x + static_cast<int>(strlen(us) + 3) * kCharW, ty, them, kFoe);
    }
    // The part of the field the fight uses (its ground and fighters)
    for (int y = 0; y < combat::kH; ++y)
        for (int x = 0; x < combat::kW; ++x)
            if (play::panel_square(x, y)) {
                if (x < bx0) bx0 = x;
                if (y < by0) by0 = y;
                if (x > bx1) bx1 = x;
                if (y > by1) by1 = y;
            }
    if (bx1 < 0) bx0 = by0 = 0, bx1 = combat::kW - 1, by1 = combat::kH - 1;
    // (under it: the turn, the aim, the colours - 320x240's page leaves the blinking line out when it doesn't fit)
    const int top = ty + kLineH + 2, foot = (big ? 3 : 4) * kLineH + 3;
    const int aw = a.w - 4, ah = a.y + a.h - top - foot;
    const int bw = bx1 - bx0 + 1, bh = by1 - by0 + 1;
    int c = aw / bw < ah / bh ? aw / bw : ah / bh;
    const int cap = big ? 12 : 8;
    if (c > cap) c = cap;
    if (c < 1) c = 1;
    geo.a = a;
    geo.c = c;
    geo.bx = bx0;
    geo.by = by0;
    geo.bw = bw;
    geo.bh = bh;
    geo.fx = a.x + (a.w - bw * c) / 2;
    geo.fy = top;
    geo.ok = true;
    // The ground: a row's runs of one kind as one fill
    for (int y = 0; y < bh; ++y)
        for (int x = 0; x < bw;) {
            const int s = play::panel_square(bx0 + x, by0 + y);
            int run = 1;
            while (x + run < bw && play::panel_square(bx0 + x + run, by0 + y) == s) ++run;
            if (s) p.fill(geo.fx + x * c, geo.fy + y * c, run * c, c, square_colour(s));
            x += run;
        }
    // The fighters: bodies first, then the rest
    for (int pass = 0; pass < 2; ++pass)
        for (int i = 0; i < f.n; ++i) {
            play::PanelDot d;
            if (play::panel_dot(i, d) && d.down == (pass == 0)) dot(p, d, blink_on);
        }
    marks(p);
    // Under it: whose turn, what is aimed at, the colours
    int ly = geo.fy + bh * c + 3;
    for (int i = 0; i < f.n; ++i) {
        play::PanelDot d;
        if (!play::panel_dot(i, d) || !d.current) continue;
        snprintf(line, sizeof line, "%d/%d", d.hp, d.hp_max);
        const int hl = static_cast<int>(strlen(line));
        p.text(a.x + 2, ly, ">", kTurn);
        put(p, a.x + 2 + 2 * kCharW, ly, d.name, d.enemy ? kFoe : kFriend, cols - hl - 3);
        put(p, a.x + 2 + (cols - hl) * kCharW, ly, line, style::kText);
        ly += kLineH;
    }
    if (f.tx >= 0) {
        int t = -1;
        play::PanelDot d;
        for (int i = 0; i < f.n && t < 0; ++i)
            if (play::panel_dot(i, d) && !d.down && d.x == f.tx && d.y == f.ty) t = i;
        p.text(a.x + 2, ly, "Aim", kAim);
        if (t >= 0) {
            snprintf(line, sizeof line, "%d/%d", d.hp, d.hp_max);
            const int hl = static_cast<int>(strlen(line));
            put(p, a.x + 2 + 4 * kCharW, ly, d.name, d.enemy ? kFoe : kFriend, cols - hl - 5);
            put(p, a.x + 2 + (cols - hl) * kCharW, ly, line, style::kText);
        } else {
            put(p, a.x + 2 + 4 * kCharW, ly, "an empty square", style::kTextMuted, cols - 4);
        }
        ly += kLineH;
    }
    if (ly + kLineH <= a.y + a.h) {
        p.text(a.x + 2, ly, "Green", kFriend);
        p.text(a.x + 2 + 6 * kCharW, ly, "party", style::kTextMuted);
        p.text(a.x + 2 + 12 * kCharW, ly, "Red", kFoe);
        p.text(a.x + 2 + 16 * kCharW, ly, "foes", style::kTextMuted);
        ly += kLineH;
    }
    if (ly + kLineH <= a.y + a.h) put(p, a.x + 2, ly, big ? "Blinking: asleep, held, stunned, fleeing, down" : "Blink: asleep, held, down", style::kTextMuted, cols);
}

void blink_battle(Painter& p, bool blink_on)
{
    if (!geo.ok) return;
    play::PanelField f;
    if (!play::panel_field(f)) return;
    bool any = false;
    for (int i = 0; i < f.n; ++i) {
        play::PanelDot d;
        if (play::panel_dot(i, d) && blinks(d)) {
            dot(p, d, blink_on);
            any = true;
        }
    }
    if (any) marks(p);
}

bool battle_blinks()
{
    play::PanelField f;
    if (!play::panel_field(f)) return false;
    for (int i = 0; i < f.n; ++i) {
        play::PanelDot d;
        if (play::panel_dot(i, d) && blinks(d)) return true;
    }
    return false;
}

int battle_cell() { return geo.ok ? geo.c : 0; }

} // namespace companion
