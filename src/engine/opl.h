// An AdLib (YM3812 / OPL2) chip for the music renderer (music.h): the chip
// writes a driver makes go in, 8-bit mono samples come out at `rate` Hz.
// The chip itself is Aaron Giles' ymfm (BSD 3-Clause; third_party/ymfm,
// THIRD_PARTY_NOTICES.md), run at its own rate (3,579,545 / 72 = 49,716 Hz)
// and averaged down to `rate`.
#pragma once

#include <cstdint>

#include "music.h"

namespace music {

// Takes the samples as they are made (a file writer on the board).
using SampleOut = void (*)(const uint8_t* samples, uint32_t n, void* user);

class Opl2Sink : public Sink {
public:
    // Allocates the chip (about 9 KB); ok() false if that failed.
    Opl2Sink(uint32_t rate, SampleOut out, void* user, int gain = 3);
    ~Opl2Sink() override;
    bool ok() const { return chip_ != nullptr; }

    void opl(uint8_t reg, uint8_t val) override;
    void advance(double seconds) override;
    void flush();

    uint32_t samples() const { return made_; }
    uint32_t last_sound() const { return last_loud_; }   // the sample after the last not quiet one

private:
    void* chip_ = nullptr;              // ymfm::ym3812 + its interface
    uint32_t rate_;
    SampleOut out_;
    void* user_;
    int gain_;
    double native_ = 0;                 // chip samples made
    double due_ = 0;                    // chip samples the time so far asks for
    int32_t acc_ = 0;
    int accn_ = 0;
    uint8_t buf_[256];
    int nbuf_ = 0;
    uint32_t made_ = 0, last_loud_ = 0;
};

}  // namespace music
