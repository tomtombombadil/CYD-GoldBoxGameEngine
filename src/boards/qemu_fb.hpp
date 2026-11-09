// The emulator build only (-D CYD_QEMU on an S3 board, tools/qemu/qemu.ini):
// a panel that is a plain frame buffer in PSRAM, in place of the board's SPI
// panel (QEMU has no display) - LovyanGFX's Panel_FrameBufferBase does the
// drawing, rotation and reading back. It gives TAPS.TXT's "shot <name>" the
// firmware's real pixels (ui::qemu_shot -> serial -> run_qemu.py -> PNG).
// Never in a board build (board files include it under #if CYD_QEMU only).
#pragma once

#if defined(CYD_QEMU)
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <esp_heap_caps.h>
#include <cstring>

class Panel_QemuFB : public lgfx::Panel_FrameBufferBase
{
public:
    Panel_QemuFB()
    {
        _write_depth = lgfx::color_depth_t::rgb565_2Byte;
        _read_depth  = lgfx::color_depth_t::rgb565_2Byte;
    }

    lgfx::color_depth_t setColorDepth(lgfx::color_depth_t) override { return _write_depth; }

    bool init(bool use_reset) override
    {
        if (!_lines_buffer) {
            // (the panel's memory size: config's memory_width x memory_height, 2 bytes a pixel)
            const int w = _cfg.memory_width, h = _cfg.memory_height;
            uint8_t* px = static_cast<uint8_t*>(heap_caps_calloc(static_cast<size_t>(w) * h, 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
            uint8_t** lines = static_cast<uint8_t**>(heap_caps_malloc(h * sizeof(uint8_t*), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
            if (!px || !lines) {
                heap_caps_free(px);
                heap_caps_free(lines);
                return false;
            }
            for (int y = 0; y < h; ++y) lines[y] = px + static_cast<size_t>(y) * w * 2;
            _lines_buffer = lines;
        }
        return Panel_FrameBufferBase::init(use_reset);
    }
};
#endif
