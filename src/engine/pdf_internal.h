// pdf.cpp's object reader, shared with pdf_text.cpp (the typeset journals'
// text). Not for other files: use pdf.h / pdf_text.h.
#pragma once

#include <cstddef>
#include <cstdint>

#include "inflate.h"
#include "pdf.h"

namespace pdf {
namespace detail {

constexpr size_t kObjBuf = 4096;
constexpr int kNodes = 256;

enum class T : uint8_t { None, Null, Bool, Num, Name, Str, Ref, Array, Dict, Keyword };

struct Node {
    T        t = T::None;
    int32_t  v = 0;            // number, ref object number, bool; arrays: how many values
    uint16_t s = 0, n = 0;     // span in the buffer (names, strings, keywords; arrays: from '[' to past ']')
    int16_t  first = -1;       // arrays and dicts: first child (dicts: key, value, key, ...)
    int16_t  next = -1;        // next sibling
};
// (a long array of numbers - a font's /Widths - keeps its numbers as nodes
// only while there's room: past that only its span and count; read the
// span's text, as pdf_text.cpp does)

// One object (or the trailer) read into a buffer and parsed
struct Obj {
    char     buf[kObjBuf];
    size_t   len = 0;
    uint32_t file_at = 0;      // where buf starts in the file
    Node     node[kNodes];
    int      nn = 0;
    int      root = -1;        // the object's value
    uint32_t stream_at = 0;    // data after "stream" (0 = none)
};

bool is_ws(char c);
bool is_delim(char c);
bool name_is(const Obj& o, int node, const char* s);
int get(const Obj& o, int dict, const char* key);
// Object num into o
bool load(dax::ByteSource& src, const Doc& doc, int num, Obj& o);
// A value that may be a reference: the object it points to, loaded into
// tmp; (*ro, *rn) the one to use
bool deref(dax::ByteSource& src, const Doc& doc, const Obj& o, int node, Obj& tmp, const Obj** ro, int* rn);
// The stream of the object in o (loaded by load) through out: FlateDecode
// (window: inflate::kWindow bytes) or no filter. tmp: for a /Length that
// is a reference. False: not a stream this reads, or out stopped it.
bool stream(dax::ByteSource& src, const Doc& doc, const Obj& o, Obj& tmp, inflate::Output& out, uint8_t* window);

} // namespace detail
} // namespace pdf
