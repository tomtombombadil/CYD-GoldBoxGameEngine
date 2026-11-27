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

} // namespace journal
