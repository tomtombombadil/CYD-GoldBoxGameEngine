// The games' music, rendered to sound files at card-scan time (Tom,
// 2026-11-14; facts: the Project's claude/music_facts.md).
//
// MicroMagic's music (Pools of Darkness, Death Knights of Krynn, Gateway and
// Treasures of the Savage Frontier): MUSIC.DAX block 0 is the game's own x86
// music driver, the other blocks its songs ("GLBR"). The driver is run as
// the game ran it - in the x86 interpreter (x86.h), from the player's own
// file - and the AdLib chip writes it makes are played by an emulated OPL2
// (opl.h). How the games call it (from their programs, our own words):
//   - loaded at a paragraph boundary, entered at offset 0x100 of its
//     segment (a .COM-style image), as a C function f(cmd, a, b) by a far
//     call: cmd 0 = load the song at far pointer b:a; cmd 1 = play (a = 0);
//     cmd 2 = is anything playing (AX 1 / 0); cmd 3 = stop
//   - cmd 0 hooks INT 1Ch (the BIOS clock's user tick) and sets the PIT to
//     its own rate; every tick the handler counts, and every other tick
//     steps the song
//   - its start waits for the tick counter (the first word the handler
//     increments) to be non-zero
// Checked: Darkness's song 1 played this way gives the same OPL2 writes as
// the original in DOSBox (986 notes, every register, timing within 1 ms).
#pragma once

#include <atomic>
#include <cstdint>

#include "dax.h"

namespace music {

// Where the chip writes go (an OPL2 emulator, or a log in the tools).
class Sink {
public:
    virtual ~Sink() = default;
    virtual void opl(uint8_t reg, uint8_t val) = 0;
    // `seconds` of time pass (the chip plays on).
    virtual void advance(double seconds) = 0;
};

enum class Result : uint8_t {
    Ok,
    NoDriver,       // MUSIC.DAX has no block 0 / it isn't the driver we know
    NoSong,         // no such song block
    NoMemory,
    DriverFailed,   // the driver stopped on something the interpreter can't do
    TooLong,        // still playing after the limit (the rest cut off)
};

const char* result_text(Result r);

// Renders song `song` of a MicroMagic MUSIC.DAX: runs the driver, tick by
// tick, sending its chip writes and the time between them to `sink`, until
// the driver says it has stopped (then `tail` seconds more for the notes to
// die away) or `max_seconds`. The length played is put in *seconds.
// Memory: the driver's image + 4 KB, the song, a 2 KB stack - all freed at
// the end (about 25 KB for the GOG files).
Result render_micromagic(dax::ByteSource& music_dax, int song, Sink& sink, double max_seconds = 600,
                         double tail = 1.5, double* seconds = nullptr);

// Renders sequence `seq` of an XMIDI file with Miles' AIL 2 driver (the
// Dark Queen's ADLIB.ADV) and its Global Timbre Library (INSTR.AD) - the
// driver run as the game runs it (music.cpp); as render_micromagic.
// Memory: the three files + the driver's state and cache, ~40 KB.
Result render_ail(dax::ByteSource& adv, dax::ByteSource& xmi, dax::ByteSource& gtl, int seq, Sink& sink,
                  double max_seconds = 600, double tail = 1.5, double* seconds = nullptr);

// Renders an Electronic Arts sound driver's song (Champions of Krynn's
// title: MIDI.DAX's driver block, SOUND.DAX's song block - both loaded by
// the caller) ticked at the PIT divisor `pit` as the game's program sets it
// (music.cpp); ends 8 s after the last chip write, else as above.
Result render_ea(const uint8_t* drv, uint32_t drv_len, const uint8_t* song, uint32_t song_len, uint16_t pit,
                 Sink& sink, double max_seconds = 600, double tail = 1.5, double* seconds = nullptr);

// The song blocks a MUSIC.DAX holds (1 up; 0 is the driver), as a bit mask.
uint32_t micromagic_songs(dax::ByteSource& music_dax);

// The rendered songs: the files the scan writes - 16-bit mono WAV at kRate
// (Tom, 2026-11-14: 16-bit 22 kHz; v0.197.0's were 8-bit 11 kHz), the
// output's own rate, so they play without resampling
constexpr uint32_t kRate = 22050;
constexpr uint32_t kWavHeader = 44;

// Whether a WAV header is one of these files' (16-bit mono PCM at kRate)
bool wav_ours(const uint8_t* h);
// The data length a header gives (bytes)
uint32_t wav_data(const uint8_t* h);

// A song played from its file: the game reads the file a piece at a time
// into the ring (push, from its own loop - a card read never happens in the
// audio task), the audio task plays from it (render8 / render16). An empty
// ring plays silence until more comes (a long card read); end() then the
// ring empty = the song is over. Threads: one pusher, one renderer. The
// ring keeps 16-bit samples for a 16-bit output (`wide`), else 8-bit ones
// (the DAC boards': twice the time in the same memory).
class Stream {
public:
    // The ring (`bytes`, from malloc - freed by the destructor)
    Stream(uint8_t* ring, uint32_t bytes, bool wide)
        : ring_(ring), cap_(wide ? bytes / 2 : bytes), wide_(wide) {}
    ~Stream();
    uint32_t room() const;                          // samples push() takes now
    void push(const int16_t* p, uint32_t n);
    void end() { end_.store(true); }                // no more data
    // n samples, volume 0-255; false once the song has played out
    bool render8(uint8_t* out, int n, int volume);
    bool render16(int16_t* out, int n, int volume);
    bool done() const { return done_.load(); }

private:
    int next(bool& have);                           // the next sample (16-bit scale)
    uint8_t* ring_;
    uint32_t cap_;                                  // samples
    bool wide_;
    std::atomic<uint32_t> head_{0}, tail_{0};       // written / read, counting up
    std::atomic<bool> end_{false}, done_{false};
};

}  // namespace music
