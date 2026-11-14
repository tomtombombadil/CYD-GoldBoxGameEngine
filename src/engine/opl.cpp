// The OPL2 for the music renderer (opl.h). ymfm's sources are compiled here
// (kept as .inc files so every build - boards, host tests, tools - gets them
// the same way, as tjpgd.inc; ymfm_pcm.inc in opl_pcm.cpp: it includes
// ymfm_fm.ipp too, which has no include guard).
#include "opl.h"

#include <cstdlib>
#include <new>

#include "third_party/ymfm/ymfm_opl.h"
#include "third_party/ymfm/ymfm_adpcm.inc"
#include "third_party/ymfm/ymfm_opl.inc"

namespace music {

namespace {
struct Chip {
    ymfm::ymfm_interface intf;
    ymfm::ym3812 opl{intf};
};
constexpr double kNative = 3579545.0 / 72.0;
}  // namespace

Opl2Sink::Opl2Sink(uint32_t rate, SampleOut out, void* user, int gain)
    : rate_(rate), out_(out), user_(user), gain_(gain) {
    void* mem = malloc(sizeof(Chip));
    if (!mem) return;
    Chip* c = new (mem) Chip();
    c->opl.reset();
    chip_ = c;
}

Opl2Sink::~Opl2Sink() {
    flush();
    if (chip_) {
        Chip* c = static_cast<Chip*>(chip_);
        c->~Chip();
        free(c);
    }
}

void Opl2Sink::opl(uint8_t reg, uint8_t val) {
    if (!chip_) return;
    Chip* c = static_cast<Chip*>(chip_);
    c->opl.write_address(reg);
    c->opl.write_data(val);
}

void Opl2Sink::advance(double seconds) {
    if (!chip_) return;
    Chip* c = static_cast<Chip*>(chip_);
    due_ += seconds * kNative;
    const double per_out = kNative / rate_;
    while (native_ + 1 <= due_) {
        ymfm::ym3812::output_data o;
        c->opl.generate(&o, 1);
        native_ += 1;
        acc_ += o.data[0];
        ++accn_;
        if (native_ >= (made_ + 1) * per_out) {
            int v = acc_ / accn_ * gain_;
            acc_ = 0;
            accn_ = 0;
            if (v > 32767) v = 32767;
            if (v < -32768) v = -32768;
            buf_[nbuf_++] = static_cast<int16_t>(v);
            ++made_;
            if (v < -512 || v > 512) last_loud_ = made_;   // (quiet: within 2 of an 8-bit sample's middle)
            if (nbuf_ == static_cast<int>(sizeof buf_ / sizeof buf_[0])) flush();
        }
    }
}

void Opl2Sink::flush() {
    if (nbuf_ && out_) out_(buf_, static_cast<uint32_t>(nbuf_), user_);
    nbuf_ = 0;
}

}  // namespace music
