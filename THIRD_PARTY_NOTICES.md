# Third-party notices

## Libraries (linked, not copied)
- **LovyanGFX** - FreeBSD license, https://github.com/lovyan03/LovyanGFX.
  `src/boards/touch_xpt2046_soft.hpp` follows the sampling approach of its
  XPT2046 driver (7 readings, median per axis) in our own code.
- **Arduino-ESP32 / ESP-IDF** (via the pioarduino PlatformIO platform) -
  LGPL-2.1 / Apache-2.0.

## Copied from Tom's own projects (MIT)
- From CYD-Classic-Games: `src/games/common/inflate.*` (raw DEFLATE
  decoder) is the base of `src/engine/inflate.*`, changed to stream through
  a 32 KB window. `src/ui/log_pack.*` (Send Log's packer) is
  `src/engine/log_pack.*` here, only its URL changed, and `web/l/index.html`
  is adapted from its page of the same name.
- From CYD-Classic-Games: `src/boards/*` (board files), `src/hal/storage.*`,
  `sdcard.*`, `touch_cal.*`, `panel_prefs.*`, `tools/version.py`,
  `tools/make_site.py`, `web/index.html`, `.github/workflows/build.yml` (adapted).

## Included third-party code
- **TJpgDec R0.03** (Tiny JPEG Decompressor), (C) ChaN 2021,
  http://elm-chan.org/fsw/tjpgd/ - `src/engine/third_party/tjpgd.*`, from
  Bodmer's TJpg_Decoder copy with his byte-swap change removed. License:
  "No restriction on use. You can use, modify and redistribute it for
  personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
  Redistributions of source code must retain the above copyright notice."
  Our changes are marked "CYD BYOG" (an MCU skip callback, C++ casts).

- **QR Code generator library (C)**, Copyright (c) Project Nayuki, MIT
  License, https://www.nayuki.io/page/qr-code-generator-library -
  `src/engine/third_party/qrcodegen.*`, unchanged (from
  github.com/nayuki/QR-Code-generator, commit 3c6d0b3). Send Logs' QR code.

- **ymfm** (Yamaha FM sound cores), Copyright (c) 2021 Aaron Giles, BSD
  3-Clause License (`src/engine/third_party/ymfm/LICENSE`),
  https://github.com/aaronsgiles/ymfm, commit 81aec25 -
  `src/engine/third_party/ymfm/`: ymfm.h, ymfm_fm.h / .ipp, ymfm_opl.h,
  ymfm_adpcm.h, ymfm_pcm.h unchanged; ymfm_opl.cpp, ymfm_adpcm.cpp and
  ymfm_pcm.cpp unchanged but renamed .inc (compiled through
  `src/engine/opl.cpp` / `opl_pcm.cpp`). The YM3812 (AdLib) chip the scan
  renders the games' music with.
- **The PDF core fonts' widths** (`src/engine/pdf_std14.h`, made by `tools/make_pdf_std14.py`): the character
  widths of Times-Roman, -Bold, -Italic, -BoldItalic, Helvetica and Helvetica-Bold from Adobe's Font Metrics
  (AFM) files for the 14 PDF core fonts. Copyright (c) 1985 - 1997 Adobe Systems Incorporated. All Rights
  Reserved. Modified: only the widths, as tables by WinAnsi code. Adobe's terms: "This file and the 14
  PostScript(R) AFM files it accompanies may be used, copied, and distributed for any purpose and without
  charge, with or without modification, provided that all copyright notices are retained; that the AFM files
  are not distributed without this file; that all modifications to this file or any of the AFM files are
  prominently noted in the modified file(s); and that this paragraph is not modified. Adobe Systems has no
  responsibility or obligation to support the use of the AFM files." (The AFM files themselves are not in the
  repo.) Times is a trademark of Linotype-Hell AG and/or its subsidiaries; Helvetica of Linotype-Hell AG.

## Loaded by the web pages (not copied into the repo)
- **ESP Web Tools** (Apache License 2.0) - the installer page loads it from unpkg.

## Used for reference only (nothing copied)
- **coab** - Curse of the Azure Bonds reimplementation in C# by Simeon Pilgrim,
  https://github.com/simeonpilgrim/coab. No license file, so all rights
  reserved: used only to learn the DAX, picture, GEO and ECL formats.
- **Espressif's es8311 codec driver** (Apache License 2.0, (C) Espressif
  Systems) - its register settings, with the ES8311 datasheet, as facts
  for our own `src/hal/es8311.*` (the ESP32-S3 board's sound); no code
  copied.
- **The boards' makers' tutorials** (the ESP32-S3 4.0" board's: CC BY-NC-SA;
  the ESP32-C5 2.8" board's README / setups) - pin facts only for
  `src/boards/esp32s3_40_cap.hpp` and `src/boards/esp32c5_28_res.hpp`; no
  code copied.

## Game data
None. The player supplies their own original game files on the SD card.
The journal tables (`src/engine/journal_tables.cpp`, `tools/journal/tables.json`)
hold only page numbers and positions inside the player's own journal PDF -
no text or pictures.
