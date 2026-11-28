// The cluebooks' maps (Tom, 2026-11-27: "the Cluebooks contain maps! Accurate maps with legends and markers"; the
// Companion shows a map's own picture with the party's arrow on it - an Enhancement of the PortMaster port and the
// Android app, whose screens can show it). Each edition of a cluebook the engine knows has a table of where each of
// its maps is (cluebook_tables.cpp, made by tools/cluebook from the checker's results): positions only. From the
// player's own PDF the scan cuts each map's picture, straightened (the scans curve near the binding - Tom), and
// reads the area's name printed above it.
//
// Plain C++, host-tested in tools/host_tests/test_dax.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dax.h"
#include "games.h"

namespace cluebook {

// A map in a book
struct Map {
    uint8_t geo;            // its GEO file's number (GEO3.DAX: 3; 0: GEO.DAX)
    uint8_t block;          // its block there
    uint8_t page;           // the PDF's page, from 1
    // The grid as drawn: the page picture's x / y as fractions of its width / height, a polynomial of degree 3 in
    // (u / 16, v / 16) - u, v the grid's own coordinates, 0 - 16, a square a unit. Terms u^i v^j, i + j <= 3, i
    // the outer: 1, v, v^2, v^3, u, uv, uv^2, u^2, u^2v, u^3
    float   cx[10], cy[10];
    float   name[4];        // the box its name is printed in: x0, x1, y0, y1 in points from the page's foot, left
    // The name as printed, where the book's text layer misreads it (its OCR - as the journals' fixes); "": none
    // shown; nullptr: read from the book
    const char* fix;
};

struct Book {
    games::Game game;
    uint16_t    pages;              // the edition's page count
    float       aspect;             // its first map page's width / height (editions of a page count told apart)
    const Map*  maps;
    int         n;
};

extern const Book kBooks[];
extern const int kBookCount;

// The edition of that game's cluebook with that many pages whose first map page is that shape (aspect < 0: the
// first with that many pages); nullptr: none known
const Book* find(games::Game game, int pages, float aspect = -1);

// A point of the grid on the page picture (fractions of its width / height)
void grid_point(const Map& m, float u, float v, float* x, float* y);

// The straightened picture: kSquare(...) pixels a square, half a square round the grid
constexpr float kMargin = 0.5f;

// What the scan makes, handed out as it goes
struct Sink {
    virtual ~Sink() = default;
    // A map's picture: w x h RGB888, square pixels a square, kMargin squares round the grid. False: stop.
    virtual bool picture(const Map& m, int square, int w, int h, const uint8_t* rgb) = 0;
    // The area's name as the book prints it, tidied (its letter "J. " gone, title case)
    virtual void name(const Map& m, const char* text) = 0;
    // n of total maps done
    virtual void progress(int n, int total) { (void)n; (void)total; }
};

// The maps of the book in src (a PDF). The maps made; -1: not a known edition of that game's cluebook (or not a
// PDF this reads), -2: no memory.
int make(dax::ByteSource& src, games::Game game, Sink& out);

// A name as the Companion shows it: "J. NERAKA CITY & BASE" -> "Neraka City & Base" (into out, cap bytes)
void tidy_name(const char* in, char* out, size_t cap);

} // namespace cluebook
