// Scroll bundles: Secret of the Silver Blades' "Bundle of N Scrolls"
// (secret_facts_2.md 29.items.4; its GAME.OVR, read 2026-11-28): Join on a
// scroll ties the character's scrolls into one item of type 0x49, up to 10
// scrolls; the scrolls hang off it (in the original a chain of records
// through the 4 bytes after the 63, saved as the bundle's record followed by
// its scrolls' - the .STF, VAULT.DAT). Its Use lists the spells of every
// scroll in it ("in Bundle"), a cast spell goes off the first scroll that
// holds it, Halve splits it, Id refuses it.
//
// In the other games the Scroll Bundles Enhancement (rules::kEnhBundles)
// gives them the same: there a bundle is the first scroll's record marked
// in its name bytes (0x00-0x09, which the games never show - they make the
// name from the words) and saved the same way, the bundle's record then its
// scrolls' (an original game reading such a file sees an empty scroll and
// the scrolls themselves).
//
// The engine keeps a bundle's scrolls here, by an id kept in the bundle's
// record (bytes 0x08-0x09): the item record moves (Trade, Drop, Deposit,
// the ground) and its scrolls with it. The scrolls are whole 63-byte
// records, on the heap only while a bundle exists; gc() lets go of those no
// item points at any more.
//
// Plain C++ (no Arduino), host-tested in tools/host_tests/test_dax.cpp.
#pragma once

#include <cstddef>
#include <cstdint>

#include "dax.h"
#include "items.h"

namespace bundle {

constexpr int kMax = 10;                // "Bundles are limited to 10 scrolls."
constexpr uint8_t kSecretType = 0x49;   // Secret's bundle item type (its ITEMS row: anyone may ready it)

// Which kind of bundles the game has: Secret's own (type 0x49 records) or the
// marked records of the Enhancement - on (Join makes them, they work) or off
// (None: kept as they are, shown as the scroll they were made from, their
// scrolls kept for the files). Set as a game opens and when the switch
// changes; the store stays.
enum class Kind : uint8_t { None, Secret, Marked };
void configure(bool secret, bool enhancement_on);
Kind kind();
inline bool on() { return kind() != Kind::None; }

// Is this item record a bundle that works (in the game's way)?
bool is(const uint8_t* r);
// A bundle record in the game's own way, whether bundles are on or not
// (what the files keep)
bool stored(const uint8_t* r);
// The Enhancement's mark (bytes 0x00-0x04)
bool marked(const uint8_t* r);
// The scrolls in it (its store's count; 0 for one the store doesn't know)
int count(const uint8_t* r);
// Its k-th scroll's record (0 = the first in the bundle), or nullptr
uint8_t* scroll(const uint8_t* r, int k);

// ---- Changing bundles
// Turns the scroll record `r` into a bundle holding a copy of itself (Secret:
// type 0x49, words 0x27 "Scroll" / count / 0x4D "Bundle of", unreadied,
// identified, no effects, count 1, value kept - GAME.OVR 0x29495); the
// Enhancement's: the record kept, marked. false: no room in the store.
bool make(uint8_t* r);
// A scroll record added at the end (count 0x39 + 1, value + its value;
// Secret also its words 0x30 and weight 0x37 = the count). false: full.
bool add(uint8_t* r, const uint8_t* scroll_rec);
// The k-th scroll taken out of the bundle (moved to `out` when given) - the
// store's list and count 0x39 only (the caller keeps the words / weight /
// value as its original does)
void take(uint8_t* r, int k, uint8_t* out = nullptr);
// A new bundle record `out` holding the bundle's first `n` scrolls (they
// leave `r`); false: no room. Secret's Halve (GAME.OVR 0x29089) - see the .cpp
bool split(uint8_t* r, int n, uint8_t* out);
// A new store entry for a copy of a bundle record (the copy gets its own
// scrolls - the record copied by memcpy would share them)
bool clone(uint8_t* copy);

// A bundle record read from a file: a new, empty entry for it (its scrolls
// then given one by one with add); false: no room (the record keeps no id)
bool start(uint8_t* r);

// ---- The store
void clear();
// Lets go of every bundle whose id `live` doesn't name (live(id) false)
void gc(bool (*live)(uint16_t id, void* ctx), void* ctx);
uint16_t id_of(const uint8_t* r);
size_t bytes_used();                     // (the memory log)

// ---- Files: a bundle's record is followed by its scrolls' records (as
// many as its count 0x39)
// How many records the bundle at `r` takes in a file (1 + its scrolls)
int file_records(const uint8_t* r);
// Writes the bundle's record as the file holds it (Secret: its name bytes
// and id cleared; the Enhancement's: the mark kept, the id cleared) into
// `out` (stride bytes, the rest zero)
void file_record(const uint8_t* r, uint8_t* out, size_t stride);
// After a bundle's record was read into `r`: its `n` scrolls' records read
// from src at `at` (stride bytes each) into a new store entry. Returns the
// bytes the scrolls took (n x stride) - the reader skips those either way.
uint32_t read_scrolls(uint8_t* r, dax::ByteSource& src, uint32_t at, uint32_t stride);
// The game reading the files is Secret (type 0x49 records are bundles)
bool secret();

// ---- Spells on the bundle's scrolls (secret_facts_2.md 29.items.4)
// Takes spell `s` off the first scroll in the bundle that has it (GAME.OVR
// 0x32ACA): the slot cleared and the scroll's spell-count word (count_at)
// down by one; a scroll left with none (the word under one_spell) leaves the
// bundle - count 0x39 down (Secret: its words / weight / value as they were),
// and a bundle left with one scroll becomes that scroll. Returns true when
// the bundle is now a plain scroll.
bool remove_spell(uint8_t* r, int s, uint8_t count_at, uint8_t one_spell);

}  // namespace bundle
