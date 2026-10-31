// The text of a typeset PDF's pages (the Steam / SNEG journals: InDesign
// books, every word a glyph of an embedded font - no page pictures): each
// page's content streams followed in their order (the order the book is
// read in), the glyphs turned into characters through each font's
// /ToUnicode map (else its /Encoding and /Differences, else WinAnsi) and
// gathered into lines as the page places them.
//
// The characters are plain ASCII (the engine's fonts): curly quotes,
// dashes, ligatures and accented letters folded to their plain forms.
//
// Not a general PDF reader: no Form XObjects' text, no vertical writing,
// no Type3 fonts' own maps (their /ToUnicode only).
//
// Plain C++, host-tested in tools/host_tests/test_dax.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dax.h"
#include "pdf.h"

namespace pdf {

// A line of a page: what one baseline shows, left to right as drawn
struct TextLine {
    const char* text = nullptr;     // ASCII, len chars (not 0-terminated)
    int         len = 0;
    float       x = 0, x_end = 0;   // where it starts / ends (points, from the page's left)
    float       y = 0;              // its baseline (points, from the page's bottom)
    float       size = 0;           // its first letter's size (points)
};

struct TextSink {
    virtual ~TextSink() = default;
    // false: stop reading the page
    virtual bool line(const TextLine& l) = 0;
};

// Reads pages' text. Holds ~50 KB while it lives (two object buffers, the
// 32 KB inflate window, the page's fonts): make one for a whole PDF.
class TextReader {
public:
    TextReader();
    ~TextReader();
    TextReader(const TextReader&) = delete;
    TextReader& operator=(const TextReader&) = delete;
    // Its memory came (else every page fails)
    bool ok() const;
    // The page's lines through out, in the content's order. Returns how
    // many; -1: the page couldn't be read (or no memory - no_memory()).
    int page(dax::ByteSource& src, const Doc& doc, int page_obj, TextSink& out);
    bool no_memory() const;
    // Its inflate window, to lend to Doc::window (never in use between
    // page() calls' own reads)
    uint8_t* window() const;

    struct Impl;

private:
    Impl* d_;
};

// Unicode code point -> ASCII (up to 3 chars into out, 0-terminated);
// returns how many (0: nothing to show)
int fold(uint32_t u, char out[4]);

} // namespace pdf
