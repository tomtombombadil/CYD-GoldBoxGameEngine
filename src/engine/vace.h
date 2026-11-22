// VACE: SSI's "Animation Control Engine" (VACE.EXE, 1991) - Pools of
// Darkness plays its ending (and its intro) with it: CONTROL.EXE runs
// "vace <mode> FINAL 1 <sound>" between two runs of GAME.EXE. This plays
// the player's FINAL.ACF / FINAL.BMA the way VACE does, on the engine's
// canvas. Facts (the Project's claude/vace_facts.md; checked frame for
// frame against VACE in DOSBox, VGA and EGA, 2026-11-22), in our own words:
//
// ACF: records of (u8 length in bytes, u8 type, arguments), read as words.
//   0 rate r: a frame step waits 261 / r ticks of 72.82 Hz (sound on - the
//     music driver's timer) or 62 / r ticks of 18.21 Hz (sound off)
//   1 the end
//   2 a colour: bytes (red, index, blue, green), 6-bit values
//   3 the next frame: it waits until the frame before + the step (the
//     very first frame doesn't wait), then is drawn
//   5 a sequence (BMA chunk) entry - the chunks are walked in order
//   8 the song: its file name in the words' high-then-low bytes ("P3"); 9
//     plays it (sound on), 10 stops it
//   11 waits until the frame before + the step, then the fade (VGA) or the
//     dissolve (EGA) to colour 0, then the screen cleared to colour 0; the
//     next colour sends the whole palette again first
//   12, 13, 14: nothing that shows
// BMA: chunks one after another, each u32 size, u16 frames, u32 offsets of
//   the frames (from the chunk's start); a frame is u16 width, u16 height,
//   then lines: u8 y (0xFF ends the frame), then pieces (u8 x, u8 flags -
//   bit 0: x + 256, bit 7: the line's last - u8 width, u8 bytes) of codes:
//   an odd byte c repeats the next byte c / 2 + 1 times, an even one copies
//   c / 2 + 1 bytes. Pixels not in a frame stay as they were.
// VGA fade (VGA.DRV): the colours on screen, highest first; one retrace,
//   then their current values read - a retrace before each of those whose
//   place in that list has bit 4 clear; then max-difference steps toward
//   colour 0's value (each component stepped Bresenham-like: an
//   accumulator starting at half the steps), a retrace before each step.
// EGA (EGA.DRV): each colour shows as a checkerboard of two EGA colours,
//   the lower numbered where x + y is even - the pair whose average is
//   nearest (squared differences of doubled 6-bit values, wrapping as
//   signed bytes) plus half the pair's own spread (the pair 15 / 14 none),
//   found when a sequence starts, from the palette then; the dissolve
//   paints colour 0 over the screen in the order of a 17-bit shift register
//   (row = its low 8 bits, column = the rest; 0, 0 first), 64000 / 60
//   pixels a retrace.
//
// Plain C++ (no Arduino), host-tested in tools/host_tests/test_dax.cpp.
#pragma once

#include <cstdint>

#include "dax.h"

namespace vace {

constexpr int kW = 320, kH = 200;
constexpr int kMaxChunks = 32;

// Where the movie draws (the front end's canvas)
class Screen {
public:
    virtual ~Screen() = default;
    // A pixel: a VGA palette index (vga) or an EGA colour 0-15
    virtual void put(int x, int y, uint8_t v) = 0;
    virtual uint8_t get(int x, int y) const = 0;
    // A palette entry shows these 6-bit values (VGA only)
    virtual void colour(int i, uint8_t r, uint8_t g, uint8_t b) = 0;
    // Song n starts (the song file "P<n>" - MUSIC.DAX block n); 0 stops it
    virtual void song(int n) = 0;
};

struct Player {
    bool vga = true, sound = true;
    const uint8_t* acf = nullptr;
    uint32_t acf_len = 0, pc = 0;
    uint32_t chunk[kMaxChunks] = {};
    uint16_t frames[kMaxChunks] = {};
    int chunks = 0, seq = 0, fr = 0;
    bool drawn = false, resend = false, ended = false, started = false;
    uint32_t epoch = 0;                   // (the first run's ms: the clock counts us from it)
    uint32_t last = 0, step_us = 0;       // the last frame's time (us), the frame step (us)
    int song = 0;
    uint8_t pal[256][3] = {};             // the ACF's colours
    uint8_t dac[256][3] = {};             // what the screen shows (VGA)
    uint8_t pair[256][2] = {};            // EGA: even / odd colours
    // The fade / dissolve under way (0 none, 1 VGA, 2 EGA)
    uint8_t fading = 0;
    uint32_t fade_at = 0;                 // its start (us)
    int n_used = 0, steps = 0, step = 0, waits0 = 0;
    uint8_t used[256] = {};
    uint8_t cur[768] = {}, dist[768] = {}, acc[768] = {};
    int8_t sign[768] = {};
    uint32_t lfsr = 0, left = 0, batch = 0;
};

// The ACF in memory (kept until the end) and the BMA's chunk table; false
// when they don't read as VACE's files
bool open(Player& p, const uint8_t* acf, uint32_t acf_len, dax::ByteSource& bma, bool vga, bool sound);
// Runs what is due by `now` (ms). False when the movie has ended; else
// *next is when to call again (ms). (Its clock counts microseconds: the
// steps are 219.7 ms - whole milliseconds drifted 1 ms a frame.)
bool run(Player& p, uint32_t now, dax::ByteSource& bma, Screen& s, uint32_t* next);
// A key: VACE ends at once (its key check)
void stop(Player& p, Screen& s);

// Pieces, for the tests and the front end
// Frame f of the chunk at chunk_pos: its pixels to the screen (EGA: through pair)
bool draw_frame(dax::ByteSource& bma, uint32_t chunk_pos, int f, Screen& s, const uint8_t (*pair)[2]);
// EGA.DRV's two colours for each palette entry (6-bit values)
void ega_pairs(const uint8_t (*pal)[3], uint8_t (*out)[2]);
// The dissolve's order: the next state of the 17-bit register (state 1 first)
uint32_t lfsr_next(uint32_t s);

}  // namespace vace
