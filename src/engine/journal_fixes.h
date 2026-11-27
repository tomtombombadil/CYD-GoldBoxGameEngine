// The Steam releases' journal text, corrected: their Gold Box Companion
// typists' slips, the apostrophes typed as spaces, the PDF sections' OCR.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "dax.h"

namespace journal {

// A Game.dat's key (its first 4 KB's FNV-1a; 0: none)
uint32_t fix_key(dax::ByteSource& dat);
// An entry's text corrected in place (text[0, len), room for cap): the new length
size_t fix_text(uint32_t key, char kind, int number, char* text, size_t len, size_t cap);

// A section whose text layer can't be read (print over a picture): shown as
// its page's picture instead, as a GOG journal's entries are - the PDF page
// (from 1), which of its pictures, and the part of it (its pixels)
struct PagePicture {
    int page = 0, image = 0;
    int x = 0, y = 0, w = 0, h = 0;
};
bool fix_picture(uint32_t key, char kind, int number, PagePicture* out);

} // namespace journal
