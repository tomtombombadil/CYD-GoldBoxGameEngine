// Just enough PDF to find the page pictures of a scanned book (the GOG
// journal PDFs: every page is one JPEG - Treasures' a few, two pages of the
// book and their headings, a spread a PDF page): the cross-reference table, the
// page tree, each page's image XObject and where its JPEG data is.
// Classic xref tables (PDF 1.4 and the like) and, since Champions of Krynn's
// GOG journal (PDF 1.6, OmniPage), cross-reference streams with objects kept
// in object streams (FlateDecode, PNG predictors); /Prev sections are
// followed. Not a general PDF reader: page pictures here; the typeset
// journals' text (content streams, fonts) in pdf_text.h.
//
// Plain C++, host-tested in tools/host_tests/test_dax.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dax.h"

namespace pdf {

constexpr int kMaxObjects = 1024;
constexpr int kMaxPages = 128;
constexpr uint32_t kInStream = 0x80000000u;

struct Doc {
    uint32_t offset[kMaxObjects] = {};     // 0 = not in use; kInStream | stream << 12 | index:
                                           // the index-th object of object stream `stream`
    int      objects = 0;
    int      root = 0;                     // the catalog's object number
    char     id[33] = {};                  // the trailer's first /ID string, hex (lower case)
    int      size = 0;                     // the trailer's /Size (the largest seen)
    // Objects from kMaxObjects on (the Steam / SNEG journals, typeset with
    // tagged structure: 2,000 - 11,400 objects): the caller's table, not
    // owned (open_large) - none: those objects can't be read
    uint32_t* more = nullptr;
    int      more_n = 0;
    // A 32 KB inflate window the caller lends for reading object streams
    // (pdf_text's reader: one window, not a second); none - each read
    // takes its own while it runs
    uint8_t* window = nullptr;
};

struct Image {
    int      width = 0, height = 0;
    bool     jpeg = false;                 // /DCTDecode
    uint32_t data_at = 0, data_len = 0;
    bool     no_memory = false;            // page_image failed for want of memory
    int      images = 0;                   // the page's pictures (its /XObject dictionary's)
};

// Reads the cross-reference table and trailer. False if it isn't a PDF this
// can read.
bool open(dax::ByteSource& src, Doc& doc);
// The same with a table for the objects past kMaxObjects when the file has
// them: (doc.size - kMaxObjects) x 4 bytes from malloc, *more (the
// caller frees it when done with doc; nullptr when not needed). False also
// when that table couldn't be had (*no_memory set).
bool open_large(dax::ByteSource& src, Doc& doc, uint32_t** more, bool* no_memory = nullptr);
// Where object num is (Doc::offset's form; 0 = none)
uint32_t slot(const Doc& doc, int num);

// The page objects in reading order; returns how many (at most max).
int pages(dax::ByteSource& src, const Doc& doc, int* page_obj, int max);

// An image the page uses (its /Resources /XObject): the index-th in the
// dictionary's order (0: the first - a scanned page's one picture; the
// Treasures journal has several a page). Needs two ~8 KB blocks while it
// runs (out.no_memory when they weren't there).
bool page_image(dax::ByteSource& src, const Doc& doc, int page_obj, Image& out, int index = 0);

// A page's pictures as the page shows them (the Treasures journal: a book
// spread a PDF page, each book page a picture, the heading strips small
// ones): each picture's index (page_image's), size and where the page puts
// its left edge (the content stream's last "cm" before its "Do"; 0 when
// unknown), left to right. Returns how many (at most max; 0: the page
// couldn't be read). Needs ~8 KB twice and a 32 KB window while it runs.
struct Picture {
    int index = 0, width = 0, height = 0;
    int x = 0;
};
constexpr int kMaxPictures = 16;
int page_pictures(dax::ByteSource& src, const Doc& doc, int page_obj, Picture* out, int max);

} // namespace pdf
