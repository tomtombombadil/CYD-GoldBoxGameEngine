// The Adventurer's Journal. The games say "record it in journal entry 31"
// or "you overhear tavern tale 12"; the printed journal (GOG: a PDF of
// scanned pages, in the game's folder) has the text. The board makes the
// entries' pictures itself from the player's own PDF during the card scan
// (Tom, 2026-10-09), using a table of where each entry is on the pages
// (journal_tables.cpp: page numbers and positions only - nothing from the
// journal is in the firmware), and shows entry N when the game mentions it.
//
// JOURNAL.DAT (in /GOLDBOX/_CYD/<game folder>/, little-endian):
//   "GBJ2"                          written last: a half-made file has none
//   u8 version (1), u8 revision, u16 entries, u16 pieces, u16 maps
//                                   revision 1 (v0.150.0): light greys kept
//                                   (paper only from 232 up); maps: entries
//                                   made from the Companion's map pictures
//   char pdf_id[32]                 the PDF's /ID (which PDF it came from)
//   u8 palette[256][3]              RGB
//   entries x { u8 kind ('J' journal entry, 'T' tavern tale, 'I' the
//               introduction, 'P' Pool's proclamations by their numerals'
//               values), u8 0,
//               u16 number, u16 first piece, u16 pieces }
//   pieces x  { u16 w, u16 h, u32 offset }  a rectangle of a page, at the
//             scan's resolution, w x h bytes (palette indexes), top to bottom;
//             version 2: w 0 = text, h bytes (ASCII, '\n' between
//             paragraphs) - the Steam / SNEG releases' Gold Box Companion
//             has each entry's text (Tom, 2026-10-10: text, not pictures)
// An entry is its pieces stacked (narrower ones centred).
//
// Plain C++, host-tested in tools/host_tests/test_dax.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dax.h"

