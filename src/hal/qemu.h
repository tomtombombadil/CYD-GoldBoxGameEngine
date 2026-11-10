// The emulator build (-D CYD_QEMU, tools/qemu/qemu.ini; never flashed):
// Espressif's QEMU runs the real firmware, the card mounted 1-bit SD/MMC,
// taps played from the card's /GOLDBOX/_QEMU/TAPS.TXT, the heap logged.
// Defined in tools/qemu/lib/qemu_hal (only that build links it).
#pragma once

#if defined(CYD_QEMU)
#include <FS.h>
#include <cstdint>

namespace qemu {
bool     sd_begin();
fs::FS&  sd_fs();
void     sd_end();
// The next scripted touch: true while "pressed" (x, y in screen pixels)
bool     touch(int32_t* x, int32_t* y);
// A "journal J 4" step of the script (once)
bool     journal(char* kind, int* number);
// The heap / stack line now
void     mem_now(const char* why);
// A "shot <name>" step of the script (once): the viewer dumps the screen
// (ui::qemu_shot) when it's drawn
bool     shot(char* name, size_t cap);
// A "runat <hex address>" step of the script (once): the game's script run
// from there while the party stands at the exploring menu (the playsim's
// RUNAT - a LOAD MONSTER's address starts its fight; the screenshots)
bool     runat(uint16_t* addr);
}
#endif