namespace journal {

// ---- the tables (journal_tables.cpp, made by tools/journal/make_table.py) --

struct Piece {
    uint8_t  page;                 // PDF page, from 1
    uint16_t x, y, w, h;           // in pixels of the page's picture
    uint8_t  image = 0;            // which of the page's pictures (pdf::page_image's index; Treasures)
};

struct EntryDef {
    char     kind;                 // 'J' / 'T' / 'I' (the introduction, number 1) / 'P' (a proclamation)
    uint8_t  number;
    uint16_t first;                // into the pieces
    uint8_t  count;
    uint8_t  flags = 0;            // kMap: a map (checked by eye; the viewer's Turn key)
};
constexpr uint8_t kMap = 1;

struct Table {
    const char*     name;          // for the scan log
    uint32_t        pdf_size;      // the PDF file
    const char*     pdf_id;        // its trailer /ID, lower-case hex
    const EntryDef* entries;
    int             n_entries;
    const Piece*    pieces;
    int             n_pieces;
};

// The table for a PDF of this size and /ID, or nullptr.
const Table* find_table(uint32_t pdf_size, const char* pdf_id);
// By the /ID alone (what JOURNAL.DAT keeps), or nullptr
const Table* find_table(const char* pdf_id);
// Whether the table marks that entry as a map
bool is_map(const Table& t, char kind, int number);
extern const Table* const kTables[];
extern const int kTableCount;

// ---- making JOURNAL.DAT ---------------------------------------------------

class Output {
public:
    virtual ~Output() = default;
    virtual bool write_at(uint32_t pos, const uint8_t* data, size_t n) = 0;
};

// Pages done so far of all the pages it needs
using Progress = void (*)(int done, int total, void* ctx);

struct GbcPictures;

// Reads the PDF's page pictures and writes JOURNAL.DAT. False on a PDF it
// can't read (or no memory); the file is then left without its "GBJ2".
// maps (optional): the Gold Box Companion's pictures (JE_NNN.jpg) - an
// entry the table marks as a map is made from its picture when there is
// one (the Companion's are often sharper than the PDF's scans).
bool make(dax::ByteSource& pdf, const Table& t, Output& out, Progress progress, void* ctx,
          GbcPictures* maps = nullptr);
constexpr int kRevision = 1;

// The palette (index -> RGB) and the colour a scanned pixel is stored as:
// paper and the faint print from the other side of the page become white
void palette_rgb(int index, uint8_t* r, uint8_t* g, uint8_t* b);
uint8_t index_of(int r, int g, int b);
constexpr uint8_t kPaper = 215;

// Levels for showing a picture piece: the darkest ink the piece really
// has (from a histogram of its palette indexes) is drawn black and the
// rest stretched up to the paper - the 72 dpi scans' grey print (the
// Treasures journal) gets its contrast back. 0: no change.
int black_point(const uint32_t hist[256]);
void level_rgb(int index, int black, uint8_t* r, uint8_t* g, uint8_t* b);

// ---- the book's columns -------------------------------------------------------
// Where a scanned page's columns of print are (Tom, 2026-10-24: "divide
// them into columns, two per page, just like the text"): the page (scaled
// down) cut into `bands` strips across; clear[x] = in how many of them
// column x has no print. The columns are the runs of print between the
// gutters (clear in most strips: a picture or heading across a gutter in a
// few doesn't close it), as [x0, x1) - narrow runs join their neighbours,
// the margins are left out. Returns how many (1 at least when n > 0; at
// most max).
constexpr int kColumnBands = 16;
// clear[] from each strip's print: ink[b * stride + x] = how much print
// column x has in strip b (band_h rows). Print: more than an eighth of
// that strip's busy columns (its 80th percentile; print is fainter on some
// pages than others) and more than a speck a row. Of the strips with
// print near a column, the share where the column itself has none, out
// of `bands` (n at most 512).
void clear_strips(const uint16_t* ink, int stride, int n, int bands, int band_h, uint8_t* clear);
int find_columns(const uint8_t* clear, int n, int bands, int* x0, int* x1, int max);

// ---- reading it -------------------------------------------------------------

struct Info {
    int      version = 0, revision = 0, maps = 0;
    int      entries = 0, pieces = 0;
    char     pdf_id[33] = {};
    uint8_t  palette[256][3] = {};
};

struct PieceInfo {
    int      w = 0, h = 0;             // text: w 0, h its length
    uint32_t offset = 0;
    bool text() const { return w == 0; }
};

bool read_info(dax::ByteSource& src, Info& out);
// The entry's pieces: first index and count. False if it isn't there.
bool find(dax::ByteSource& src, const Info& info, char kind, int number, int* first, int* count);
bool piece(dax::ByteSource& src, const Info& info, int i, PieceInfo& out);
// A picture piece's black point (black_point of up to 32 of its rows,
// evenly spread; row: a buffer of the piece's width)
int piece_black(dax::ByteSource& src, const PieceInfo& p, uint8_t* row);

// ---- the Steam / SNEG releases: the Gold Box Companion's journal ------------
// GBC/Games/<nn. Title>/Game.dat (the player's own copy) holds each entry's
// text as "<journal N>" ... "</journal>" (plain text, lines broken at ~70
// columns, a blank line between paragraphs); the picture entries say
// "(map)" there (the first, long one "(this is a long text ...)") and are
// JE_NNN.jpg beside it.

struct GbcEntry {
    int      number = 0;
    uint32_t at = 0, len = 0;          // the text between the tags, in Game.dat
    bool     picture = false;          // a short "( ... )" note: its JE_NNN.jpg is the entry
};
constexpr int kMaxGbcEntries = 160;

// The entries in Game.dat (read in pieces); how many (at most max)
int gbc_entries(dax::ByteSource& dat, GbcEntry* out, int max);

// The entry's text, tidied: CR LF to LF, the lines of a paragraph joined
// with spaces, one '\n' between paragraphs, no leading / trailing blanks.
// Returns the length (at most cap - 1; out is 0-terminated).
size_t gbc_text(dax::ByteSource& dat, const GbcEntry& e, char* out, size_t cap);

// The entry's picture note in tidied text: the first paragraph that is a
// short "( ... )" - "(map)", "(picture)" (Pool of Radiance: some entries
// have text, the picture, then more text). a / b: where it starts / ends
// (b past it). False: none.
bool gbc_marker(const char* t, size_t len, size_t* a, size_t* b);

// Opens JE_NNN.jpg for entry `number` (false: none); the source stays
// valid until the next call
struct GbcPictures {
    virtual ~GbcPictures() = default;
    virtual dax::ByteSource* open(int number) = 0;
    virtual void close() {}                // the last one opened (a file slot back)
};

// A section of a typeset journal's text (make_book): what it is and where
// its text is (in the text file make_book wrote them to)
struct Section {
    char     kind = 'J';                   // as Mention: 'J', 'T', 'P', 'I'
    uint16_t number = 0;
    uint32_t at = 0, len = 0;
};
constexpr int kMaxSections = 400;

// Sections for make_gbc to add: items[0, n) and their text file (opened
// when make_gbc copies them, closed after - the card's file slots)
struct ExtraText {
    virtual ~ExtraText() = default;
    virtual dax::ByteSource* open() = 0;
    virtual void close() = 0;
    const Section* items = nullptr;
    int n = 0;
};

// Writes JOURNAL.DAT (version 2) from Game.dat: text pieces for the text
// entries; an entry with a picture note whose JE file is there gets its
// text before the note, the picture (the whole JPEG at its own size) and
// its text after (up to 3 pieces); without the JE file the note stays in
// the text. id: what read_info's pdf_id gets (to know the file is
// ready on the next scan).
// extra (optional): the PDF's own sections - each a text entry, unless
// Game.dat has that journal entry (an empty dat: the sections only).
bool make_gbc(dax::ByteSource& dat, GbcPictures& pics, const char* id, Output& out, Progress progress, void* ctx,
              ExtraText* extra = nullptr);

// ---- typeset journals: the Steam / SNEG PDFs' text -----------------------------
// Their PDFs are books typeset in InDesign (no page pictures): the scan
// reads the text from the player's PDF (pdf_text.h) and keeps it as
// BOOK.TXT in the game's _CYD folder - the book view shows it as text
// (Tom, 2026-10-31: the book wanted on the board) - and finds the
// numbered sections in it ("Tale 12:", "Proclamation CXIV", "Journal Entry
// 7", "Tavern Tale 3", "Introduction") for the entries Game.dat lacks.
//
// BOOK.TXT (little-endian):
//   "GBB1", u8 version (1), u8 0, u16 pages
//   u32 offset[pages + 1]          page i's text: offset[i] .. offset[i + 1]
//   the pages' text                ASCII, paragraphs with '\n' between
// The sections' text goes to a second file (sect): each section's
// paragraphs ('\n' between), at Section::at.
//
// How a page's lines become paragraphs, and a section's end: the lines
// come in the content's order (each story's text in reading order); a
// line goes on its paragraph when it's the same size, in the same column
// within about a line and a half below, or at the top of the next column
// / page when the paragraph's last line was at the bottom of its column;
// an indented line starts one. A section: its heading's paragraph, then
// the lines of the page's body size that go on from its last one (other
// sizes - pictures' captions, a page's heading - passed over); it ends at
// the next heading or where the text doesn't go on.
struct BookInfo {
    int  pages = 0;                        // pages read
    int  failed = 0;                       // pages that couldn't be read
    int  sections = 0;                     // into the caller's list
    int  count[4] = {};                    // 'J', 'T', 'P', 'I' found
    uint32_t chars = 0;                    // the book's text
    bool no_memory = false;
};
bool make_book(dax::ByteSource& pdf, Output& book, Output& sect, Section* sections, int max, BookInfo& info,
               Progress progress, void* ctx);
constexpr int kBookHeader = 8;
// A BOOK.TXT's pages (0: not one)
int book_pages(dax::ByteSource& book);
// Page p's (from 1) text: where and how long. False: not there.
bool book_page(dax::ByteSource& book, int p, uint32_t* at, uint32_t* len);

// A line of the book that starts a section: its kind and number (tests;
// make_book's own check). "Journal Entry 12:", "JOURNAL ENTRY 12", "Tale
// 3:", "Tavern Tale 3", "Tavern Tale #3", "Proclamation CXIV",
// "Introduction".
bool section_heading(const char* line, int len, char* kind, int* number);
// A Roman numeral's value (n letters at s), or -1
int roman_number(const char* s, int n);

// ---- the game's text ----------------------------------------------------------

// Looks through printed game text for the journal's references, in order:
//   'J' "JOURNAL ENTRY 31", "JOURNAL AS ENTRY 59", "JOURNAL\nENTRY 44",
//       "ENTRY #12", JOURNAL a few words before ENTRY in the same sentence
//       ("... INTO YOUR JOURNAL CALLING IT ENTRY 10"), "ENTRY 55 IN YOUR
//       JOURNAL", a list ("UNDER ENTRIES 23 AND 14") and "(ENTRY 33)" (Pool:
//       papers listed one a line after "... INTO YOUR JOURNAL:")
//   'T' "TAVERN TALE 12" / "TAVERN TALE #12" / "TAVERN TALE NUMBER 12"
//   'P' Pool's City Hall: "PROCLAMATION CXIV", "PROCLAMATIONS LXIV, CIX, AND
//       LIX" - the Roman numerals' values (the journal's "Proclamation <N>")
//   'I' "THE INTRODUCTION TO YOUR JOURNAL" (Gateway's and Treasures' starts: 1)
// Up to max into out; returns how many.
struct Mention {
    char kind;
    int  number;
};
int find_mentions(const char* text, Mention* out, int max);
// The last complete mention's kind (else 0) and *number.
char find_mention(const char* text, int* number);

// A mention's name as the viewer says it: "Journal Entry 5", "Tavern Tale
// 3", "Proclamation CXIV", "Introduction"
void mention_name(char kind, int number, char* out, size_t cap);

} // namespace journal
