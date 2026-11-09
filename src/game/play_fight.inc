// Combat (Curse first): the Play Test's front end for engine/combat - part
// of play.cpp (included inside its anonymous namespace: it uses the Play
// Test's screen, words and files).
//
// LOAD MONSTER reads MON<area>CHA (the record), SPC (effects) and ITM
// (items) of the monster, its CPIC<area> icon; COMBAT then builds the
// battlefield (indoors from the 3D map, outdoors a random field), places
// both sides and plays the rounds: initiative, each fighter's turn - the
// player's menu "Move View Aim Use Cast Turn Quick Done" (Move: tap a
// square next to the fighter - or the pad - to step or attack that way,
// the menu line to stop; Aim: "Next Prev Manual Target Center Exit", or tap
// someone on the field; Done: "Guard Delay Quit Bandage Speed Exit"), the
// computer's turns for monsters and Quick fighters; attacks, damage,
// dying and bleeding, the round's end. Then "The party has won." with the
// experience, and the treasure ("View Take Pool Share Exit").
//
// The screen (as the games draw it): the combat frame, a 7 x 7 view of the
// field (24 x 24 squares from pixel 8, 8), the fighter whose turn it is on
// the right (name, "Hitpoints", "AC", the readied weapon; messages from
// row 10), row 23 for "Range = N", the menu on row 24. Colours 0 and 8 are
// swapped while fighting (the field's black shows dark grey).

constexpr int kViewX = 8, kViewY = 8, kSq = 24;

// The monsters loaded: their groups each a block of its own, made as a
// LOAD MONSTER needs one (v0.136.1: 8 fixed, 507 bytes each, took 4 KB
// in every fight - most have one or two kinds)
struct MonsterLoad {
    combat::Monster* m = nullptr;
    int n = 0, cap = 0;
    // Darkness's Monster Summoning reserve (R10): its monsters from here on,
    // their group, its icon loaded (at the first summons)
    int reserve_from = -1, reserve_group = -1;
    bool reserve_icon = false;
    combat::Group* g[combat::kMaxGroups] = {};
    int groups = 0;
    ~MonsterLoad()
    {
        for (combat::Group*& p : g) {
            delete p;
            p = nullptr;
        }
    }
};
MonsterLoad* ml = nullptr;

bool in_arena(const void* p);
void fight_clear_monsters()
{
    if (!ml) return;
    for (int k = 0; k < ml->n; ++k) {
        if (in_arena(ml->m[k].items)) ml->m[k].items = nullptr;     // (the arena's, not the heap's)
        ml->m[k].release();
    }
    free(ml->m);
    delete ml;
    ml = nullptr;
}

// Reads DAX block `block` of a file (raw bytes), up to cap; its size or -1
int read_block(const char* file, int block, uint8_t* out, int cap)
{
    plat::File f;
    if (!open_dax(file, f)) return -1;
    plat::FileSource src(f);
    int n = -1;
    const dax::Entry* e = d->idx.find(static_cast<uint8_t>(block));
    if (e) {
        dax::RleReader r(src, d->idx, *e);
        const uint32_t want = e->raw_size < static_cast<uint32_t>(cap) ? e->raw_size : static_cast<uint32_t>(cap);
        n = static_cast<int>(r.read(out, want));
    }
    f.close();
    return n;
}

// A character / monster record (MON<area>CHA block) into rec, Curse's
// layout (Pool's 285-byte records moved into it)
bool read_record_block(const char* file, int block, uint8_t* rec, uint8_t* ext = nullptr, uint8_t* sx = nullptr)
{
    if (dkk()) {
        // Death Knights' 216-byte records with their spell part, the MON<n>WIZ
        // block of the same number when there is one (party::from_dkk - an
        // NPC keeps its extension; a monster its flags 2 (0x7B) and magic
        // resistance (0xC1): sx)
        uint8_t* k = static_cast<uint8_t*>(malloc(party::kDkkRecordSize + party::kDkkSpellSize));
        const bool ok = k && read_block(file, block, k, party::kDkkRecordSize) == static_cast<int>(party::kDkkRecordSize);
        uint8_t* w = nullptr;
        if (ok) {
            char wiz[24];
            snprintf(wiz, sizeof wiz, "%s", file);
            char* at = strstr(wiz, "CHA.");
            if (at) memcpy(at, "WIZ.", 4);
            if (at && read_block(wiz, block, k + party::kDkkRecordSize, party::kDkkSpellSize) ==
                          static_cast<int>(party::kDkkSpellSize))
                w = k + party::kDkkRecordSize;
        }
        if (ok && sx) {
            sx[0] = k[0x7B];
            sx[1] = k[0xC1];
        }
        if (ok) {
            party::Character* tmp = ext ? new (std::nothrow) party::Character : nullptr;
            if (tmp) {
                party::from_dkk(k, w, *tmp);
                memcpy(rec, tmp->rec, party::kRecordSize);
                memcpy(ext, tmp->ext, party::kExtSize);
                delete tmp;
            } else {
                party::from_dkk(k, w, rec);
            }
        }
        free(k);
        return ok;
    }
    if (secret_files()) {
        // Secret's 439-byte records (party::from_secret; an NPC keeps its
        // extension; a monster its flags 2 and magic resistance: sx)
        uint8_t* sec = static_cast<uint8_t*>(malloc(party::kSecretRecordSize));
        const bool ok = sec && read_block(file, block, sec, party::kSecretRecordSize) ==
                                   static_cast<int>(party::kSecretRecordSize);
        if (ok && sx) {
            sx[0] = sec[0x15E];
            sx[1] = sec[0x1A5];
        }
        if (ok) {
            if (ext) {
                party::Character* tmp = new (std::nothrow) party::Character;
                if (tmp) {
                    party::from_secret(sec, *tmp);
                    memcpy(rec, tmp->rec, party::kRecordSize);
                    memcpy(ext, tmp->ext, party::kExtSize);
                    delete tmp;
                } else {
                    party::from_secret(sec, rec);
                }
            } else {
                party::from_secret(sec, rec);
            }
        }
        free(sec);
        return ok;
    }
    if (darkness_files()) {
        // Darkness's 510-byte records (party::from_darkness - an NPC keeps
        // its extension; a monster its flags 2 (0x1A5) and magic resistance
        // (0x1EB): sx)
        uint8_t* dk = static_cast<uint8_t*>(malloc(party::kDarknessRecordSize));
        const bool ok = dk && read_block(file, block, dk, party::kDarknessRecordSize) ==
                                  static_cast<int>(party::kDarknessRecordSize);
        if (ok && sx) {
            sx[0] = dk[0x1A5];
            sx[1] = dk[0x1EB];
        }
        if (ok) {
            party::Character* tmp = ext ? new (std::nothrow) party::Character : nullptr;
            if (tmp) {
                party::from_darkness(dk, *tmp);
                memcpy(rec, tmp->rec, party::kRecordSize);
                memcpy(ext, tmp->ext, party::kExtSize);
                delete tmp;
            } else {
                party::from_darkness(dk, rec);
            }
        }
        free(dk);
        return ok;
    }
    if (krynn()) {
        // Champions' 409-byte records (party::from_krynn - an NPC keeps its
        // extension; a monster its flags 2 (0x140) and magic resistance
        // (0x187): sx)
        uint8_t* k = static_cast<uint8_t*>(malloc(party::kKrynnRecordSize));
        const bool ok = k && read_block(file, block, k, party::kKrynnRecordSize) ==
                                 static_cast<int>(party::kKrynnRecordSize);
        if (ok && sx) {
            sx[0] = k[0x140];
            sx[1] = k[0x187];
        }
        if (ok) {
            party::Character* tmp = ext ? new (std::nothrow) party::Character : nullptr;
            if (tmp) {
                party::from_krynn(k, *tmp);
                memcpy(rec, tmp->rec, party::kRecordSize);
                memcpy(ext, tmp->ext, party::kExtSize);
                delete tmp;
            } else {
                party::from_krynn(k, rec);
            }
        }
        free(k);
        return ok;
    }
    if (!pool_files()) return read_block(file, block, rec, party::kRecordSize) == static_cast<int>(party::kRecordSize);
    uint8_t pool[party::kPoolRecordSize];
    if (read_block(file, block, pool, sizeof pool) != static_cast<int>(sizeof pool)) return false;
    party::from_pool(pool, rec);
    return true;
}

// A monster's items (MON<area>ITM block): Curse's 63-byte records, or
// Darkness's 17-byte ones - an item's bytes 0x2E-0x3E (type, words, plus
// ... value, effects; darkness_facts.md 31), the rest of the record zero
// (the name is made from its words). How many (up to cap) into out.
int read_monster_items(const char* file, int block, uint8_t* out, int cap)
{
    if (!darkness_files() || dkk()) {          // (Death Knights: 63-byte items, as Curse's)
        const int n = read_block(file, block, out, cap * static_cast<int>(items::kRecordSize));
        return n > 0 ? n / static_cast<int>(items::kRecordSize) : 0;
    }
    constexpr int kRow = 17;
    uint8_t* rows = static_cast<uint8_t*>(malloc(static_cast<size_t>(cap) * kRow));
    const int n = rows ? read_block(file, block, rows, cap * kRow) : -1;
    const int k = n > 0 ? n / kRow : 0;
    for (int i = 0; i < k; ++i) {
        uint8_t* it = out + i * items::kRecordSize;
        memset(it, 0, items::kRecordSize);
        memcpy(it + 0x2E, rows + i * kRow, kRow);
    }
    free(rows);
    return k;
}

// Pool's NPC as its load makes it (START ovr18 +0xEC4 / +0x1262, ovr24 +0):
// a character's fight icon - size 1 for dwarves, gnomes and halflings, 2
// for elves, half-elves, half-orcs and humans; body, arm and leg colours
// and shield, weapon a d6 each as (d + 8) x 16 + d, the hair 0xC4 - a
// random number (0xAB; Random(256)), the View portrait: a body by one of
// its classes picked by a die (cleric / magic-user / thief / else the
// fighter's row, a d5) and a head (a d8) from the program's tables
// (variant.npc_look: heads 16 bytes, then bodies 10 each - cleric,
// fighter, magic-user, thief; [sex * n + die]), then its class values
// worked out (THAC0, saves, slots - the file's are its own)
__attribute__((noinline)) void pool_npc_look(party::Character& ch)
{
    uint8_t* r = ch.rec;
    const int race = r[0x74];
    if (race == 1 || race == 3 || race == 5) r[0x144] = 1;
    else if (race == 2 || race == 4 || race == 6 || race == 7) r[0x144] = 2;
    for (int k = 1; k <= 6; ++k) {
        if (k == 4) {
            r[0x144 + k] = 0xC4;
            continue;
        }
        const int dd = rng.roll(6, 1);
        r[0x144 + k] = static_cast<uint8_t>((dd + 8) * 16 + dd);
    }
    r[0x126] = static_cast<uint8_t>(rng.roll(256, 1) - 1);
    const uint16_t look = d->prof->variant.npc_look;
    int cls[8], n = 0;
    for (int i = 0; i <= 6; ++i)
        if (static_cast<int8_t>(r[0x109 + i]) > 0) cls[++n] = i;
    if (n == 0) cls[n = 1] = 2;
    const int pick = cls[rng.roll(n, 1)];
    const int sex = r[0x119];
    const int body_row = pick == 0 ? 16 : pick == 5 ? 36 : pick == 6 ? 46 : 26;
    const int b = rng.roll(5, 1), h = rng.roll(8, 1);
    uint8_t t;
    if (look && program_ds(static_cast<uint16_t>(look + body_row + sex * 5 + b), &t, 1)) r[0x140] = t;
    if (look && program_ds(static_cast<uint16_t>(look + sex * 8 + h), &t, 1)) r[0x13F] = t;
    MagicRules* m = nullptr;
    if (load_rules(m)) classes::class_bonuses(ch, m->tables);
    delete m;
    if (names) rules::recalc(ch, *names, d->facts);
}

// ADD NPC: MON<area> block `id` (its record, effects, items) joins the party
// at its end and is selected; its fight icon is CPIC<area> block `id` (Pool:
// its own character icon - pool_npc_look)
bool npc_join(int id)
{
    if (pt->count >= party::kMaxParty) return false;
    party::Character& ch = pt->m[pt->count];
    ch = party::Character{};
    char file[24];
    area_fmt(file, sizeof file, "MON%dCHA.DAX");
    if (!read_record_block(file, id, ch.rec, ch.ext)) {
        plat::log("[play] ADD NPC: %s #%d not there\n", file, id);
        return false;
    }
    area_fmt(file, sizeof file, "MON%dITM.DAX");
    ch.n_items = read_monster_items(file, id, &ch.items[0][0], party::kMaxItems);
    area_fmt(file, sizeof file, "MON%dSPC.DAX");
    const int na = read_block(file, id, &ch.affects[0][0], sizeof ch.affects);
    ch.n_affects = na > 0 ? na / static_cast<int>(party::kAffectSize) : 0;
    ch.rec[0x126] = static_cast<uint8_t>(id);
    if (pool_files()) pool_npc_look(ch);
    // The lowest icon slot no one else has
    for (int slot = 0; slot < 8; ++slot) {
        bool used = false;
        for (int i = 0; i < pt->count; ++i)
            if (pt->m[i].rec[0x143] == slot) used = true;
        if (!used) {
            ch.rec[0x143] = static_cast<uint8_t>(slot);
            break;
        }
    }
    pt->selected = pt->count++;
    char nm[20];
    ch.name(nm, sizeof nm);
    plat::log("[play] ADD NPC %d: %s joins\n", id, nm);
    return true;
}

void fight_load_monster(int id, int copies, int icon, bool reserve)
{
    if (!ml) ml = new (std::nothrow) MonsterLoad;
    if (!ml || ml->groups >= combat::kMaxGroups || ml->n >= combat::kMaxMonsters) return;
    // The group's block first: the record goes straight into it (its copies'
    // base); a load that stops keeps the block for the next one
    if (!ml->g[ml->groups]) ml->g[ml->groups] = new (std::nothrow) combat::Group;
    if (!ml->g[ml->groups]) {
        plat::log("[play] LOAD MONSTER: not enough memory\n");
        return;
    }
    combat::Group& g = *ml->g[ml->groups];
    new (&g) combat::Group();           // (in place: it's a big block for the stack)
    uint8_t* rec = g.rec;
    uint8_t sx[2] = {};
    uint8_t aff[combat::kMonsterAffects * party::kAffectSize];
    int n_aff = 0;
    if (hl()) {
        // (Dark Queen: MONCHA.GLB's monster - its record, effects and items in one)
        int ni = 0;
        if (!q_load_monster(id, rec, sx, &g.item[0][0], &ni, combat::kGroupItems, aff, &n_aff, combat::kMonsterAffects)) {
            plat::log("[play] MONCHA.GLB #%d: no monster\n", id);
            return;
        }
        g.n_items = static_cast<uint8_t>(ni);
    }
    // MON<area>CHA.DAX
    char cha[24];
    area_fmt(cha, sizeof cha, "MON%dCHA.DAX");
    if (!hl() && !read_record_block(cha, id, rec, nullptr, sx)) {
        plat::log("[play] %s #%d: no monster\n", cha, id);
        return;
    }
    // Secret's Level (GAME.OVR 0x101A): the other side's (record 0x197 = 1)
    // hit points and maximum x (level + 2) / 4 - Novice half, Squire three
    // quarters, Veteran as they are, Adept 5/4, Champion 3/2 - kept in a
    // byte as the original's; both 1 when either comes to 0
    // Darkness (GAME.OVR 0x621): the same with its difficulty 1-5 (game
    // byte 0x27) as (v + 1) / 4 - 3 as they are - and a LOAD MONSTER with
    // 48 or more loaded already loads nothing (0x5E1; copies stop at 63)
    if (darkness_files() && ml->n >= 48 && !reserve) return;
    const int level = secret_files() ? vm->get(0x4CF4) : darkness_files() ? (vm->get(0x4CF4) & 0xFF) - 1 : 2;
    if (rec[0x197] == 1 && level >= 0 && level < 5 && level != 2 && !reserve) {
        rec[0x1A4] = static_cast<uint8_t>(rec[0x1A4] * (level + 2) / 4);
        rec[0x78] = static_cast<uint8_t>(rec[0x78] * (level + 2) / 4);
        if (rec[0x1A4] == 0 || rec[0x78] == 0) rec[0x1A4] = rec[0x78] = 1;
    }
    g.id = static_cast<uint8_t>(id);
    g.icon = static_cast<uint8_t>(icon);
    if (!hl()) {
        char itm[24];
        area_fmt(itm, sizeof itm, "MON%dITM.DAX");
        g.n_items = static_cast<uint8_t>(read_monster_items(itm, id, &g.item[0][0], combat::kGroupItems));
        char spc[24];
        area_fmt(spc, sizeof spc, "MON%dSPC.DAX");
        const int na = read_block(spc, id, aff, sizeof aff);
        n_aff = na > 0 ? na / static_cast<int>(party::kAffectSize) : 0;
    }
    for (int k = 0; k < g.n_items; ++k) name_from_words(g.item[k]);
    if (copies < 1) copies = 1;
    if (ml->n + copies > combat::kMaxMonsters) copies = combat::kMaxMonsters - ml->n;
    if (ml->n + copies > ml->cap) {
        const int cap = ml->n + copies;
        void* p = realloc(ml->m, static_cast<size_t>(cap) * sizeof(combat::Monster));
        if (!p) {
            plat::log("[play] LOAD MONSTER: not enough memory\n");
            return;
        }
        ml->m = static_cast<combat::Monster*>(p);
        ml->cap = cap;
    }
    // Each copy: the group's record, its own effects
    int made = 0;
    for (int k = 0; k < copies; ++k) {
        combat::Monster& m = ml->m[ml->n];
        new (&m) combat::Monster();
        m.base = g.rec;
        m.flags2 = sx[0];
        m.magic_res = sx[1];
        if (n_aff && !m.room_for_aff(n_aff)) {
            plat::log("[play] LOAD MONSTER: not enough memory\n");
            break;
        }
        m.n_aff = n_aff;
        if (n_aff) memcpy(m.aff, aff, static_cast<size_t>(n_aff) * party::kAffectSize);
        m.group = static_cast<uint8_t>(ml->groups);
        ++ml->n;
        ++made;
    }
    if (!made) return;
    ++ml->groups;
    plat::log("[play] %s %d x%d (icon %d): %d monsters\n", reserve ? "the summoning reserve" : "LOAD MONSTER", id, copies,
              icon, ml->n);
}

// The arena (CALL 0x8000 / 0x8001 - Pool START.EXE ovr8 +0x1A6F, Curse
// ovr8 +0x1BA0, the same code): everyone whose name isn't the selected
// member's is out of the next fight (in-combat 0); a duel (0x8000) adds a
// copy of the selected at the list's end - "ROLF", Quick, the monsters'
// side, control 0xB2, no effects, the next monster icon slot with CPIC
// block 11, and its items copied one by one onto the front of its list
// (so the other way round)
void fight_duel(bool rolf)
{
    if (!pt->count || pt->selected < 0 || pt->selected >= pt->count) return;
    const party::Character& sel = pt->m[pt->selected];
    const int len = sel.rec[0] < 15 ? sel.rec[0] : 15;
    for (int i = 0; i < pt->count; ++i) {
        const uint8_t* r = pt->m[i].rec;
        if (r[0] != sel.rec[0] || memcmp(r + 1, sel.rec + 1, static_cast<size_t>(len)) != 0) pt->m[i].rec[0x196] = 0;
    }
    plat::log("[play] the arena: member %d alone%s\n", pt->selected, rolf ? " against ROLF" : "");
    if (!rolf) return;
    if (!ml) ml = new (std::nothrow) MonsterLoad;
    if (!ml || ml->groups >= combat::kMaxGroups || ml->n >= combat::kMaxMonsters) return;
    if (!ml->g[ml->groups]) ml->g[ml->groups] = new (std::nothrow) combat::Group;
    if (!ml->g[ml->groups]) {
        plat::log("[play] the duel: not enough memory\n");
        return;
    }
    combat::Group& g = *ml->g[ml->groups];
    new (&g) combat::Group();
    memcpy(g.rec, sel.rec, sizeof g.rec);
    memset(g.rec, 0, 16);
    g.rec[0] = 4;
    memcpy(g.rec + 1, "ROLF", 4);
    g.rec[0x196] = 1;
    g.rec[0x143] = static_cast<uint8_t>(8 + ml->groups);
    g.icon = 11;
    if (ml->n + 1 > ml->cap) {
        void* p = realloc(ml->m, static_cast<size_t>(ml->n + 1) * sizeof(combat::Monster));
        if (!p) {
            plat::log("[play] the duel: not enough memory\n");
            return;
        }
        ml->m = static_cast<combat::Monster*>(p);
        ml->cap = ml->n + 1;
    }
    combat::Monster& m = ml->m[ml->n];
    new (&m) combat::Monster();
    m.base = g.rec;
    m.group = static_cast<uint8_t>(ml->groups);
    m.flags2 = sel.ext[party::kExtFlags2];
    m.magic_res = sel.ext[party::kExtMagicRes];
    // Its own items (all of them - a group holds 8), the other way round
    if (sel.n_items > 0) {
        auto* list = static_cast<uint8_t(*)[items::kRecordSize]>(malloc(static_cast<size_t>(sel.n_items) * items::kRecordSize));
        if (list) {
            for (int k = 0; k < sel.n_items; ++k) memcpy(list[k], sel.items[sel.n_items - 1 - k], items::kRecordSize);
            m.items = list;
            m.n_items = static_cast<uint8_t>(sel.n_items);
        }
    }
    // (its differences from the selected's record are its own changes: the
    // fight's start works its values out again with its own items)
    m.set(0x198, 1);                    // Quick
    m.set(0x197, 1);                    // the other side
    m.set(0xF7, 0xB2);                  // control
    ++ml->n;
    ++ml->groups;
}

// The sides' to-hit bonuses (round 11: each game's attacker-against-target
// roll - tools/devkit/tools/tohit.py finds it): d20 + the attacker's to-hit
// + its side's bonus >= the AC needed. The bonus: the scripts' bytes - Curse's
// area words 0x7F71 (the party) / 0x7F70 (the others; Death Knights the same
// bytes of its byte map, GAME.OVR 0x30D2E); Darkness / Treasures / the Dark
// Queen their game block's 0x2F / 0x01 (GAME.OVR 0x32A43, 0x3C6D6; DQK.EXE
// 0x84DD). The party's then the difficulty's: Darkness-type 1 +2, 2 +1,
// 4 -1, 5 -2 (game byte 0x27); Gateway's 0-4 (word 0x4CF4, GAME.OVR 0x3D5D8)
// 0 +2 ... 4 -2. Secret's (GAME.OVR 0x36E88) and Death Knights' (0x30D42)
// compare a constant, never their difficulty - none there; the Game Fixes
// give theirs (SPEC 12). PoolRad, Curse, Champions: no difficulty.
void side_bonuses(combat::Battle& b)
{
    int level = -1;                             // 0-4, 2 the plain one
    if (dk_ui() && !dkk()) {
        b.to_hit_party = static_cast<int8_t>(vm->get(0x2F));
        b.to_hit_monsters = static_cast<int8_t>(vm->get(0x01));
        level = (vm->get(0x27) & 0xFF) - 1;
    } else {
        b.to_hit_party = static_cast<int8_t>(vm->get(0x7F71));
        b.to_hit_monsters = static_cast<int8_t>(vm->get(0x7F70));
        if (gateway()) level = vm->get(0x4CF4);
        else if (rules::fixes() && (secret_files() || dkk())) level = vm->get(0x4CF4) - (dkk() ? 1 : 0);
    }
    static constexpr int8_t kLevel[5] = {2, 1, 0, -1, -2};
    if (level >= 0 && level < 5) b.to_hit_party += kLevel[level];
}

// Darkness's Monster Summoning reserve (R10; GAME.OVR 0x7D2B, at every
// COMBAT): 10 monsters of the MON block the frame style (block +0x39)
// picks, out of the fight (status 9) until a Monster Summoning brings them
// in; no difficulty for them (the loader alone)
void load_reserve(const combat::Tables& t)
{
    if (!ml || !d->prof->fight.tables.reserve || ml->groups >= combat::kMaxGroups) return;
    const int style = vm->get(0x3A) & 0xFF;
    if (style > 4 || !t.reserve_mon[style]) return;
    const int from = ml->n, group = ml->groups;
    fight_load_monster(t.reserve_mon[style], 10, t.reserve_pic[style], true);
    if (ml->groups == group) return;
    ml->reserve_from = from;
    ml->reserve_group = group;
    uint8_t* r = ml->g[group]->rec;
    r[0x195] = 9;
    r[0x196] = 0;
    r[0xF7] |= 0x32;
}

// The k-th loaded monster's own record (LOAD CHARACTER past the party: the
// scripts can change one before the fight - the Thieves' Guild hideout makes
// four of the loaded thieves fight on the party's side); nullptr past them
// (the one taken gets a whole record of its own - its group's copies share one)
uint8_t* fight_monster_record(int k)
{
    return ml && k >= 0 && k < ml->n ? ml->m[k].whole() : nullptr;
}

// ---- The fight's arena (v0.136.1): memory a fight borrows instead of
// asking the heap. While the field is up the script waits for the fight's
// result and runs none of its code, so its code (GameState::code, 7.5 KB)
// waits on the card (_CYD/CODE.TMP) and holds the fighters' blocks and
// pictures; the 3D view's memory past the field's 40 tiles (parked with
// the walls, ~2 KB) holds pictures. The arena only grows (a picture let go
// leaves its room unused); what doesn't fit goes on the heap as before.
// The pictures go at the fight's art's end (end_fight_art), the fighters
// with the fight, then the code comes back - before the script goes on.
void* arena_take(size_t n, bool code_only);
bool in_arena(const void* p);
void arena_free(void* p);
void fight_arena_end();
void monster_items_off_arena();
combat::Fighter* make_fighters(int n);
void release_fighters(combat::Fighter* f);

// ---- Pictures, 4 bits a pixel (a VGA game in 256 colours: a byte a
// pixel, its values - px8)
struct Pic4 {
    uint8_t w = 0, h = 0;
    uint8_t* px[2] = {};                // ready, attack: rows of rowb(w) bytes
    // A 16-colour game's character icon with a colour recoloured to 0: its
    // see-through plane after the rows (pic4_mask) - the originals make the
    // plane from the pictures as loaded and recolour after it, so a part
    // made colour 0 stays drawn (Pool's Dirten: all his colours 0)
    bool own_mask = false;
};

inline bool px8() { return vga != nullptr; }
inline int rowb(int w) { return px8() ? w : w / 2; }
inline int nib(const uint8_t* p, int w, int x, int y)
{
    if (px8()) return p[y * w + x];
    const uint8_t b = p[y * (w / 2) + x / 2];
    return x & 1 ? b & 15 : b >> 4;
}
inline void set_nib(uint8_t* p, int w, int x, int y, int v)
{
    if (px8()) {
        p[y * w + x] = static_cast<uint8_t>(v);
        return;
    }
    uint8_t& b = p[y * (w / 2) + x / 2];
    b = static_cast<uint8_t>(x & 1 ? (b & 0xF0) | (v & 15) : (b & 0x0F) | (v << 4));
}

void free_pic(Pic4& p)
{
    arena_free(p.px[0]);
    arena_free(p.px[1]);
    p = Pic4{};
}

// Darkness's fight pictures are VGA blocks (darkness_facts.md 7 4.7): each
// pixel in its palette entry's EGA colour (its EGA mode), values under the
// block's first entry EGA colours themselves; the value 8 is see-through
// (the original's mask colour for CPIC / COMSPR / CHEAD / CBODY - GAME.OVR
// 0x3FFA4: their loads pass 8), kept as a bit plane after the rows (1 =
// see-through) since its pictures use every colour, black and dark grey
// included. The rows' see-through pixels are 0 (the head is OR'd onto the
// body as the original's EGA path does - GAME.OVR 0x10FC5).
bool fight_masks() { return d->prof->variant.vga || d->prof->variant.hlib; }      // (Dark Queen's parts: their own see-through)
constexpr int kMaskColour = 8;
uint32_t mask_bytes(int w, int h) { return (static_cast<uint32_t>(w) * h + 7) / 8; }
const uint8_t* mask_of(const uint8_t* p, int w, int h) { return fight_masks() && p ? p + rowb(w) * h : nullptr; }
// A picture's see-through plane: Darkness's / the Dark Queen's, or an icon's own (Pic4::own_mask)
const uint8_t* pic4_mask(const Pic4& ic, const uint8_t* p)
{
    if (fight_masks()) return mask_of(p, ic.w, ic.h);
    return ic.own_mask && p ? p + rowb(ic.w) * ic.h : nullptr;
}
inline bool masked(const uint8_t* m, int w, int x, int y)
{
    const int k = y * w + x;
    return (m[k >> 3] >> (k & 7)) & 1;
}

// A masked picture made w2 x h2 (the Dark Queen's attack frames may be a row
// shorter than the ready one - CPIC 209 is 24 x 23 to 81's 24 x 24 - and a
// Pic4 holds one size): its pixels where both have them, see-through
// elsewhere. p is freed; nullptr when there's no memory.
uint8_t* fit_masked(uint8_t* p, int w, int h, int w2, int h2)
{
    if (!p || (w == w2 && h == h2)) return p;
    const uint32_t nb = static_cast<uint32_t>(rowb(w2)) * h2;
    uint8_t* out = static_cast<uint8_t*>(calloc(1, nb + mask_bytes(w2, h2)));
    if (out) {
        const uint8_t* m = p + rowb(w) * h;
        uint8_t* m2 = out + nb;
        for (int y = 0; y < h2; ++y)
            for (int x = 0; x < w2; ++x) {
                const bool in = x < w && y < h;
                if (in) set_nib(out, w2, x, y, nib(p, w, x, y));
                if (!in || masked(m, w, x, y)) {
                    const int k = y * w2 + x;
                    m2[k >> 3] = static_cast<uint8_t>(m2[k >> 3] | 1 << (k & 7));
                }
            }
    }
    free(p);
    return out;
}

uint8_t* load_vga4(dax::RleReader& r, uint32_t raw_size, int frame, int* w, int* h)
{
    uint8_t hdr[pic::kVgaHeaderSize];
    pic::VgaHeader vh;
    if (r.read(hdr, sizeof hdr) != sizeof hdr || !pic::parse_vga_header(hdr, raw_size, vh) || frame >= vh.frames)
        return nullptr;
    uint8_t map[256];
    for (int i = 0; i < 256; ++i) map[i] = static_cast<uint8_t>(i & 15);
    // (a VGA screen: the values, the block's palette entries sent - the
    // fight's kinds keep apart: ground 32-63, COMSPR 64-95, the party
    // 96-143, the monsters 144-255; darkness_facts.md 7 4.7)
    pic::VgaPalette* pal = vga_load();
    if (pal ? !pic::read_vga_palette(r, vh, pal->rgb) : r.skip(vh.count * 3u) != vh.count * 3u) return nullptr;
    if (pal) {
        pal->first = vh.first;
        pal->count = vh.count;
        vga_take();
    }
    if (!pic::read_vga_ega(r, vh, map)) return nullptr;
    if (px8())
        for (int i = 0; i < 256; ++i) map[i] = static_cast<uint8_t>(i);
    const uint32_t at = 10u + vh.count * 3u + (vh.count + 1u) / 2u;
    const uint32_t skip = vh.pixels_at + static_cast<uint32_t>(frame) * vh.frame_bytes() - at;
    if (r.skip(skip) != skip) return nullptr;
    const int pw = vh.width_px(), ph = vh.height;
    const uint32_t nb = static_cast<uint32_t>(rowb(pw)) * ph;
    uint8_t* out = static_cast<uint8_t*>(calloc(1, nb + mask_bytes(pw, ph)));
    if (!out) return nullptr;
    uint8_t* m = out + nb;
    for (int y = 0; y < ph; ++y)
        for (int x = 0; x < pw; ++x) {
            const int v = r.next();
            if (v < 0) {
                free(out);
                return nullptr;
            }
            if (v == kMaskColour) {
                const int k = y * pw + x;
                m[k >> 3] = static_cast<uint8_t>(m[k >> 3] | 1 << (k & 7));
            } else {
                set_nib(out, pw, x, y, map[v]);
            }
        }
    *w = pw;
    *h = ph;
    return out;
}

// Frame `frame` of a picture block into 4-bit rows (malloc'd); w, h
// (Darkness: a VGA block, its see-through plane after the rows)
uint8_t* load_pic4(const char* file, int block, int frame, int* w, int* h)
{
    if (hl()) return frame ? nullptr : q_load_pic4(file, block, w, h);
    plat::File f;
    if (!open_dax(file, f)) return nullptr;
    plat::FileSource src(f);
    uint8_t* out = nullptr;
    const dax::Entry* e = d->idx.find(static_cast<uint8_t>(block));
    if (e && d->prof->variant.vga) {
        dax::RleReader r(src, d->idx, *e);
        out = load_vga4(r, e->raw_size, frame, w, h);
    } else if (e) {
        dax::RleReader r(src, d->idx, *e);
        uint8_t hdr[pic::kHeaderSize];
        pic::Header ph;
        if (r.read(hdr, sizeof hdr) == sizeof hdr && pic::parse_header(hdr, e->raw_size, ph) && frame < ph.frames) {
            const uint32_t fb = ph.frame_bytes();
            out = static_cast<uint8_t*>(malloc(fb));
            bool ok = out != nullptr;
            ok = ok && pic::read_frame(r, ph, frame, out);
            if (!ok) {
                free(out);
                out = nullptr;
            } else {
                *w = ph.width_px();
                *h = ph.height;
            }
        }
    }
    f.close();
    return out;
}

// Draws a 4-bit picture clipped to (cx0, cy0)-(cx1, cy1): mirrored, colour 0 see-through
bool blit_swap = false;             // Gateway's fight: its pictures' 0 and 8 swapped as they're drawn

void blit4(pic::Canvas& c, const uint8_t* p, int w, int h, int x, int y, bool mirror, bool see_through, int cx0,
           int cy0, int cx1, int cy1, const uint8_t* own_mk = nullptr)
{
    if (!p) return;
    // (Darkness: its see-through plane; own_mk: an icon's own, Pic4::own_mask)
    const uint8_t* mk = !see_through ? nullptr : own_mk ? own_mk : mask_of(p, w, h);
    for (int j = 0; j < h; ++j) {
        const int yy = y + j;
        if (yy < cy0 || yy >= cy1) continue;
        for (int i = 0; i < w; ++i) {
            const int xx = x + i;
            if (xx < cx0 || xx >= cx1) continue;
            const int px = mirror ? w - 1 - i : i;
            const int v = nib(p, w, px, j);
            if (see_through && (mk ? masked(mk, w, px, j) : v == 0)) continue;
            c.set(xx, yy, static_cast<uint8_t>(blit_swap && (v == 0 || v == 8) ? v ^ 8 : v));
        }
    }
}

// ---- The fight's state (on the heap while fighting)
enum class FSt : uint8_t {
    Begin, Turn, Menu, Move, Aim, DoneMenu, Speed, Ai, Pages, RoundEnd, Results, Destroyed, AllyAsk, FleeAsk,
    ContinueAsk, Next, UseAim, AbortAsk, RoundRest, Clouds, Resume, FlameAsk, FlameAbort, OldAge,
    NpcShare,                           // Pool: the NPCs' share page, the results page next
    More                                // the Dark Queen: a page's box full - a key, then the rest (res 0xCEEA)
};

// A page's pictures: a missile or spell flying, a burst / sparkles on a
// fighter (the games' magic hits), a fallen one's skull blinking; each with
// the sound it starts with (a sound alone: kind None)
struct Fx {
    // Redraw: the field again (a cloud laid); Bolt: a flight to (x1, y1), then lightning along the
    // fight's bolt segments
    enum Kind : uint8_t { None, Fly, Burst, Skull, Redraw, Bolt } kind = None;
    uint8_t sound = 0;
    combat::Flight fl;                  // Fly / Burst: the pictures, their cycle, ms a step
    uint8_t loops = 1;                  // Burst: times round the cycle
    int8_t  who = -1;                   // Burst / Skull: the fighter
    uint8_t x0 = 0, y0 = 0, x1 = 0, y1 = 0;     // Fly: from, to (field squares)
};

// (tests: the fight's pages, pictures and waits told as they happen - the
// sweep's fx_check.inc: the waits against the originals' rules, the order
// of an attack's frames; `wait` the ms set, `n` a skull frame's squares
// drawn / a sweep's blow number). Board builds have none of it.
enum class FxEv : uint8_t { Page, Frame, PageWait, AttackPause, FieldBack, SweepBlow, Quick, RollsClear };
#ifdef CYD_TEST_HOOKS
void (*g_fx_hook)(FxEv ev, uint32_t wait, int n) = nullptr;
#define CYD_TEST_FX(ev, wait, n)                                                 \
    do {                                                                         \
        if (g_fx_hook) g_fx_hook(FxEv::ev, static_cast<uint32_t>(wait), (n));     \
    } while (0)
#else
#define CYD_TEST_FX(ev, wait, n) \
    do {                         \
    } while (0)
#endif

constexpr int kPages = 24;            // a turn's messages at most (a fireball on a crowd: the rest dropped)

// A page's lines are in the fight's text pool (v0.86.1: 24 x 5 x 48 fixed
// took 5.8 KB; a turn's pages rarely fill a third of the pool)
constexpr int kPageText = 2048;
struct Page {
    uint16_t line_at[5];                // where each line starts in Fight::ptext
    uint8_t colour[5];
    uint8_t weapon = 0;                 // Darkness's Kalistes: its blow's weapon + "+4" in the panel's rows 7-9
    uint8_t names = 0;                  // the lines that are names (bit k: printed as they are, a row each)
    bool    cont = false;               // goes on below the page before (its rows kept), as the originals' fall after a hit
    int8_t  row = 0;                    // its first row when not 10 (the Dark Queen's res 0xB14E rows: "lost a spell", "reflects it!" at 12)
    bool    clears = false;             // its wait clears 10-21 (res 0xB14E's wait: 0xB245) - a page going on after it starts on a blank box
    bool    blank = false;              // its own row on a blank box (the page before it cleared at its end)
    bool    keep_end = false;           // a page after it goes on below the page before it (DQK.EXE 0x25020's fall row)
    int     lines = 0;
    uint8_t sound = 0;                  // the game's sound effect as it shows (0: none)
    Fx      fx[2];                      // then these, one after the other; then the page's wait
};

// The rolls shown (CYD_COMBAT_ROLLS, Tom 2026-10-31): the engine's record
// of the rolls just made, and each page's (at most 2: to hit / a save, then
// damage) - a block of its own only while a fight runs with the switch on
struct RollShow {
    combat::RollLog log;
    combat::Roll page[kPages][2];
};
// (light grey: apart from the game's own words - the Dark Queen's are light
// grey, so cyan there)
uint8_t roll_colour() { return hl() ? 3 : 7; }

constexpr int kTileBytes = kSq * kSq / 2;
constexpr int kTiles = 40;
// A field picture's bytes (a VGA game in 256 colours: a byte a pixel)
inline int tile_bytes() { return px8() ? kSq * kSq : kTileBytes; }

// A fight's state in pieces - the battle, this, each field picture a block
// of its own - so a heap broken up by the area's pictures and a big monster
// load still finds room (Tom, 2026-10-10: the Thieves' Guild ambush, 17
// monsters, ran out on the 2.8")
struct Fight {
    combat::Battle& b;                  // (its own block: make_fight)
    int8_t old[12][3] = {};             // its old-age steps (kind, member, the weeping one)
    int    old_n = 0, old_i = 0;
    // (Treasures in 256 colours: no background picture while the fight
    // lasts - its ovl 0x62 +0xF4 lets it go, the script's COMBAT reads it
    // again after, ov43+0x1643; DOSBox VGA: the fight's panel black)
    explicit Fight(combat::Battle& battle) : b(battle) { tr_background(false); }
    ~Fight()
    {
        tr_background(true);
        for (uint8_t*& t : tile) {
            free(t);
            t = nullptr;
        }
        delete &b;
        // (end_fight_art let the pictures go; any left are let go before the arena goes)
        for (Pic4& p : icon) free_pic(p);
        arena_free(cursor);
        arena_free(skull);
        arena_free(outline);
        release_fighters(fighters);
        fighters = nullptr;
        monster_items_off_arena();
        fight_arena_end();
        combat::g_rolls = nullptr;
        delete rolls;
    }
    Fight(const Fight&) = delete;
    Fight& operator=(const Fight&) = delete;
    combat::Tables t;
    uint8_t* tile[kTiles] = {};         // the field's 40 pictures (each kTileBytes)
    combat::Fighter* fighters = nullptr; // b.f's storage: the party and the monsters loaded
    Pic4     icon[8 + combat::kMaxGroups];
    uint8_t* cursor = nullptr;          // COMSPR 0x19 (ready): the box under the one whose turn it is
    uint8_t* skull = nullptr;           // COMSPR 11 (attack)
    uint8_t* outline = nullptr;         // COMSPR 0x19 (attack): the box round a target being aimed at
    // The fight's words, packed (v0.85.7: 161 x 40 fixed was 6.4 KB): word i
    // at wbuf + woff[i]
    static constexpr int kWordPool = 2400;
    char     wbuf[kWordPool];
    uint16_t woff[profile::kFightWords];
    bool     tiles_in_world = false;    // the field's tiles in the 3D view's memory (parked to the card)
    FSt      st = FSt::Begin;
    FSt      after = FSt::Turn;         // where the pages go on to
    int      cur = -1;
    uint32_t until = 0;
    bool     attack_pause = false;      // an attack's frame drawn: its 100 ms before the pages
    Page     page[kPages];
    int      pages = 0, at = 0;
    char     ptext[kPageText];          // the pages' lines, one after another
    int      ptext_used = 0;
    // Move: where the fighter started (Esc undoes the move)
    int      move_x = 0, move_y = 0, move_left = 0;
    // Aim: the candidates by distance, the one shown
    int      cand[combat::kMaxFighters];
    int      n_cand = 0, ci = 0;
    bool     manual = false;
    int      mx = 0, my = 0;            // Manual's square
    int      pending = -1;              // an ally to attack (asked)
    int      flee_dir = 8;
    // Casting: the spell, the targets picked, how many more, an area's radius (-1: none)
    int      spell = 0;
    int      sp_t[combat::kMaxFighters] = {};
    int      sp_n = 0, sp_need = 0, sp_area = -1;
    int      pick_budget = -1;          // spell aim 5: picked one by one against this (-1: not that kind)
    bool     pick_size = false;         // ... costs by size (Faerie Fire), else by Hit Dice
    bool     jump = false;              // the aim is Treasures' jumping boots' (tr_jump), not a spell
    bool     page_dropped = false;      // the last add_page found no room (its pictures / sound aren't set)
    bool     cont_next = false;         // the next add_page goes on below the last page shown
    int8_t   page_end = 10;             // the row after the last page shown's print (the originals' text row + 1)
    // The Dark Queen's full box (res 0xCD31 +0x1B9): the page's line and
    // character it goes on from after a key, the box's first row
    int8_t   more_line = -1, more_top = 0;
    uint8_t  more_at = 0;
    int8_t   yielding = -1;             // the one whose "Surrenders" is on the pages (leaves the field after them)
    // A sweep under way (melee): its blows one at a time, each one's pages
    // (the hit, the fall, the skull, the icon gone) before the next blow
    int8_t   sweep_by = -1, sweep_t[8] = {};
    uint8_t  sweep_n = 0, sweep_at = 0;
    bool     arena = false;             // an arena fight (CALL 0x8000 / 0x8001): the selected member alone
    // Thrown weapons used up (a dagger, a spear, darts): onto the treasure after the fight
    uint8_t  thrown[6][items::kRecordSize];
    int      n_thrown = 0;
    // Champions: weapons left in a baaz's stone body (back to their owners
    // after the fight - GAME.OVR 0x7B5D) and the bozaks that have exploded
    uint8_t  lost[6][items::kRecordSize];
    int8_t   lost_who[6] = {};
    int      n_lost = 0;
    uint8_t  burst[(combat::kMaxFighters + 7) / 8] = {};
    // Secret's Delayed Blast Fireballs set to go off (secret_facts_2.md 26)
    struct Blast { int8_t caster = -1; uint8_t x = 0, y = 0, area = 0, rounds = 0, pw = 0; };
    Blast    blast[4];
    bool     ai_checked = false;        // this turn's morale and spell choice done
    bool     ai_diced = false;          // (Champions) this turn's first dice rolled - champ_ai_dice
    bool     flee_told = false;         // this turn's morale part done (a page said: its items and spells after it)
    bool     fx_done = false;           // this turn's start effects (turn_effects) done
    bool     continuing = false;        // "Continue Battle": Yes - the party's turns with no enemies left
    int      use_item = -1;             // Use: the item the spell comes from (its charge goes)
    bool     use_scroll = false;        // ... a scroll
    combat::Outcome out;
    int      share = 0;
    bool     treasure_only = false;
    uint8_t  res_word = 0;              // the results page's first word (profile::FightWord)
    // The page's pictures playing (fx_at: the one; 2 = done, the page waits)
    int      fx_at = 2;
    bool     fx_on = false, fx_redraw = false;  // one is running; a skull to clear at the page's end
    Fx       fx;
    combat::FlightPath path;
    uint8_t* fx_pic[2] = {};            // its ready / attack pictures (COMSPR)
    int      fx_step = 0;
    int      bolt_at = -1;              // a Bolt picture: the segment flying (-1: the flight to the square first)
    uint8_t  under[kSq * kSq];          // what the flying picture covers
    int      under_x = 0, under_y = 0;
    bool     has_under = false;
    int      aim_x = -1, aim_y = -1;    // where a spell was aimed (an area's centre)
    RollShow* rolls = nullptr;          // the rolls shown (CYD_COMBAT_ROLLS; nullptr: switched off)
};
Fight* fg = nullptr;

// A new fight's state (nullptr: no memory)
Fight* make_fight()
{
    combat::Battle* b = new (std::nothrow) combat::Battle;
    if (!b) return nullptr;
    Fight* f = new (std::nothrow) Fight(*b);
    if (!f) delete b;
    // The rolls shown (no memory for them: the fight goes on without)
    if (f && CYD_COMBAT_ROLLS && opts.rolls) f->rolls = new (std::nothrow) RollShow;
    combat::g_rolls = f && f->rolls ? &f->rolls->log : nullptr;
    return f;
}

// The field's pictures' blocks (false: no memory - those got are freed)
// The 3D view's walls aren't drawn during a fight: they wait on the card
// (_CYD/WORLD.TMP) and the field's 40 tiles use their memory (v0.85.7,
// Tom's 3.2": 11.5 KB that a fight no longer has to find); back after
// the fight (end_fight_art). False: the card couldn't take them.
static_assert(kTiles * kTileBytes <= sizeof(view3d::World), "the field's tiles must fit in the 3D view's memory");
void world_park_path(char* out, size_t cap) { snprintf(out, cap, "%s/_CYD/WORLD.TMP", games::kRootDir); }
bool world_big = false;             // (Gateway: big wall sets' tiles freed while parked - read again after)
bool park_world()
{
    if (world_parked || !plat::card_ready()) return false;
    char path[96];
    world_park_path(path, sizeof path);
    plat::File f = plat::open(path, "w");
    if (!f) return false;
    // (Darkness's background picture, 3.8 KB, isn't drawn in a fight
    // either: it goes, and reads again when the 3D view is next drawn)
    view3d::back_free(d->world);
    // (Gateway's big wall sets' far tiles go too: read again after the fight)
    bool big = false;
    for (view3d::TileSet& ts : d->world.sets)
        if (ts.extra) {
            view3d::free_extra(ts);
            big = true;
        }
    world_big = big;
    // (Treasures: its wall slots' tiles on the heap go too - read again after)
    if (d->world.tr) view3d::tr_free(d->world);
    const bool ok = f.write(reinterpret_cast<const uint8_t*>(&d->world), sizeof(view3d::World)) == sizeof(view3d::World);
    f.close();
    if (ok) world_parked = true;
    else {
        // (the card didn't take it: the walls stay in memory - what was let
        // go above reads again now, the background when next drawn)
        if (d->world.tr && host) host->slots_done();
        for (int i = 0; big && host && i < 3; ++i)
            if (d->wall_block[i] >= 0 && d->wall_set[i] > 0) host->load_walls(d->wall_set[i], d->wall_block[i]);
        world_big = false;
    }
    return ok;
}
void unpark_world()
{
    if (!world_parked) return;
    char path[96];
    world_park_path(path, sizeof path);
    plat::File f = plat::open(path, "r");
    const bool ok = f && f.read(reinterpret_cast<uint8_t*>(&d->world), sizeof(view3d::World)) == sizeof(view3d::World);
    if (f) f.close();
    world_parked = false;
    if (ok && d->world.tr && host) host->slots_done();     // (Treasures: the slots' tiles again; none when closing)
    if (!ok || world_big) {
        // (the card lost it - or Gateway's big sets' tiles were let go: the
        // walls read again from the game's files)
        if (!ok) log_error("The 3D view's walls couldn't be read back after a fight: reloaded.");
        world_big = false;
        // (none when the Play Test closes mid-fight: the host is gone by then)
        for (int i = 0; host && i < 3; ++i)
            if (d->wall_block[i] >= 0 && d->wall_set[i] > 0) host->load_walls(d->wall_set[i], d->wall_block[i]);
    }
}

// The arena's two parts: [0] the script's code, [1] the 3D view's memory
// past the field's tiles
struct Arena {
    uint8_t* at[2] = {};
    uint32_t cap[2] = {}, used[2] = {};
    bool code_parked = false;
    bool pictures = false;              // pictures may go in (the fight's art is up)
};
Arena arena;

void code_park_path(char* out, size_t cap) { snprintf(out, cap, "%s/_CYD/CODE.TMP", games::kRootDir); }

// The script's code onto the card: its memory the arena's first part
bool park_code()
{
    if (arena.code_parked) return true;
    if (!plat::card_ready()) return false;
    char path[96];
    code_park_path(path, sizeof path);
    plat::File f = plat::open(path, "w");
    if (!f) return false;
    const bool ok = f.write(ecl::code_of(d->gs), d->gs.code_cap) == d->gs.code_cap;
    f.close();
    if (!ok) return false;
    arena.code_parked = true;
    // (from an 8-byte boundary: the code follows GameState's 4-byte
    // code_cap, and the fighters' records hold pointers)
    uint8_t* base = ecl::code_of(d->gs);
    const uint32_t pad = static_cast<uint32_t>((8u - (reinterpret_cast<uintptr_t>(base) & 7u)) & 7u);
    arena.at[0] = base + pad;
    arena.cap[0] = d->gs.code_cap > pad ? d->gs.code_cap - pad : 0;
    arena.used[0] = 0;
    return true;
}

// The 3D view's memory past the tiles (the walls parked): the second part
void arena_world(bool on)
{
    constexpr uint32_t kUsed = (kTiles * kTileBytes + 7u) & ~7u;
    arena.at[1] = on ? reinterpret_cast<uint8_t*>(&d->world) + kUsed : nullptr;
    arena.cap[1] = on ? static_cast<uint32_t>(sizeof(view3d::World)) - kUsed : 0;
    arena.used[1] = 0;
}

void* arena_take(size_t n, bool code_only)
{
    const uint32_t need = static_cast<uint32_t>((n + 7u) & ~static_cast<size_t>(7u));
    if (!code_only && !arena.pictures) return nullptr;
    for (int k = 0; k < (code_only ? 1 : 2); ++k)
        if (arena.at[k] && arena.cap[k] - arena.used[k] >= need) {
            void* p = arena.at[k] + arena.used[k];
            arena.used[k] += need;
            return p;
        }
    return nullptr;
}

bool in_arena(const void* p)
{
    const uint8_t* q = static_cast<const uint8_t*>(p);
    for (int k = 0; k < 2; ++k)
        if (q && arena.at[k] && q >= arena.at[k] && q < arena.at[k] + arena.cap[k]) return true;
    return false;
}

void arena_free(void* p)
{
    if (!in_arena(p)) free(p);
}

// A picture of n bytes (malloc'd) into the arena when there's room
uint8_t* arena_adopt(uint8_t* p, size_t n)
{
    if (!p || in_arena(p)) return p;
    uint8_t* q = static_cast<uint8_t*>(arena_take(n, false));
    if (!q) return p;
    memcpy(q, p, n);
    free(p);
    return q;
}

// A fight picture's bytes (load_pic4's block: 4-bit rows, Darkness's see-through plane after them)
size_t pic_bytes(int w, int h) { return static_cast<size_t>(rowb(w)) * h + (fight_masks() ? mask_bytes(w, h) : 0); }
void arena_adopt(Pic4& ic)
{
    for (uint8_t*& p : ic.px) p = arena_adopt(p, pic_bytes(ic.w, ic.h) + (ic.own_mask ? mask_bytes(ic.w, ic.h) : 0));
}
// A COMSPR picture for the whole fight (the arena when there's room)
uint8_t* fight_pic(int block)
{
    int w = 0, h = 0;
    uint8_t* p = load_pic4("COMSPR.DAX", block, 0, &w, &h);
    return p ? arena_adopt(p, pic_bytes(w, h)) : nullptr;
}

// The monsters still loaded keep their own items past the fight: those in
// the arena go to the heap (no memory: back to the group's list)
void monster_items_off_arena()
{
    for (int k = 0; ml && k < ml->n; ++k) {
        combat::Monster& m = ml->m[k];
        if (!in_arena(m.items)) continue;
        const size_t sz = static_cast<size_t>(m.n_items > 0 ? m.n_items : 1) * items::kRecordSize;
        auto* p = static_cast<uint8_t(*)[items::kRecordSize]>(malloc(sz));
        if (p) memcpy(p, m.items, sz);
        m.items = p;
        if (!p) m.n_items = 0;
    }
}

// The script's code back (the fight is gone: nothing of it is in the arena)
void fight_arena_end()
{
    arena_world(false);
    if (!arena.code_parked) return;
    arena = Arena{};
    char path[96];
    code_park_path(path, sizeof path);
    plat::File f = plat::open(path, "r");
    const bool ok = f && f.read(ecl::code_of(d->gs), d->gs.code_cap) == static_cast<int>(d->gs.code_cap);
    if (f) f.close();
    if (!ok) {
        // (the card lost it: the script reads again from the game's files)
        log_error("The script couldn't be read back after a fight: reloaded.");
        if (host) host->load_script(d->gs.script, ecl::code_of(d->gs), &d->gs.code_len);
    }
}

static_assert(std::is_trivially_destructible<combat::Fighter>::value, "the fighters' blocks are let go without destructors");
combat::Fighter* make_fighters(int n)
{
    void* p = arena_take(static_cast<size_t>(n) * sizeof(combat::Fighter), true);
    if (!p) return new (std::nothrow) combat::Fighter[n];
    combat::Fighter* f = static_cast<combat::Fighter*>(p);
    for (int i = 0; i < n; ++i) new (&f[i]) combat::Fighter();
    return f;
}

void release_fighters(combat::Fighter* f)
{
    if (!in_arena(f)) delete[] f;
}

bool alloc_tiles()
{
    // (a VGA game in 256 colours: its tiles don't fit the 3D view's memory -
    // on the heap; those boards have the PSRAM)
    if (!px8() && !fg->tiles_in_world && park_world()) {
        uint8_t* base = reinterpret_cast<uint8_t*>(&d->world);
        for (int i = 0; i < kTiles; ++i) fg->tile[i] = base + i * kTileBytes;
        fg->tiles_in_world = true;
        arena_world(true);
    }
    if (fg->tiles_in_world) return true;
    for (uint8_t*& t : fg->tile)
        if (!t && !(t = static_cast<uint8_t*>(malloc(tile_bytes())))) {
            for (uint8_t*& u : fg->tile) {
                free(u);
                u = nullptr;
            }
            return false;
        }
    return true;
}

const char* fw(int i) { return fg ? fg->wbuf + fg->woff[i] : ""; }
bool in_fight() { return fg != nullptr; }

void load_fight_words()
{
    // Packed one after another (each read into a 40-byte scratch first);
    // offset 0 is an empty word for those not read or past the pool
    fg->wbuf[0] = 0;
    int at = 1;
    for (int i = 0; i < profile::kFightWords; ++i) fg->woff[i] = 0;
    OvrWords src;
    if (!src.open()) return;
    for (int i = 0; i < profile::kFightWords; ++i) {
        const uint32_t at_word = d->prof->fight.words[i] ? d->prof->fight.words[i] : profile::duel_word(*d->prof, i);
        if (!at_word) continue;
        char t[72];                     // (Darkness's two-row menu: 63)
        t[0] = 0;
        read_word(src, at_word, t, sizeof t);
        if (hl()) {
            // (Dark Queen's words are printf formats here and there: their
            // "%s" / "%d" taken out - the engine puts the values after them)
            char* o = t;
            for (const char* q = t; *q; ++q) {
                if (q[0] == '%' && (q[1] == 's' || q[1] == 'd')) {
                    ++q;
                    continue;
                }
                *o++ = *q;
            }
            *o = 0;
        }
        const int n = static_cast<int>(strlen(t)) + 1;
        if (at + n > Fight::kWordPool) {
            log_error("Fight: the words don't all fit (kWordPool).");
            break;
        }
        memcpy(fg->wbuf + at, t, n);
        fg->woff[i] = static_cast<uint16_t>(at);
        at += n;
    }
}

bool load_fight_tables()
{
    plat::File f;
    if (!open_file(d->prof->program, f)) return false;
    plat::FileSource src(f);
    exepack::Info info;
    struct Ctx {
        plat::FileSource* src;
        exepack::Info* info;
        uint32_t base;
    } ctx{&src, &info, d->prof->create.ds_image};
    bool ok = exepack::parse(src, info) == exepack::Status::Ok &&
              combat::read_tables(
                  fg->t, d->prof->fight.tables,
                  [](void* p, uint16_t ds, uint8_t* out, size_t n) {
                      Ctx* c = static_cast<Ctx*>(p);
                      return exepack::read(*c->src, *c->info, c->base + ds, out, n) == exepack::Status::Ok;
                  },
                  &ctx);
    f.close();
    return ok;
}

// Frames first .. first + n - 1 of a 24 x 24 picture block into the tiles
void load_frames(const char* file, int at, int n)
{
    plat::File f;
    if (!open_dax(file, f)) return;
    plat::FileSource src(f);
    const dax::Entry* e = d->idx.find(1);
    if (e) {
        dax::RleReader r(src, d->idx, *e);
        uint8_t hdr[pic::kHeaderSize];
        pic::Header ph;
        if (r.read(hdr, sizeof hdr) == sizeof hdr && pic::parse_header(hdr, e->raw_size, ph) && ph.width_px() == kSq &&
            ph.height == kSq)
        {
            uint8_t map[16];
            if (pic::begin_frames(r, ph, map))
                for (int k = 0; k < n && k < ph.frames && at + k < 40; ++k)
                    if (!pic::next_frame(r, ph, map, fg->tile[at + k])) break;
        }
    }
    f.close();
}

// Darkness: the field's pictures are a VGA block's frames - DUNGCOM block
// = the look (LOAD FILES' second operand) indoors, WILDCOM block = the
// region (game byte 0x3A when 0-2, else 0) outside (darkness_facts.md S7;
// 26 / 40 frames of 24 x 24, the ground table's pictures)
void load_vga_frames(const char* file, int block)
{
    plat::File f;
    if (!open_dax(file, f)) return;
    plat::FileSource src(f);
    const dax::Entry* e = d->idx.find(static_cast<uint8_t>(block));
    if (e) {
        for (int k = 0; k < kTiles; ++k) {
            dax::RleReader r(src, d->idx, *e);
            int w = 0, h = 0;
            uint8_t* t = load_vga4(r, e->raw_size, k, &w, &h);
            if (!t) break;
            if (w == kSq && h == kSq) memcpy(fg->tile[k], t, tile_bytes());  // (see-through: the colour 0 under it)
            free(t);
        }
    }
    f.close();
}

// The field's pictures: DUNGCOM / WILDCOM (0-33), RANDCOM (34-39)
void load_tiles(bool indoors)
{
    for (uint8_t* t : fg->tile)
        if (t) memset(t, 0, tile_bytes());
    if (hl()) {
        q_load_tiles();
        return;
    }
    if (d->prof->variant.tr) {
        // Treasures (its ovl 0x68 +0xEFA): in 3D and not the overland (game
        // byte 0x3F) DUNGCOM block = game byte 0x51, or wall slot 0's block;
        // else WILDCOM block 0
        const uint8_t* gb = d->gs.area1;
        if (gb[0x21] && !gb[0x3F]) load_vga_frames("DUNGCOM.DAX", gb[0x51] ? gb[0x51] : d->save.look & 0xFF);     // (the slots kept in save.look)
        else load_vga_frames("WILDCOM.DAX", 0);
        return;
    }
    if (d->prof->variant.vga) {
        const int region = vm->get(0x3A) & 0xFF;
        if (indoors) load_vga_frames("DUNGCOM.DAX", d->save.look ? d->save.look : 1);
        else load_vga_frames("WILDCOM.DAX", region <= 2 ? region : 0);
        return;
    }
    load_frames(indoors ? "DUNGCOM.DAX" : "WILDCOM.DAX", 0, indoors ? 25 : 34);
    load_frames("RANDCOM.DAX", 34, 6);
}

// A party member's icon from its record (head 0x141, body 0x142, size
// 0x144, colours 0x145-0x14A; alter_icon_facts.md 1.6): CBODY / CHEAD
// blocks (+ 0x40 large, + 0x80 the attack frame), the head over the
// body's top rows - where both have a colour the two colour numbers OR'd,
// as the program does -, then each pixel recoloured once from its own
// colour: default d -> the byte's low nibble, d + 8 -> its high
// b: a 24 x 24 body picture, made the icon in place; h: the head (hh rows)
void icon_compose(uint8_t* b, const uint8_t* h, int hh, const uint8_t* colours)
{
    static const uint8_t kDefault[6] = {1, 2, 3, 4, 6, 7};
    uint8_t map[16];
    for (int i = 0; i < 16; ++i) map[i] = static_cast<uint8_t>(i);
    for (int i = 0; i < 6; ++i) {
        map[kDefault[i]] = colours[i] & 15;
        map[kDefault[i] + 8] = colours[i] >> 4;
    }
    uint8_t* const bm = fight_masks() ? b + rowb(kSq) * kSq : nullptr;
    if (h && fight_masks() && px8()) {
        // Darkness's VGA path (GAME.OVR 0x10F8F): the head's pixels that
        // aren't see-through replace the body's
        const uint8_t* hm = h + rowb(kSq) * hh;
        for (int y = 0; y < hh && y < kSq; ++y)
            for (int x = 0; x < kSq; ++x) {
                if (masked(hm, kSq, x, y)) continue;
                set_nib(b, kSq, x, y, nib(h, kSq, x, y));
                const int k = y * kSq + x;
                bm[k >> 3] = static_cast<uint8_t>(bm[k >> 3] & ~(1 << (k & 7)));
            }
    } else if (h && fight_masks()) {
        // Darkness's EGA path (GAME.OVR 0x10FC5): the head's colours OR'd
        // onto the body's, see-through where both are
        const uint8_t* hm = h + (kSq / 2) * hh;
        for (int y = 0; y < hh && y < kSq; ++y)
            for (int x = 0; x < kSq; ++x) {
                set_nib(b, kSq, x, y, nib(h, kSq, x, y) | nib(b, kSq, x, y));
                const int k = y * kSq + x;
                if (!masked(hm, kSq, x, y)) bm[k >> 3] = static_cast<uint8_t>(bm[k >> 3] & ~(1 << (k & 7)));
            }
    } else if (h) {
        for (int y = 0; y < hh && y < kSq; ++y)
            for (int x = 0; x < kSq; ++x) {
                const int v = nib(h, kSq, x, y);
                if (v) set_nib(b, kSq, x, y, v | nib(b, kSq, x, y));
            }
    }
    if (px8()) {
        // (VGA, img 0x1C30+0x18B0: the values under 16 through the table, the
        // others as they are; a pixel made the see-through 8 is see-through)
        for (int y = 0; y < kSq; ++y)
            for (int x = 0; x < kSq; ++x) {
                const int k = y * kSq + x;
                const int v = nib(b, kSq, x, y);
                if (v >= 16 || (bm && masked(bm, kSq, x, y))) continue;
                set_nib(b, kSq, x, y, map[v]);
                if (bm && map[v] == kMaskColour) bm[k >> 3] = static_cast<uint8_t>(bm[k >> 3] | 1 << (k & 7));
            }
        return;
    }
    for (int y = 0; y < kSq; ++y)
        for (int x = 0; x < kSq; ++x) set_nib(b, kSq, x, y, map[nib(b, kSq, x, y)]);
}

// ready_only: the ready pose alone (the Companion panel's Party view)
void build_icon(const uint8_t* rec, Pic4& ic, bool ready_only = false)
{
    free_pic(ic);
#if CYD_ICON_GALLERY
    // A Gold Box Companion icon picked for this character (the player's
    // copy of the Companion on the card)
    if (opts.gallery && d->icon_picks.count && !fight_masks()) {
        char nm[16];
        rec_name(rec, nm);
        const int pair = gbc_icons::find(d->icon_picks, nm);
        if (pair > 0) {
            ic.px[0] = load_gbc_icon(2 * pair - 1);
            ic.px[1] = load_gbc_icon(2 * pair);
            if (ic.px[0] && ic.px[1]) {
                ic.w = ic.h = kSq;
                return;
            }
            free_pic(ic);
        }
    }
#endif
    if (hl()) {
        // (Dark Queen: one CBODY picture a character - the record's icon
        // byte, + 128 the attack frame; no head, no recolouring)
        for (int fr = 0; fr < (ready_only ? 1 : 2); ++fr) {
            int bw = 0, bh = 0;
            ic.px[fr] = q_load_pic4("CBODY", rec[0x142] + fr * 128, &bw, &bh);
            if (fr && ic.px[0] && ic.px[1]) ic.px[1] = fit_masked(ic.px[1], bw, bh, ic.w, ic.h);      // (one size)
            else if (ic.px[fr]) ic.w = static_cast<uint8_t>(bw), ic.h = static_cast<uint8_t>(bh);
        }
        return;
    }
    const int head = rec[0x141], body = rec[0x142];
    // (Pool: size 0 - an NPC added from a monster file, as the graveyard's
    // Dirten - draws the small pictures too; ovr18 +0x482 names the file by
    // a table of the size, DS 0x8C5; DOSBox: Valjevo's fights)
    // (the size names the file's last letter, DS 0x8C5: 'T' = the large
    // pictures, anything else - size 0's byte too - the small ones; Pool's
    // DAX loader, ovr33 +0x1C8)
    const bool small = rec[0x144] == 1 || (pool_files() && rec[0x144] == 0);
    // A 16-colour game with a colour recoloured to 0: the icon keeps its
    // own see-through plane, made before the recolouring (the originals'
    // loader makes it from the picture's colour 0, the head's and body's
    // ANDed, ovr18 +0x32F; the recolouring, START 0x649C, leaves it) - a
    // part made colour 0 is drawn, not see-through. Pool's Dirten (all his
    // colours 0): his outline drawn in colour 0 over the wall behind him.
    bool own = false;
    if (!fight_masks() && !px8())
        for (int k = 0; k < 6; ++k)
            if (!(rec[0x145 + k] & 15) || !(rec[0x145 + k] >> 4)) own = true;
    const size_t pb = static_cast<size_t>(rowb(kSq)) * kSq, mb = mask_bytes(kSq, kSq);
    for (int fr = 0; fr < (ready_only ? 1 : 2); ++fr) {
        int bw = 0, bh = 0, hw = 0, hh = 0;
        uint8_t* b = load_pic4("CBODY.DAX", body + (small ? 0 : 0x40) + fr * 0x80, 0, &bw, &bh);
        // (Darkness: one head for both pictures - GAME.OVR 0x3FFC3; its CHEAD has no +0x80 blocks)
        uint8_t* h = load_pic4("CHEAD.DAX", head + (small ? 0 : 0x40) + (fight_masks() ? 0 : fr * 0x80), 0, &hw, &hh);
        if (!b || bw != kSq || bh != kSq) {
            free(b);
            free(h);
            continue;
        }
        if (h && hw != kSq) {
            free(h);
            h = nullptr;
        }
        if (own) {
            uint8_t* b2 = static_cast<uint8_t*>(realloc(b, pb + mb));
            if (!b2) {
                free(b);
                free(h);
                continue;
            }
            b = b2;
            uint8_t* m = b + pb;
            memset(m, 0, mb);
            for (int y = 0; y < kSq; ++y)
                for (int x = 0; x < kSq; ++x)
                    if (!nib(b, kSq, x, y) && !(h && y < hh && nib(h, kSq, x, y))) {
                        const int k = y * kSq + x;
                        m[k >> 3] = static_cast<uint8_t>(m[k >> 3] | 1 << (k & 7));
                    }
        }
        icon_compose(b, h, hh, rec + 0x145);
        free(h);
        ic.px[fr] = b;
        ic.w = ic.h = kSq;
    }
    if (own && (ic.px[0] || ic.px[1])) ic.own_mask = true;
}

// A party member's icon: the head over the body's top rows, recoloured
void load_monster_icon(int slot, int block);

void compose_icon(int slot, const party::Character& ch)
{
    // An NPC from the monsters' files (ADD NPC: computer-run, its MON block
    // at 0x126): its own picture. (A player character's 0x126 is a random
    // number and head / body 0 are ordinary choices - v0.40.3: the GOG
    // sample party fought without icons.)
    // (Pool: an NPC is drawn as a character - its CHEAD / CBODY icon, ovr18
    // +0x482; 0x126 is a random number there)
    if (!pool_files() && ch.rec[0xF7] >= 0x80 && ch.rec[0x126] && !ch.rec[0x141] && !ch.rec[0x142]) {
        load_monster_icon(slot, ch.rec[0x126]);
        if (fg->icon[slot].px[0]) return;
    }
    build_icon(ch.rec, fg->icon[slot]);
    arena_adopt(fg->icon[slot]);
}

void load_monster_icon(int slot, int block)
{
    Pic4& ic = fg->icon[slot];
    free_pic(ic);
    char file[16];
    area_fmt(file, sizeof file, "CPIC%d.DAX");
    for (int fr = 0; fr < 2; ++fr) {
        int w, h;
        uint8_t* p = load_pic4(file, block + fr * 0x80, 0, &w, &h);
        if (!p) continue;
        if (fr && ic.px[0] && fight_masks() && (w != ic.w || h != ic.h)) {
            // (the attack frame made the ready one's size: a Pic4 has one -
            // the Dark Queen's Sivak 24 x 23 / 24 x 24 drew a black row)
            p = fit_masked(p, w, h, ic.w, ic.h);
            if (!p) continue;
            w = ic.w;
            h = ic.h;
        }
        ic.px[fr] = arena_adopt(p, pic_bytes(w, h));
        ic.w = static_cast<uint8_t>(w);
        ic.h = static_cast<uint8_t>(h);
    }
}

void end_fight_art()
{
    if (!fg) return;
    // (the pictures first: some are in the 3D view's memory)
    arena.pictures = false;
    for (Pic4& p : fg->icon) free_pic(p);
    arena_free(fg->cursor);
    arena_free(fg->skull);
    arena_free(fg->outline);
    fg->cursor = fg->skull = fg->outline = nullptr;
    free(fg->fx_pic[0]);
    free(fg->fx_pic[1]);
    fg->fx_pic[0] = fg->fx_pic[1] = nullptr;
    arena_world(false);
    for (uint8_t*& t : fg->tile) {
        if (!fg->tiles_in_world) free(t);
        t = nullptr;
    }
    if (fg->tiles_in_world) {
        fg->tiles_in_world = false;
        unpark_world();
    }
    // (the magic tables a fight loaded aren't needed for walking: they read
    // again for the treasure's Detect or the camp - Tom's 4.0" log: 10 KB
    // stayed after each fight)
    end_magic();
    sound_hold(false);
    memlog::note("fight over");
}

// ---- Drawing
// The results' "press <enter>/<return> to continue" (Gateway's lit as a
// menu's one choice - DOSBox)
void press_enter_line(pic::Canvas& c)
{
    clear_menu_line(c);
    if (gateway()) {
        const char* t = fw(profile::kPressEnter);
        c.fill(0, text::kMenuRow * 8, static_cast<int>(strlen(t)) * 8, 8, 15);
        font::draw_text(c, d->font, t, 0, text::kMenuRow, 0, -1);
        return;
    }
    put(c, fw(profile::kPressEnter), 0, text::kMenuRow, 15);
}

int fight_palette_on = 0;          // play::fight_colours()

void palette_fight(bool on, bool editor = false)
{
    fight_palette_on = !on ? 0 : editor && d->prof->variant.eight_black ? 2
                     : d->prof->variant.fight_unswapped || d->prof->variant.field_swap ? 0 : 1;
    // (Gateway: the field's pictures swap 0 and 8, the panel beside them
    // stays black - DOSBox)
    blit_swap = on && !editor && d->prof->variant.field_swap;
    dirty(0, pic::kScreenH);
}

// (the Dark Queen's: the party's grey, the other side's green - EGA 2, DOSBox)
uint8_t name_colour(const combat::Fighter& f)
{
    if (hl() && f.up()) return f.team() ? 2 : 7;
    return !f.up() ? 12 : f.team() ? 14 : 11;
}

void fighter_name(const combat::Fighter& f, char* out, size_t cap)
{
    char nm[16];
    f.rec.read(reinterpret_cast<uint8_t*>(nm), 1, 15);
    const int len = f.rec[0] < 15 ? f.rec[0] : 15;
    snprintf(out, cap, "%.*s", len, nm);
}

void draw_square(pic::Canvas& c, int x, int y);

// A fighter's icon. The attack pose goes on the field's ground: the
// fighter's squares drawn again first (the original's 0x74572 before its
// attack icon - the ready pose and the turn's box under it go; Tom's board:
// both poses at once). `bare`: the ready pose on the ground alone (no turn
// box - the original's 0x75356 before "Surrenders")
void draw_icon(pic::Canvas& c, int i, bool attack = false, bool bare = false)
{
    const combat::Fighter& f = fg->b.f[i];
    if (!f.size) return;
    const Pic4& ic = fg->icon[f.icon];
    const uint8_t* p = ic.px[attack && ic.px[1] ? 1 : 0];
    if (!p) return;
    if (attack || bare) {
        int sx[4], sy[4];
        const int n = combat::squares(f.size, sx, sy);
        for (int k = 0; k < n; ++k) {
            const int gx = f.x + sx[k], gy = f.y + sy[k];
            if (gx >= fg->b.vx && gx < fg->b.vx + 7 && gy >= fg->b.vy && gy < fg->b.vy + 7) draw_square(c, gx, gy);
        }
    }
    const int x = kViewX + (f.x - fg->b.vx) * kSq, y = kViewY + (f.y - fg->b.vy) * kSq;
    blit4(c, p, ic.w, ic.h, x, y, f.facing >= 4, true, kViewX, kViewY, kViewX + 7 * kSq, kViewY + 7 * kSq,
          ic.own_mask ? pic4_mask(ic, p) : nullptr);
}

void draw_square(pic::Canvas& c, int x, int y)
{
    const int sx = kViewX + (x - fg->b.vx) * kSq, sy = kViewY + (y - fg->b.vy) * kSq;
    const uint8_t* g = combat::tile(fg->b, fg->t, x, y);
    const int pic_no = x >= 0 && x < combat::kW && y >= 0 && y < combat::kH && fg->b.ground[y][x] ? g[3] : -1;
    if (pic_no < 0 || pic_no >= kTiles || !fg->tile[pic_no])
        c.fill(sx, sy, kSq, kSq, d->prof->variant.fight_unswapped || blit_swap ? 8 : 0);     // (shown dark grey either way)
    else blit4(c, fg->tile[pic_no], kSq, kSq, sx, sy, false, false, 0, 0, pic::kScreenW, pic::kScreenH);
}

void draw_field(pic::Canvas& c)
{
    for (int j = 0; j < 7; ++j)
        for (int i = 0; i < 7; ++i) draw_square(c, fg->b.vx + i, fg->b.vy + j);
    for (int k = 0; k < fg->b.n; ++k)
        if (fg->b.f[k].up()) draw_icon(c, k);
    dirty(kViewY, kViewY + 7 * kSq);
}

void clear_panel(pic::Canvas& c, int r0 = 1, int r1 = 21)
{
    c.fill(23 * 8, r0 * 8, 16 * 8, (r1 - r0 + 1) * 8, 0);
    dirty_rows(r0, r1);
}

// Words wrapped into the panel (columns 23-38) from `row`; the next row
int panel_text(pic::Canvas& c, const char* s, int row, uint8_t colour, int last = 21)
{
    char line[17];
    while (*s && row <= last) {
        while (*s == ' ') ++s;
        int n = static_cast<int>(strlen(s));
        if (n > 16) {
            n = 16;
            while (n > 0 && s[n] != ' ') --n;
            if (n == 0) n = 16;
        }
        memcpy(line, s, n);
        line[n] = 0;
        put(c, line, 23, row++, colour);
        s += n;
    }
    return row;
}

// The originals' own boxed print (the weapon rows 23-38 x 7-9) - a word
// printed only when it fits, its trailing space included; returns the row
// after the one the print ended on, as the originals' text row + 1 (one
// more when the last word filled the box's last column).
// - PoolRad / Curse / Secret / Darkness / Champions / Knights (PoolRad
//   START 0x77F8, Curse START 0x5320, Darkness GAME 0x4E3A): a word is
//   its leading punctuation (! , - . : ; ?), letters up to a space or a
//   punctuation mark (that mark the word's last), then its trailing space
//   or more marks; it goes on the line if its last character, the space
//   too, is within the box, else on the next row (the "space just past
//   the edge" exception there is never reached).
// - Gateway / Treasures (GAME.OVR 0x51EC5, Treasures GAME 0x6BC8): words
//   split at spaces only; a word whose trailing space is the one past the
//   box drops it; a word ending on the box's last column is printed and
//   the row ends there.
// - The Dark Queen (DQK.EXE res 0xCD31, its fight messages through res
//   0xB14E: the name at the row, the words from the next one, columns
//   23-38, rows to 21): the first rule's words, but a word whose trailing
//   space is the one just past the box is printed without it and the row
//   ends there (queen)
int panel_box_text(pic::Canvas& c, const char* s, int row, uint8_t colour, int last, bool sf, bool queen = false, int* cut = nullptr)
{
    const int x0 = 23, x1 = 38;
    const int len = static_cast<int>(strlen(s));
    if (!len) return row + 1;
    auto at = [&](int i) { return s[i - 1]; };       // (1-based, as the originals)
    auto punct = [](char ch) { return ch && strchr("!,-.:;?", ch) != nullptr; };
    int col = x0, start = 1;
    auto print = [&](int a, int b) {
        char line[40];
        int n = 0;
        for (int i = a; i <= b && n < 39; ++i) line[n++] = at(i);
        line[n] = 0;
        if (row <= last) put(c, line, col, row, colour);
        col += n;
    };
    auto skip_spaces = [&]() {
        while (start <= len && at(start) == ' ') ++start;
    };
    while (start <= len && row <= last) {
        int end = start;
        if (sf) {
            while (end <= len && at(end) != ' ' && at(end) != '\n') ++end;
            if (end <= len && at(end) == ' ') ++end;
            --end;
            if (end < start) {                       // (a line break)
                ++start;
                col = x0;
                ++row;
                continue;
            }
            int v = col + (end - start);
            if (v < x1) {
                print(start, end);
                start = end + 1;
                continue;
            }
            if (col <= x1 && v == x1 + 1 && at(end) == ' ') --end;
            v = col + (end - start);
            if (v == x1) {
                print(start, end);
                start = end + 1;
                col = x0;
                ++row;
                skip_spaces();
            } else {
                col = x0;
                ++row;
                skip_spaces();
                if (end - start + 1 > x1 - x0 + 1) end = start + (x1 - x0);   // (longer than the box)
                print(start, end);
                start = end + 1;
            }
            continue;
        }
        while (end < len && punct(at(end))) ++end;
        while (end < len && !punct(at(end)) && at(end) != ' ') ++end;
        if (at(end) != ' ')
            while (end + 1 < len && punct(at(end + 1))) ++end;
        if (queen && col + (end - start) == x1 + 1 && at(end) == ' ') {
            print(start, end - 1);
            start = end + 1;
            col = x0;
            ++row;
            skip_spaces();
            continue;
        }
        if (col + (end - start) > x1) {
            if (col == x0) end = start + (x1 - x0);   // (longer than the box)
            else {
                col = x0;
                ++row;
                skip_spaces();
                continue;
            }
        }
        print(start, end);
        start = end + 1;
    }
    // (the Dark Queen's: a new row past the box with two characters or more
    // left - its 0xCEEA waits for a key, clears the box and goes on at its
    // top; `cut` = the character it goes on from, 0-based - the caller
    // waits. One left: its loop goes on and prints it on the row past the box)
    if (cut) *cut = queen && row > last && start < len ? start - 1 : -1;
    if (queen && row == last + 1 && start == len) {
        const char one[2] = {at(start), 0};
        put(c, one, x0, row, colour);
        col = x0 + 1;
    }
    if (col > x1) ++row;
    return row + 1;
}

void draw_panel_for(pic::Canvas& c, int i)
{
    clear_panel(c);
    if (i < 0) return;
    const combat::Fighter& f = fg->b.f[i];
    char nm[20], t[48];
    fighter_name(f, nm, sizeof nm);
    put(c, nm, 23, 1, name_colour(f));
    put(c, fw(profile::kHitpoints), 23, 3, 10);
    snprintf(t, sizeof t, "%d", f.hp());
    put(c, t, 33, 3, f.hp() < f.hp_max() ? 14 : 10);
    put(c, fw(profile::kAcW), 23, 5, 10);
    const int ac = f.rec[0x19A];
    snprintf(t, sizeof t, "%s%d", ac > 60 ? "-" : "", ac > 60 ? ac - 60 : 60 - ac);
    put(c, t, 26, 5, 10);
    int row = 7;
    const int wi = combat::weapon(f, *names);
    if (wi >= 0) {
        names->name(items::Item{f.items[wi]}, t, sizeof t - 1);
        // (the Dark Queen's, res 0xA50B: the name through its boxed print
        // 0xCD31, rows 7-9; the status two rows past where it ended - 0xA561;
        // its name builder res 0x9FDC ends every part with " ", or "s " for
        // the plural - the last one too)
        if (hl()) {
            strcat(t, " ");
            row = panel_box_text(c, t, 7, 10, 9, false, true) + 1;
        } else {
            // (the originals' names keep their last word's space - PoolRad
            // START 0x12CC, Gateway GAME.OVR 0x3F384: " " / "s " added)
            strcat(t, " ");
            row = panel_box_text(c, t, 7, 10, 9, gateway() || d->prof->variant.tr) + 1;
        }
    }
    if (!f.up()) put(c, name_of(d->health[0], 13, 9, f.status()), 23, row, 15);
}

// The view brought to fighter i (radius r: moved only when further than
// that from the centre)
// The view moved so square (x, y) is within r of its middle (0: in the
// middle - as far as the field's edges let it)
void focus_at(int x, int y, int r)
{
    combat::Battle& b = fg->b;
    int cx = b.vx + 3, cy = b.vy + 3;
    if (x - cx > r || cx - x > r || r == 0xFF) cx = x;
    if (y - cy > r || cy - y > r || r == 0xFF) cy = y;
    if (cx < 3) cx = 3;
    if (cx > combat::kW - 4) cx = combat::kW - 4;
    if (cy < 3) cy = 3;
    if (cy > combat::kH - 4) cy = combat::kH - 4;
    b.vx = cx - 3;
    b.vy = cy - 3;
}

void focus(int i, int r)
{
    focus_at(fg->b.f[i].x, fg->b.f[i].y, r);
}

bool on_screen(int i)
{
    const combat::Fighter& f = fg->b.f[i];
    return f.x >= fg->b.vx - 1 && f.x < fg->b.vx + 7 && f.y >= fg->b.vy - 1 && f.y < fg->b.vy + 7;
}

// The one whose turn it is: the box under them, their icon on it
void mark(pic::Canvas& c, int i)
{
    const combat::Fighter& f = fg->b.f[i];
    if (!f.size || !fg->cursor) return;
    int sx[4], sy[4];
    const int n = combat::squares(f.size, sx, sy);
    for (int k = 0; k < n; ++k) {
        const int x = kViewX + (f.x + sx[k] - fg->b.vx) * kSq, y = kViewY + (f.y + sy[k] - fg->b.vy) * kSq;
        blit4(c, fg->cursor, kSq, kSq, x, y, false, true, kViewX, kViewY, kViewX + 7 * kSq, kViewY + 7 * kSq);
    }
    draw_icon(c, i);
}

// (the Dark Queen's Aim: only what's aimed at marked - DOSBox)
bool q_aim_unmarked = false;

void draw_fight(pic::Canvas& c)
{
    if (hl()) q_draw_fight_frame(c);
    else {
        c.fill(0, 0, pic::kScreenW, 24 * 8, 0);
        layout::combat(c, d->tables, d->frame_tiles);
    }
    draw_field(c);
    if (fg->cur >= 0) {
        if (!q_aim_unmarked) mark(c, fg->cur);
        draw_panel_for(c, fg->cur);
    }
    dirty(0, pic::kScreenH);
}

void status_line(pic::Canvas& c, const char* t)
{
    if (hl()) {
        q_status(c, t);
        return;
    }
    c.fill(0, 23 * 8, pic::kScreenW, 8, 0);
    if (t && *t) put(c, t, 0, 23, 10);
    dirty_rows(23, 23);
}

// ---- Messages: pages in the panel from row 10, a game delay each
void add_page(int who, const char* a, const char* b2 = nullptr, int whom = -1, const char* c2 = nullptr)
{
    // (dropped when the pages or their text pool are full: 5 longest lines' room)
    fg->page_dropped = fg->pages >= kPages || fg->ptext_used > kPageText - 5 * 48;
    if (fg->page_dropped) return;
    if (fg->rolls) fg->rolls->page[fg->pages][0].kind = fg->rolls->page[fg->pages][1].kind = combat::RollKind::None;
    Page& p = fg->page[fg->pages++];
    p.lines = 0;
    p.sound = 0;
    p.weapon = 0;
    p.names = 0;
    p.cont = fg->cont_next;
    p.row = 0;
    p.clears = false;
    p.blank = false;
    p.keep_end = false;
    fg->cont_next = false;
    p.fx[0] = p.fx[1] = Fx{};
    auto line = [&](const char* s, uint8_t col) {
        if (p.lines < 5 && s) {
            char* at = fg->ptext + fg->ptext_used;
            const int n = snprintf(at, 48, "%s", s);
            p.line_at[p.lines] = static_cast<uint16_t>(fg->ptext_used);
            fg->ptext_used += (n < 47 ? n : 47) + 1;
            p.colour[p.lines++] = col;
        }
    };
    char nm[20];
    if (who >= 0) {
        fighter_name(fg->b.f[who], nm, sizeof nm);
        p.names |= static_cast<uint8_t>(1u << p.lines);
        line(nm, name_colour(fg->b.f[who]));
    }
    line(a, 10);
    if (whom >= 0) {
        fighter_name(fg->b.f[whom], nm, sizeof nm);
        p.names |= static_cast<uint8_t>(1u << p.lines);
        line(nm, name_colour(fg->b.f[whom]));
    }
    line(b2, 10);
    line(c2, 10);
}

const char* page_line(const Page& p, int k) { return fg->ptext + p.line_at[k]; }

// The sound the last page queued makes as it shows
void page_sound(int id)
{
    if (fg->pages > 0 && !fg->page_dropped) fg->page[fg->pages - 1].sound = static_cast<uint8_t>(id);
}

// ---- The rolls shown (CYD_COMBAT_ROLLS, Tom 2026-10-31): the engine writes
// down each roll (combat::g_rolls); a page takes those it tells of
// A new attack's / spell's rolls: the record starts again
void rolls_clear()
{
    if (!fg || !fg->rolls) return;
#ifdef CYD_TEST_HOOKS
    {
        // (tests: the blows whose to-hit roll no page took - Show Rolls' "every blow")
        int left = 0;
        const combat::RollLog& l = fg->rolls->log;
        for (int i = 0; i < l.n; ++i)
            if (!(l.taken >> i & 1) && l.r[i].kind == combat::RollKind::Hit) ++left;
        CYD_TEST_FX(RollsClear, fg->page_dropped ? 1 : 0, left);
    }
#endif
    fg->rolls->log.n = 0;
    fg->rolls->log.taken = 0;
}
// The first roll of kind k against fighter `who` not on a page yet (blow: a
// weapon attack's hit number, 0xFF the others', 0xFE any; last: the latest),
// now on one; nullptr: none
const combat::Roll* take_roll(combat::RollKind k, int who, int blow, bool last = false)
{
    if (!fg || !fg->rolls) return nullptr;
    combat::RollLog& l = fg->rolls->log;
    int got = -1;
    for (int i = 0; i < l.n; ++i) {
        const combat::Roll& r = l.r[i];
        if ((l.taken >> i & 1) || r.kind != k || r.who != who || (blow != 0xFE && r.blow != blow)) continue;
        got = i;
        if (!last) break;
    }
    if (got < 0) return nullptr;
    l.taken |= 1u << got;
    return &l.r[got];
}
// The last page queued shows roll r too (2 at most)
void page_roll(const combat::Roll* r)
{
    if (!r || !fg->rolls || !fg->pages || fg->page_dropped) return;
    combat::Roll* p = fg->rolls->page[fg->pages - 1];
    for (int k = 0; k < 2; ++k)
        if (p[k].kind == combat::RollKind::None) {
            p[k] = *r;
            return;
        }
}

// ---- The pages' pictures (facts: the Project's combat_setup_facts.md 6.5 /
// 6.6 and sound_facts.md). A flight: the picture steps 8 pixels at a time
// toward the target, stopping a step short, `delay` ms a step, drawn over
// the field and the field put back after each step; a burst: the 4
// pictures on the fighter's square 70 ms each (sparkles: as many rounds as
// the game speed + 1); the fallen: their squares blink skull / box 9 times,
// 10 ms each, and the skull stays until the page ends.
constexpr int kViewW = 7 * kSq;

void fx_free()
{
    free(fg->fx_pic[0]);
    free(fg->fx_pic[1]);
    fg->fx_pic[0] = fg->fx_pic[1] = nullptr;
}

// A COMSPR picture by Curse's number (the engine's): the game's own (the
// profile's fight.comspr; none: as Curse's)
int comspr(int pic)
{
    const uint8_t* m = d->prof->fight.comspr;
    return m[0] || m[1] ? (pic < 16 ? m[pic] : pic) : pic;
}

bool fx_load(int pic)
{
    fx_free();
    int w = 0, h = 0, w2 = 0, h2 = 0;
    pic = comspr(pic);
    fg->fx_pic[0] = load_pic4("COMSPR.DAX", pic, 0, &w, &h);
    fg->fx_pic[1] = load_pic4("COMSPR.DAX", pic + 0x80, 0, &w2, &h2);
    if (!fg->fx_pic[0] || w != kSq || h != kSq || (fg->fx_pic[1] && (w2 != kSq || h2 != kSq))) {
        plat::log("[fight] COMSPR %d: no 24 x 24 picture (%d x %d)\n", pic, w, h);
        fx_free();
        return false;
    }
    return true;
}

// The field under a picture at (x, y): kept / put back
void under_put(pic::Canvas& c)
{
    if (!fg->has_under) return;
    fg->has_under = false;
    for (int j = 0; j < kSq; ++j) {
        const int y = fg->under_y + j;
        if (y < kViewY || y >= kViewY + kViewW) continue;
        for (int i = 0; i < kSq; ++i) {
            const int x = fg->under_x + i;
            if (x >= kViewX && x < kViewX + kViewW) c.set(x, y, fg->under[j * kSq + i]);
        }
    }
    dirty(fg->under_y, fg->under_y + kSq);
}

void under_save(pic::Canvas& c, int x, int y)
{
    fg->under_x = x;
    fg->under_y = y;
    fg->has_under = true;
    for (int j = 0; j < kSq; ++j)
        for (int i = 0; i < kSq; ++i) {
            const int xx = x + i, yy = y + j;
            if (xx >= kViewX && xx < kViewX + kViewW && yy >= kViewY && yy < kViewY + kViewW)
                fg->under[j * kSq + i] = c.get(xx, yy);
        }
}

// One of the cycle's pictures (bit 0: the attack picture, bit 1 mirrored)
void fx_draw(pic::Canvas& c, int x, int y, uint8_t slot)
{
    const uint8_t* p = (slot & 1) && fg->fx_pic[1] ? fg->fx_pic[1] : fg->fx_pic[0];
    blit4(c, p, kSq, kSq, x, y, (slot & 2) != 0, true, kViewX, kViewY, kViewX + kViewW, kViewY + kViewW);
    dirty(y, y + kSq);
}

int sq_x(int fx) { return kViewX + (fx - fg->b.vx) * kSq; }
int sq_y(int fy) { return kViewY + (fy - fg->b.vy) * kSq; }

// One step of the running picture; false when it's over
bool fx_step(pic::Canvas& c)
{
    Fx& e = fg->fx;
    const int k = fg->fx_step++;
    switch (e.kind) {
    case Fx::Bolt:
    case Fx::Fly: {
        under_put(c);
        while (!combat::flight_step(fg->path)) {
            // A bolt: on along its segments (the lightning picture, COMSPR 6, 50 ms a step)
            if (e.kind != Fx::Bolt || fg->bolt_at + 1 >= fg->b.n_bolt) return false;
            if (fg->bolt_at < 0) {
                if (!fx_load(6)) return false;
                e.fl = combat::spell_flight(6, 50);
            }
            const combat::BoltSeg& sg = fg->b.bolt[++fg->bolt_at];
            combat::flight_begin(fg->path, sg.x0, sg.y0, sg.x1, sg.y1);
        }
        const int x = kViewX + (fg->path.x - fg->b.vx * 3) * 8, y = kViewY + (fg->path.y - fg->b.vy * 3) * 8;
        under_save(c, x, y);
        fx_draw(c, x, y, e.fl.slot[e.fl.frames ? k % e.fl.frames : 0]);
        fg->until = plat::now_ms() + (e.fl.delay ? e.fl.delay : 1);
        CYD_TEST_FX(Frame, e.fl.delay ? e.fl.delay : 1, 1);
        return true;
    }
    case Fx::Burst: {
        under_put(c);
        if (k >= e.loops * 4) return false;
        const combat::Fighter& f = fg->b.f[e.who];
        const int x = sq_x(f.x), y = sq_y(f.y);
        under_save(c, x, y);
        fx_draw(c, x, y, e.fl.slot[k & 3]);
        fg->until = plat::now_ms() + 70;
        CYD_TEST_FX(Frame, 70, 1);
        return true;
    }
    case Fx::Skull: {
        if (k >= 9) return false;
        const combat::Fighter& f = fg->b.f[e.who];
        const uint8_t* p = k & 1 ? fg->cursor : fg->skull;
        int sx[4], sy[4];
        // (every square it stood on - a big monster's fallen keeps its size
        // in down_size: Tom, 2026-11-06, the skull on one square of four)
        const int n = combat::squares(f.size ? f.size : f.down_size ? f.down_size : 1, sx, sy);
        for (int q = 0; q < n && p; ++q) {
            const int x = sq_x(f.x + sx[q]), y = sq_y(f.y + sy[q]);
            blit4(c, p, kSq, kSq, x, y, false, true, kViewX, kViewY, kViewX + kViewW, kViewY + kViewW);
            dirty(y, y + kSq);
        }
        fg->fx_redraw = true;
        fg->until = plat::now_ms() + 10;
        CYD_TEST_FX(Frame, 10, p ? n : 0);
        return true;
    }
    default: return false;
    }
}

// Starts a page's picture (its sound first); false: nothing to show
bool fx_begin(pic::Canvas& c, const Fx& e)
{
    if (e.sound) sfx(e.sound);
    if (e.kind == Fx::None) return false;
    if (e.kind == Fx::Redraw) {
        draw_field(c);
        if (fg->cur >= 0 && fg->b.f[fg->cur].up()) mark(c, fg->cur);
        return false;
    }
    fg->fx = e;
    fg->fx_step = 0;
    fg->has_under = false;
    switch (e.kind) {
    case Fx::Fly:
    case Fx::Bolt:
        if (!fx_load(e.fl.pic)) return false;
        fg->bolt_at = -1;
        combat::flight_begin(fg->path, e.x0, e.y0, e.x1, e.y1);
        break;
    case Fx::Burst:
        if (e.who < 0 || !fx_load(e.fl.pic)) return false;
        if (!on_screen(e.who)) {
            focus(e.who, 3);
            draw_field(c);
            if (fg->cur >= 0) mark(c, fg->cur);
        }
        break;
    case Fx::Skull:
        if (e.who < 0 || !fg->skull) return false;
        if (!on_screen(e.who)) {
            focus(e.who, 3);
            draw_field(c);
            if (fg->cur >= 0) mark(c, fg->cur);
        }
        break;
    default: return false;
    }
    if (!fx_step(c)) {
        fx_free();
        return false;
    }
    fg->fx_on = true;
    return true;
}

// The page's next picture, or its wait when they're all done
void page_fx(pic::Canvas& c)
{
    const Page& p = fg->page[fg->at - 1];
    while (fg->fx_at < 2) {
        if (fx_begin(c, p.fx[fg->fx_at])) return;
        ++fg->fx_at;
    }
    fg->until = plat::now_ms() + game_delay_ms() * 2;
    if (!fg->until) fg->until = 1;
    CYD_TEST_FX(PageWait, game_delay_ms() * 2, 0);
}

// The running picture done (or cut short by a tap): the field put back
void fx_end(pic::Canvas& c)
{
    under_put(c);
    fx_free();
    fg->fx_on = false;
    ++fg->fx_at;
}

// A page over: a skull left on the field goes
void page_over(pic::Canvas& c)
{
    if (!fg->fx_redraw) return;
    fg->fx_redraw = false;
    CYD_TEST_FX(FieldBack, 0, 0);
    draw_field(c);
    if (fg->cur >= 0 && fg->b.f[fg->cur].up()) mark(c, fg->cur);
}

bool log_pages = false;                  // (the playsim's PAGELOG: each page's lines printed)

// "To Hit: 17+2=19" (the short label when the long one makes it over the
// panel's 16 columns)
void roll_line(char* t, size_t cap, const char* label, const char* brief, const combat::Roll& r)
{
    const int m = r.mod < 0 ? -r.mod : r.mod;
    snprintf(t, cap, "%s%d%c%d=%d", label, r.nat, r.mod < 0 ? '-' : '+', m, r.nat + r.mod);
    if (strlen(t) > 16) snprintf(t, cap, "%s%d%c%d=%d", brief, r.nat, r.mod < 0 ? '-' : '+', m, r.nat + r.mod);
}
// A page's rolls at the panel's foot (its last rows, under the page's own
// lines - never over them: with too few rows the "Needed" lines go first,
// then the last lines): to hit / a save and the total needed, then damage
void show_rolls(pic::Canvas& c, const combat::Roll* pr, int row)
{
    char line[4][24];
    bool need[4] = {};
    int n = 0;
    static const combat::RollKind kOrder[3] = {combat::RollKind::Hit, combat::RollKind::Save, combat::RollKind::Damage};
    for (combat::RollKind kind : kOrder)
        for (int k = 0; k < 2 && n < 3; ++k) {
            const combat::Roll& r = pr[k];
            if (r.kind != kind) continue;
            if (kind == combat::RollKind::Damage) {
                roll_line(line[n++], sizeof line[0], "Damage: ", "Dmg: ", r);
                continue;
            }
            if (kind == combat::RollKind::Save && r.need < 0) {
                snprintf(line[n++], sizeof line[0], "Save: %d immune", r.nat);
                continue;
            }
            roll_line(line[n++], sizeof line[0], kind == combat::RollKind::Hit ? "To Hit: " : "Save: ",
                      kind == combat::RollKind::Hit ? "Hit: " : "Save:", r);
            snprintf(line[n], sizeof line[0], "Needed: %d", r.need);
            need[n++] = true;
        }
    // (rows row .. 21 are free)
    const int free_rows = 22 - row;
    for (int k = n - 1; k >= 0 && n > free_rows; --k)
        if (need[k]) {
            for (int j = k; j < n - 1; ++j) {
                memcpy(line[j], line[j + 1], sizeof line[0]);
                need[j] = need[j + 1];
            }
            --n;
        }
    if (n > free_rows) n = free_rows < 0 ? 0 : free_rows;
    for (int k = 0; k < n; ++k) {
        put(c, line[k], 23, 22 - n + k, roll_colour());
        if (log_pages) plat::log("[rolls] %s\n", line[k]);
    }
}

void page_lines(pic::Canvas& c, int k0, int from, int row);
void show_page(pic::Canvas& c)
{
    page_over(c);
    CYD_TEST_FX(Page, 0, fg->at);
    const Page& p = fg->page[fg->at++];
    if (log_pages) plat::log("[page] %s %s %s\n", p.lines > 0 ? page_line(p, 0) : "", p.lines > 1 ? page_line(p, 1) : "",
                                 p.lines > 2 ? page_line(p, 2) : "");
    if (p.sound) sfx(p.sound);
    if (p.weapon) {
        // Kalistes' blow (GAME.OVR 0x1D924): the weapon and "+4" in the
        // attacker's weapon rows - the window 23-38 x 7-9 cleared, colour 10
        char t[40], a[24], b4[8];
        const auto& kw = d->prof->fight.kalistes_words;
        ow(kw[p.weapon - 1], a, sizeof a);
        ow(kw[6], b4, sizeof b4);
        snprintf(t, sizeof t, "%s%s", a, b4);
        clear_panel(c, 7, 9);
        panel_box_text(c, t, 7, 10, 9, false);
    }
    // The originals' messages (PoolRad ovr14 +0x2FA, Curse / Secret / Darkness
    // / Gateway / Treasures / Champions the same routine): a name printed as
    // it is (a row of its own), the words through the boxed print from the
    // next row - the next print starts at its last row + 1 (+ 1 more when it
    // ended on the box's last column); a fall after a hit goes on below the
    // hit's words, the rows above kept (the box cleared from there down).
    // (The Dark Queen's: res 0xB14E - the name at the row, the words through
    // its boxed print 0xCD31 from the next; a fall goes on at the row after
    // the hit's / damage text's last, DQK.EXE 0x25020 / res 0x8F04)
    // (a page after the Dark Queen's "lost a spell" - its own row, its wait
    // clearing 10-21, res 0xB245 - goes on at the row kept, the box blank)
    int row = p.row ? p.row : 10;
    const bool after_own_row = fg->at >= 2 && fg->page[fg->at - 2].clears;
    if (p.cont && fg->page_end > 10 && fg->page_end <= 21) row = fg->page_end;
    clear_panel(c, (p.cont && after_own_row) || p.blank ? 10 : row, 21);
    page_lines(c, 0, 0, row);
}

// A page's lines from line k (its character `from` on), at `row`; then its
// rolls and pictures (the page's wait) - or, the Dark Queen's box full, the
// key it waits for first (FSt::More: res 0xCEEA's "Press button or enter to
// continue", the box rows from the words' first cleared, on from there)
void page_lines(pic::Canvas& c, int k0, int from, int row)
{
    const Page& p = fg->page[fg->at - 1];
    const bool sf = gateway() || d->prof->variant.tr;
    for (int k = k0; k < p.lines; ++k) {
        const char* s = page_line(p, k) + (k == k0 ? from : 0);
        if (p.names & (1u << k)) {
            if (row <= 21) put(c, s, 23, row, p.colour[k]);
            ++row;
        } else {
            int cut = -1;
            const int top = row;
            row = panel_box_text(c, s, row, p.colour[k], 21, sf, hl(), hl() ? &cut : nullptr);
            if (cut >= 0) {
                fg->more_line = static_cast<int8_t>(k);
                fg->more_at = static_cast<uint8_t>((k == k0 ? from : 0) + cut);
                fg->more_top = static_cast<int8_t>(top);
                fg->st = FSt::More;
                fg->until = 0;
                // (res 0xD400 -> 0xD35F -> 0xE6C1 -> 0xE645: its arguments
                // unused - the prompt box's "Press [Return] to continue." at
                // row 24, as press_line draws it)
                press_line(c, 11);
                return;
            }
        }
    }
    if (!p.keep_end) fg->page_end = static_cast<int8_t>(row < 10 ? 10 : row > 22 ? 22 : row);
    if (fg->rolls && opts.rolls) show_rolls(c, fg->rolls->page[fg->at - 1], row);
    fg->fx_at = 0;
    page_fx(c);
}

// The key after a full box: cleared from the words' first row, the rest there
void page_more(pic::Canvas& c)
{
    const int k = fg->more_line, top = fg->more_top, from = fg->more_at;
    fg->more_line = -1;
    fg->st = FSt::Pages;
    clear_menu_line(c);
    clear_panel(c, top, 21);
    page_lines(c, k, from, top);
}

// A page's pictures, as the games show them
Fx fly_fx(int from, int to, const combat::Flight& fl)
{
    Fx e;
    e.kind = Fx::Fly;
    e.fl = fl;
    e.x0 = static_cast<uint8_t>(fg->b.f[from].x);
    e.y0 = static_cast<uint8_t>(fg->b.f[from].y);
    e.x1 = static_cast<uint8_t>(fg->b.f[to].x);
    e.y1 = static_cast<uint8_t>(fg->b.f[to].y);
    return e;
}

// A magic hit on fighter `who`: damage - the burst (sound 3), once; an
// effect - sparkles (sound 4), the game speed + 1 times round
Fx hit_fx(int who, bool sparkles)
{
    Fx e;
    e.kind = Fx::Burst;
    e.who = static_cast<int8_t>(who);
    e.fl = combat::spell_flight(sparkles ? 9 : 10, 70);
    e.sound = sparkles ? sound::kMagicStars : sound::kMagicHit;
    int speed = vm ? vm->get(0x4BFC) & 0xFF : 0;
    if (speed > 9) speed = 9;
    e.loops = static_cast<uint8_t>(sparkles ? speed + 1 : 1);
    return e;
}

Fx skull_fx(int who)
{
    Fx e;
    e.kind = Fx::Skull;
    e.who = static_cast<int8_t>(who);
    e.sound = sound::kDeath;
    return e;
}

// The last page queued gets picture n (0, 1)
void page_fx_set(int n, const Fx& e)
{
    if (fg->pages > 0 && !fg->page_dropped) fg->page[fg->pages - 1].fx[n] = e;
}

// The pages queued: shown one after another, then on to `next`
void say(pic::Canvas& c, FSt next)
{
    fg->after = next;
    fg->st = FSt::Pages;
    fg->at = 0;
    if (menu_on) clear_menu_line(c);
    // (the attack's frame: the originals' Delay(100) whatever the speed;
    // ours 10 ms + 10 ms a speed step, 100 at 9 - Tom, 2026-11-06, SPEC 12)
    if (fg->pages && fg->attack_pause) {
        int speed = vm ? vm->get(0x4BFC) & 0xFF : 0;
        if (speed > 9) speed = 9;
        fg->until = plat::now_ms() + 10 + 10 * speed;          // (the first page after it: fight_tick)
        CYD_TEST_FX(AttackPause, 10 + 10 * speed, 0);
    }
    else if (fg->pages) show_page(c);
    else fg->until = 1;
    fg->attack_pause = false;
}

// ---- Turns
void begin_round(pic::Canvas& c);
void next_turn(pic::Canvas& c);
void fight_menu(pic::Canvas& c);
void end_fight(pic::Canvas& c);

void end_turn(pic::Canvas& c)
{
    if (fg->cur >= 0) {
        combat::Fighter& f = fg->b.f[fg->cur];
        f.delay = 0;
        f.moves = 0;
    }
    fg->attack_pause = false;
    status_line(c, "");
    next_turn(c);
}

// Someone down: "goes down" / "and is Dying" / "is killed", the skull blinking on them
void bozak_burst(int t);
void kapak_acid(int t);
void damage_text(int amount, int kind, char* t, size_t cap);
// The Dark Queen's fall after a damage text (DQK.EXE res 0x8F04): on the
// same screen, from the row after that text's last (res 0xB14E at
// [0xAE4E] + 1, no wait between) - "is killed" (0xFC), "Goes Down and is
// Dying" ("%s and is Dying" 0x1A5 round "Goes Down" 0x19B), "Goes Down"
void q_fall(int who, combat::Down dn)
{
    fg->cont_next = fg->pages > 0 && !fg->page_dropped;
    // (record 0xC8 & 8 - the undead -: "Goes Down" whatever the status, res
    // 0x8F04 0x9157)
    if (dn == combat::Down::Dead && !(fg->b.f[who].flags2 & 8)) {
        add_page(who, fw(profile::kIsKilled));
        return;
    }
    if (dn != combat::Down::Dying || (fg->b.f[who].flags2 & 8)) {
        add_page(who, fw(profile::kGoesDownS));
        return;
    }
    // (the format's "%s" taken out at loading: " and is Dying" after it)
    char t[48];
    snprintf(t, sizeof t, "%s%s", fw(profile::kGoesDownS), fw(profile::kGoesDownDyingQ));
    add_page(who, t);
}

// The Dark Queen: a caster hurt with a spell under way loses it at once -
// after a blow's page (DQK.EXE 0x2522F) or a damage text's (0x9106): res
// 0xB14E(target, "lost a spell", row 12, wait), the memorized spell marked
// spent (res 0xAF26), the spell gone - nothing at its turn. After a blow the
// fall goes on at the row after the blow's words (0x251F2: kept before);
// after a damage text, below "lost a spell" (0x91B4: [0xAE4E] + 1).
// Returns whether its page was added.
// The computer's spell begun earlier waits for its AI - after the morale
// test and the items (Darkness, Treasures, Death Knights, the Dark Queen,
// Champions); the others cast it at the turn's start
bool late_spell() { return darkness_files() || hl() || d->prof->game == games::Game::ChampionsOfKrynn; }

void forget_spell(combat::Fighter& f, int sp);
bool q_lost_spell(int who, bool after_blow)
{
    if (!hl()) return false;
    combat::Fighter& f = fg->b.f[who];
    if (!f.spell || f.can_cast) return false;
    if (!f.spell_n) forget_spell(f, f.spell);
    f.spell = 0;
    f.spell_n = 0;
    add_page(who, fw(profile::kLostSpell));
    if (fg->page_dropped) return false;
    Page& p = fg->page[fg->pages - 1];
    p.row = 12;
    p.clears = true;
    p.keep_end = after_blow;
    return true;
}

void down_page(int t, bool after_hit = false)
{
    if (fg->b.f[t].escaped) {
        // Pool's vampire: "Turns gaseous and escapes"
        add_page(t, fw(profile::kGaseous));
        return;
    }
    const combat::Down dn = combat::down_state(fg->b.f[t]);
    // After a hit (PoolRad ovr14 +0x550, Curse / Secret / Darkness / Gateway /
    // Treasures / Champions alike): below the hit's words, "goes down", then
    // "and is Dying" or (dead, stoned, gone) the name again and "is killed"
    // (the Dark Queen's after a hit, DQK.EXE 0x25020: the same - its res
    // 0xB14E at the row after the hit's words; after a damage text: q_fall)
    fg->cont_next = after_hit;
    if (hl() && !after_hit) q_fall(t, dn);
    else if (hl() && (fg->b.f[t].flags2 & 8)) add_page(t, fw(profile::kGoesDown));   // (DQK.EXE 0x2528C: the undead)
    else if (dn == combat::Down::Dead && after_hit) add_page(t, fw(profile::kGoesDown), nullptr, t, fw(profile::kIsKilled));
    else if (dn == combat::Down::Dead) add_page(t, fw(profile::kIsKilled));
    else if (dn == combat::Down::Dying) add_page(t, fw(profile::kGoesDown), fw(profile::kIsDying));
    else add_page(t, fw(profile::kGoesDown));
    page_fx_set(1, skull_fx(t));
    bozak_burst(t);
    kapak_acid(t);
}

// Champions' kapak going down (GAME.OVR 0x12C1F): "dissolves into acid!",
// its square an acid pool for 1d6 rounds
void kapak_acid(int t)
{
    combat::Battle& b = fg->b;
    const uint8_t ac = b.fx ? b.fx->mon.acid_pool : 0;
    if (!ac || !b.f[t].has(ac) || (fg->burst[t / 8] >> (t % 8) & 1) || !d->w_acid[0][0]) return;
    fg->burst[t / 8] |= static_cast<uint8_t>(1 << (t % 8));
    add_page(t, d->w_acid[0]);
    combat::lay_acid(b, fg->t, b.f[t].x, b.f[t].y, rng);
}

// The acid's 1d6 on one with a square in it (a step in, a round's end -
// GAME.OVR 0x34F7F): "takes N points of damage", no save
void acid_page(int i)
{
    bool dn = false;
    const int done = combat::acid_burn(fg->b, i, rng, &dn);
    if (done <= 0) return;
    char t[48];
    damage_text(done, 0, t, sizeof t);
    add_page(i, t);
    page_fx_set(1, hit_fx(i, false));
    if (dn) down_page(i);
}

// Champions' bozak going down (krynn_facts; GAME.OVR 0x124A1): "explodes!",
// then each fighter within a square of it: the damage (3d6), down, or
// "is stunned" when its save against spells fails
void bozak_burst(int t)
{
    combat::Battle& b = fg->b;
    const uint8_t ex = b.fx ? b.fx->mon.explode : 0;
    if (!ex || !b.f[t].has(ex) || (fg->burst[t / 8] >> (t % 8) & 1) || !d->w_burst[0][0]) return;
    fg->burst[t / 8] |= static_cast<uint8_t>(1 << (t % 8));
    add_page(t, d->w_burst[0]);
    int who[combat::kMaxFighters], dmg[combat::kMaxFighters];
    bool dn[combat::kMaxFighters], st[combat::kMaxFighters];
    const int n = combat::bozak_burst(b, fg->t, t, rng, who, dmg, dn, st, combat::kMaxFighters);
    char line[48];
    for (int k = 0; k < n; ++k) {
        damage_text(dmg[k], 0, line, sizeof line);
        add_page(who[k], line);
        page_fx_set(1, hit_fx(who[k], false));
        if (dn[k]) down_page(who[k]);                   // (another bozak: its own burst)
        else if (st[k]) add_page(who[k], d->w_burst[1]);
    }
}

// Champions' baaz slain by a party member's blow (GAME.OVR 0x129AD): a d20
// at least the slayer's Dexterity - 3 leaves the weapon in the stone body -
// "loses his weapon" - till the fight's end
void baaz_weapon(int a, int t)
{
    combat::Battle& b = fg->b;
    const uint8_t sd = b.fx ? b.fx->mon.stone_death : 0;
    combat::Fighter& f = b.f[a];
    if (!sd || !b.f[t].has(sd) || f.member < 0 || !names || fg->n_lost >= 6 || !d->w_burst[2][0]) return;
    const int w = combat::weapon(f, *names);
    if (w < 0 || !combat::baaz_keeps_weapon(f, rng)) return;
    party::Character& ch = pt->m[f.member];
    // (one kind of weapon never stays: type 6 with word 0x79 - 0x12A61)
    if (ch.items[w][0x2E] == 6 && ch.items[w][0x31] == 0x79) return;
    add_page(a, d->w_burst[2]);
    memcpy(fg->lost[fg->n_lost], ch.items[w], items::kRecordSize);
    fg->lost[fg->n_lost][0x34] = 0;
    fg->lost_who[fg->n_lost++] = static_cast<int8_t>(f.member);
    rules::remove_item(ch, w);
    f.n_items = ch.n_items;
    rules::recalc(ch, *names, d->facts);
}

// Stepping into a cloud: its save or nausea, as a page (the cloud spell's words)
const combat::FightSpell* fight_spell(int sp);
void poison_page(int i)
{
    add_page(i, fw(profile::kIsPoisoned));
    down_page(i);
}

void damage_text(int amount, int kind, char* t, size_t cap);
// Darkness's Blade Barrier on one in its knives (R2: a step into them, a
// round's end): "is cut by knives", the damage (magic) - no save
void knives_page(int i)
{
    bool dn = false;
    const int done = combat::knives_cut(fg->b, i, rng, &dn);
    const combat::FightSpell* fs = fight_spell(0x65);
    if (done <= 0 || !fs) return;
    char t[48];
    ow(fs->word, t, sizeof t);
    add_page(i, t);
    damage_text(done, 8, t, sizeof t);
    add_page(i, t);
    page_fx_set(1, hit_fx(i, false));
    if (dn) down_page(i);
}

void cloud_breath(int i);
// A step's grounds: a cloud's, then (Champions) the kapak's acid
void cloud_step(int i)
{
    cloud_breath(i);
    if (fg->b.f[i].up() && combat::on_acid(fg->b, i)) acid_page(i);
}

void cloud_breath(int i)
{
    combat::Battle& b = fg->b;
    const combat::Fighter& f = b.f[i];
    if (f.up() && combat::in_knives(b, f.x, f.y)) {
        knives_page(i);
        return;
    }
    if (f.up() && combat::in_poison(b, f.x, f.y)) {
        if (combat::breathe_poison(b, i, rng)) poison_page(i);
        return;
    }
    if (!f.up() || !combat::in_cloud(b, f.x, f.y)) return;
    const combat::Did r = combat::breathe_cloud(b, i, rng);
    if (r == combat::Did::Unaffected) return;
    const auto& fs = d->prof->fight;
    for (int k = 0; k < fs.n_spells; ++k)
        if (fs.spells[k].does == combat::SpellDoes::Cloud) {
            char t[48];
            ow(r == combat::Did::Word ? fs.spells[k].word : fs.spells[k].word2, t, sizeof t);
            add_page(i, t);
            // (the Dark Queen, DQK.EXE 0x816B: one coughing - the save made -
            // loses the spell under way, "lost a spell" at row 12)
            if (r == combat::Did::Word2 && hl() && b.f[i].spell) {
                b.f[i].can_cast = false;
                q_lost_spell(i, false);
            }
            return;
        }
}

// "takes N points of damage from Fire" ... (spell_facts.md 1.7)
void damage_text(int amount, int kind, char* t, size_t cap)
{
    const char* from = "";
    const int k = kind & 0xF7;
    if (k == 0x01) from = fw(profile::kFromFire);
    else if (k == 0x02) from = fw(profile::kFromCold);
    else if (k == 0x04) from = fw(profile::kFromElec);
    else if (k == 0x10) from = fw(profile::kFromAcid);
    else if (kind == 0x08) from = fw(profile::kFromMagic);
    if (amount == 1) snprintf(t, cap, "%s%s", fw(profile::kTakes1), from);
    else snprintf(t, cap, "%s%d%s%s", fw(profile::kTakes), amount, fw(profile::kPointsOfDamage), from);
}

void spell_pages(int caster, const combat::SpellLine* l, int n, const combat::FightSpell& fs, bool each);

// The beholder's spells (Fear, Slow, Sleep): cast as the computer casts,
// each picking its own targets
// (`at` >= 0: at that one, its own pages before - Darkness's vampire gaze)
void special_cast(int i, int sp, int at)
{
    combat::Battle& b = fg->b;
    if (!load_magic()) return;
    const combat::FightSpell* fs = fight_spell(sp);
    if (!fs) return;
    const int pw = combat::power_of(b.f[i], mrules->tables, sp, false);
    int tg[combat::kMaxFighters], nt = 0;
    if (at >= 0 && at < b.n) {
        tg[nt++] = at;
        combat::SpellLine lines[24];
        rolls_clear();
        const int n = combat::cast(b, mrules->tables, i, sp, *fs, tg, nt, rng, lines, 24, pw);
        spell_pages(i, lines, n, *fs, false);
        return;
    }
    if (!combat::spell_targets(b, fg->t, mrules->tables, i, *fs, sp, pw, rng, tg, &nt) || nt <= 0) return;
    const int ax = b.f[tg[0]].x, ay = b.f[tg[0]].y;
    if (fs->does == combat::SpellDoes::Fear || fs->does == combat::SpellDoes::Cone)
        nt = combat::cone(b, fg->t, i, ax, ay, 6, fs->does == combat::SpellDoes::Fear ? 3 : 2, tg, combat::kMaxFighters);
    add_page(i, fw(profile::kCastsASpell));
    Fx e;
    e.kind = Fx::Fly;
    e.fl = combat::spell_flight(5, 30);
    e.x0 = static_cast<uint8_t>(b.f[i].x);
    e.y0 = static_cast<uint8_t>(b.f[i].y);
    e.x1 = static_cast<uint8_t>(ax);
    e.y1 = static_cast<uint8_t>(ay);
    e.sound = sound::kSpell;
    page_fx_set(0, e);
    combat::SpellLine lines[24];
    rolls_clear();
    const int n = combat::cast(b, mrules->tables, i, sp, *fs, tg, nt, rng, lines, 24, pw);
    spell_pages(i, lines, n, *fs, false);
}

// A party member drained of a level (Pool): their class values worked out
// again - THAC0, saves, spell slots, thief skills; a fighter's attacks (3
// past 6th level, else 2: Pool's ovr24 routine)
void drained_member(int who)
{
    const combat::Fighter& f = fg->b.f[who];
    if (f.member < 0 || f.member >= pt->count) return;
    const bool had = mrules != nullptr;
    if (!had && !load_rules(mrules)) return;
    party::Character& ch = pt->m[f.member];
    classes::class_bonuses(ch, mrules->tables);
    if (names) rules::recalc(ch, *names, d->facts);
    if (!had) end_magic();
}

// Darkness's own level drain (its GAME.OVR 0x1CF3B; combat::g_dk_drain): the
// record's work by create::dk_drain - a party member's own (with its best
// levels / experience / HP), a monster's whole record; the hit dice from
// the program (DS hp_count / hp_dice). 0 lives, 1 collapses, 2 dies
bool read_hp_facts(create::Facts& fa)
{
    const auto& pc = d->prof->create;
    plat::File f;
    if (!pc.ds_image || !open_file(d->prof->program, f)) return false;
    bool ok;
    {
        plat::FileSource src(f);
        exepack::Info info;
        ok = exepack::parse(src, info) == exepack::Status::Ok &&
             exepack::read(src, info, pc.ds_image + pc.hp_count, fa.hp_count, 8) == exepack::Status::Ok &&
             exepack::read(src, info, pc.ds_image + pc.hp_dice, fa.hp_dice, 8) == exepack::Status::Ok;
    }
    f.close();
    return ok;
}

__attribute__((noinline)) int dk_drain_hook(combat::Battle&, int who, int n)
{
    combat::Fighter& f = fg->b.f[who];
    const bool had = mrules != nullptr;
    if (!had && !load_rules(mrules)) {
        memlog::note("drain: no memory for the rule tables");
        return 0;
    }
    int res = 0;
    create::Facts fa{};
    if (read_hp_facts(fa)) {
        create::DrainResult r = create::DrainResult::Lives;
        if (f.member >= 0 && f.member < pt->count) {
            r = create::dk_drain(pt->m[f.member], mrules->tables, fa, rng, n);
        } else if (combat::Monster* m = f.rec.monster()) {
            uint8_t* w = m->whole();
            party::Character* tmp = w ? new (std::nothrow) party::Character : nullptr;
            if (tmp) {
                memcpy(tmp->rec, w, party::kRecordSize);
                r = create::dk_drain(*tmp, mrules->tables, fa, rng, n);
                memcpy(w, tmp->rec, party::kRecordSize);
                delete tmp;
            } else {
                memlog::note("drain: no memory for the monster's record");
            }
        }
        res = r == create::DrainResult::Dies ? 2 : r == create::DrainResult::Collapses ? 1 : 0;
    }
    if (!had) end_magic();
    return res;
}

// What the engine says happened besides the plain blows (monster_fx_facts.md),
// as pages: the special's words, its picture flying, the results
void event_words(const combat::Event& e);
// An event's page, with its rolls (CYD_COMBAT_ROLLS): the save of the one it
// tells of, the damage as rolled
void event_page(const combat::Event& e)
{
    const int before = fg->pages;
    event_words(e);
    if (fg->rolls && fg->pages != before) {
        page_roll(take_roll(combat::RollKind::Save, e.who, 0xFE));
        if (e.ev == combat::Ev::Damage || e.ev == combat::Ev::Unaffected)
            page_roll(take_roll(combat::RollKind::Damage, e.who, 0xFF));
    }
    // (the Dark Queen: a caster hurt loses the spell under way - res 0x8F04)
    if (e.ev == combat::Ev::Damage && fg->pages != before) q_lost_spell(e.who, false);
}
void event_words(const combat::Event& e)
{
    char t[48];
    const int who = e.who;
    auto said = [&](int word) { add_page(who, fw(word)); };
    auto fallen = [&](int word) {
        add_page(who, fw(word));
        page_fx_set(1, skull_fx(who));
    };
    switch (e.ev) {
    case combat::Ev::Engulfs:
    case combat::Ev::Hugs:
    case combat::Ev::DrainsBlood:
    case combat::Ev::Constricts: {
        char nm[20];
        fighter_name(fg->b.f[e.to >= 0 ? e.to : who], nm, sizeof nm);
        const int w = e.ev == combat::Ev::Engulfs ? profile::kEngulfs
                      : e.ev == combat::Ev::Hugs ? profile::kHugs
                      : e.ev == combat::Ev::DrainsBlood ? profile::kDrainsBloodGw
                                                         : profile::kConstrictsGw;
        snprintf(t, sizeof t, "%s%s", fw(w), nm);
        add_page(who, t);
        // (the Dark Queen's "engulfs %s" / "constricts %s": res 0xB14E at row
        // 12 with a wait - DQK.EXE 0x5181C / 0x529E9 -, after the blow's
        // page, whose end cleared the box - 0x25330)
        if (hl() && !fg->page_dropped && (e.ev == combat::Ev::Engulfs || e.ev == combat::Ev::Constricts)) {
            Page& p = fg->page[fg->pages - 1];
            p.row = 12;
            p.clears = p.blank = true;
        }
        break;
    }
    case combat::Ev::BreathesAcid: said(profile::kBreathesAcid); break;
    case combat::Ev::BreathesFire: said(profile::kBreathesFire); break;
    case combat::Ev::HoundFire: said(profile::kBreathesFireH); break;
    case combat::Ev::ThrowsLightning: said(profile::kThrowsLightning); break;
    case combat::Ev::SpitsAcid: said(profile::kSpitsAcid); break;
    case combat::Ev::SpitsMisses: said(profile::kSpitsMisses); break;
    case combat::Ev::Gazes: said(profile::kGazes); break;
    case combat::Ev::BreathesCold: said(profile::kBreathesCold); break;
    case combat::Ev::BreathesPoison: said(profile::kBreathesPoison); break;
    case combat::Ev::BreathesLightning: said(profile::kBreathesLightning); break;
    case combat::Ev::Swallowed:
        fallen(profile::kSwallowed);
        // Secret's remorhaz (GAME.OVR 0x128F1): then "NAME is killed", and
        // half the time "REMORHAZ belches in satisfaction", else "NAME is
        // digested instantly"
        if (fw(profile::kBelches)[0]) {
            said(profile::kIsKilled);
            if (e.to >= 0 && rng.roll(100, 1) <= 50) add_page(e.to, fw(profile::kBelches));
            else said(profile::kDigested);
        }
        break;
    case combat::Ev::StaresBlankly: said(profile::kStaresBlankly); break;
    case combat::Ev::RayDisintegrate: said(profile::kRayDisint); break;
    case combat::Ev::RayStone: said(profile::kRayStone); break;
    case combat::Ev::RayDeath: said(profile::kRayDeath); break;
    case combat::Ev::RayWounds: said(profile::kWoundsYou); break;
    case combat::Ev::Fly: {
        // On the last page: its first free picture
        if (!fg->pages || fg->page_dropped) break;
        Fx f;
        f.kind = Fx::Fly;
        f.fl = combat::spell_flight(e.pic, e.delay);
        if (e.frames == 1) f.fl.frames = 1;
        f.x0 = static_cast<uint8_t>(fg->b.f[who].x);
        f.y0 = static_cast<uint8_t>(fg->b.f[who].y);
        f.x1 = e.x;
        f.y1 = e.y;
        if (e.pic == 6 && !e.kind) f.sound = sound::kLightning;     // (kind 1: silent - Darkness's gazes)
        Page& p = fg->page[fg->pages - 1];
        p.fx[p.fx[0].kind == Fx::None ? 0 : 1] = f;
        break;
    }
    case combat::Ev::Avoids: said(profile::kAvoidsIt); break;
    case combat::Ev::Poisoned: said(profile::kIsPoisoned); break;
    case combat::Ev::Paralyzed: said(profile::kIsParalyzedC); break;
    case combat::Ev::ParalyzedLow: said(profile::kIsParalyzedL); break;
    case combat::Ev::Reflects:
        said(profile::kReflects);
        // (the Dark Queen's: at row 12 under the gaze's page, no wait between -
        // DQK.EXE 0x4EAEF / 0x4FBB7 / 0x59958)
        if (hl() && !fg->page_dropped) fg->page[fg->pages - 1].row = 12;
        break;
    case combat::Ev::Stoned: fallen(profile::kIsStonedR); break;
    case combat::Ev::GazeStoned: fallen(profile::kIsStonedG); break;
    case combat::Ev::Disintegrated: fallen(profile::kIsDisint); break;
    case combat::Ev::Killed:
        fallen(profile::kIsKilled);
        bozak_burst(who);
        break;
    case combat::Ev::Suffocates: fallen(profile::kSuffocates); break;
    case combat::Ev::Damage:
        damage_text(e.amount, e.kind, t, sizeof t);
        add_page(who, t);
        page_fx_set(1, hit_fx(who, false));
        break;
    case combat::Ev::Unaffected: said(profile::kIsUnaffectedS); break;
    case combat::Ev::Down: {
        if (fg->b.f[who].escaped) {
            add_page(who, fw(profile::kGaseous));
            break;
        }
        const combat::Down dn = combat::down_state(fg->b.f[who]);
        if (hl()) q_fall(who, dn);
        else if (dn == combat::Down::Dead) add_page(who, fw(profile::kIsKilled));
        else if (dn == combat::Down::Dying) add_page(who, fw(profile::kGoesDownS), fw(profile::kIsDying));
        else add_page(who, fw(profile::kGoesDownS));
        page_fx_set(1, skull_fx(who));
        bozak_burst(who);
        break;
    }
    case combat::Ev::StandsUp:
    case combat::Ev::GetsUp: {
        add_page(who, fw(e.ev == combat::Ev::StandsUp ? profile::kStandsUp : profile::kGetsBackUp));
        Fx r;
        r.kind = Fx::Redraw;
        page_fx_set(1, r);
        break;
    }
    case combat::Ev::Cast: special_cast(who, e.amount, e.to); break;
    // Darkness's monsters (R14)
    case combat::Ev::Terrified: said(profile::kTerrifiedDk); break;
    case combat::Ev::Beheaded: fallen(profile::kBeheadedDk); break;
    case combat::Ev::SlainBolt: fallen(profile::kSlainBoltDk); break;
    case combat::Ev::ParaGazes: said(profile::kParaGazeDk); break;
    case combat::Ev::RayStoned: fallen(profile::kRayStonedDk); break;
    case combat::Ev::VampGazes: said(profile::kGazesCharm); break;
    case combat::Ev::Dispelled: fallen(profile::kIsDispelled); break;
    case combat::Ev::ResistsDispel: said(profile::kResistsDispel); break;
    case combat::Ev::Zapped: said(profile::kGetsZapped); break;
    case combat::Ev::BoltFly: {
        // Thrown lightning: to the square, then along the bolt
        if (!fg->pages || fg->page_dropped) break;
        Fx f;
        f.kind = Fx::Bolt;
        f.fl = combat::spell_flight(e.pic, 50);
        f.x0 = static_cast<uint8_t>(fg->b.f[who].x);
        f.y0 = static_cast<uint8_t>(fg->b.f[who].y);
        f.x1 = e.x;
        f.y1 = e.y;
        f.sound = sound::kLightning;
        Page& p = fg->page[fg->pages - 1];
        p.fx[p.fx[0].kind == Fx::None ? 0 : 1] = f;
        break;
    }
    case combat::Ev::DiesPoison: fallen(profile::kDiesFromPoison); break;
    // Pool's monsters
    case combat::Ev::Drained: {
        char lv[16];
        if (e.amount > 1) snprintf(lv, sizeof lv, "%d%s", e.amount, fw(profile::kNLevels));
        else snprintf(lv, sizeof lv, "%s", fw(profile::kDrainLevel));
        snprintf(t, sizeof t, "%s%s", fw(profile::kIsDrained), lv);
        add_page(who, t);
        drained_member(who);
        break;
    }
    case combat::Ev::DeadForever: fallen(profile::kDeadForever); break;
    // Darkness's drain: "gets drained" (the class values again), "dies from
    // drainning", "collapses from drainning"
    case combat::Ev::DkDrained:
        said(profile::kIsDrained);
        drained_member(who);
        break;
    case combat::Ev::DkDrainDies: fallen(profile::kDeadForever); break;
    case combat::Ev::DkDrainCollapses: fallen(profile::kCollapsesDrainDk); break;
    case combat::Ev::Diseased: said(profile::kIsDiseased); break;
    case combat::Ev::SucksBlood: said(profile::kSucksBlood); break;
    case combat::Ev::HeldFast: said(profile::kHeldFast); break;
    case combat::Ev::Rakes: said(profile::kRakes); break;
    case combat::Ev::CharmGaze: said(profile::kGazesCharm); break;
    case combat::Ev::Charmed: said(profile::kIsCharmedG); break;
    case combat::Ev::Breathes: said(profile::kBreathesB); break;
    case combat::Ev::FearParalyzed: said(profile::kFearParalyzed); break;
    case combat::Ev::FearWeakened: said(profile::kFearWeakened); break;
    case combat::Ev::Weakened: said(profile::kIsWeakened); break;
    case combat::Ev::Regenerates: said(profile::kRegeneratesGw); break;
    case combat::Ev::FallsDead: fallen(profile::kFallsDeadGw); break;
    case combat::Ev::Ages:
        said(profile::kAges);
        age_check = true;               // (old age is looked at when the fight ends - Treasures' [0x9318])
        break;
    case combat::Ev::Regen: {
        char w[24];
        cw(fg->b.f[who].hp() >= fg->b.f[who].hp_max() ? profile::kFullyHealed : profile::kPartlyHealed, w, sizeof w);
        add_page(who, w);
        break;
    }
    }
}

// An attack and what it did, as pages. A shot (`ammo`: the item that flies,
// -1 none) flies first: its picture and sound on the first page. The
// weapon's attacks made (each shot uses a missile).
void attack_pages(int a, int t, const combat::Attack& at, bool behind, bool shot, const Fx& fly);
int do_attack(pic::Canvas& c, int a, int t, bool behind, bool shot = false, int ammo = -1)
{
    combat::Battle& b = fg->b;
    if (!on_screen(a) || !on_screen(t)) {
        focus(t, 3);
        draw_fight(c);
    }
    draw_icon(c, a, true);
    dirty(kViewY, kViewY + 7 * kSq);
    // (the originals' attack: the attack frame, then Delay(100) - fixed, not
    // the game speed's - before the blow's messages: Curse GAME.OVR 0x14E59,
    // Pool 0x14132, Secret 0x1670B, Darkness 0x20013, the same in the others)
    fg->attack_pause = true;
    Fx fly;
    if (shot) {
        const combat::Fighter& f = b.f[a];
        fly = fly_fx(a, t, combat::shot_flight(b, f, ammo, combat::direction(f.x, f.y, b.f[t].x, b.f[t].y)));
        fly.sound = fly.fl.sound;
    }
    rolls_clear();
    const combat::Attack at = combat::attack(b, a, t, names, rng, behind);
    attack_pages(a, t, at, behind, shot, fly);
    return at.shots;
}

// An attack's pages: each hit ("Attacks ... Hitting for N points of
// damage") and what it did besides, a miss, the target going down
void attack_pages(int a, int t, const combat::Attack& at, bool behind, bool shot, const Fx& fly)
{
    char line[48];
    bool first = true;
    // With the rolls shown (CYD_COMBAT_ROLLS) and more than one blow (darts'
    // three, a fighter's two ...): every blow its own page in turn - a miss
    // too ("Misses", its roll), each shot flying - so the player sees each
    // one (Tom, 2026-11-06: "only 1 dart thrown"). The originals page the
    // hits only, a miss just when nothing hit; play is the same either way.
    bool each = fg->rolls && at.n > 1 && !at.slain;
    for (int k = 0; each && k < at.n; ++k)
        if (at.image[k]) each = false;          // (Secret's images: the original's pages)
    // The sounds: a shot's flight, then a blow each hit, a swish for no hit at all
    auto hit_page = [&]() {
        if ((first || each) && shot) {
            page_fx_set(0, fly);
            Fx blow;
            blow.sound = sound::kHit;
            page_fx_set(1, blow);
        } else {
            page_sound(sound::kHit);
        }
        first = false;
    };
    int hit_pg = -1;                    // (the last hit's page: a fall goes on below it)
    // The target's name is printed before the blow's damage (PoolRad ovr14
    // +0x372, the damage at +0x484): in its colour while still standing
    auto target_up = [&]() {
        if (fg->page_dropped || hl()) return;
        Page& p = fg->page[fg->pages - 1];
        for (int k = 0; k < p.lines; ++k)
            if (k > 0 && (p.names & (1u << k))) p.colour[k] = fg->b.f[t].team() ? 14 : 11;
    };
    if (at.slain) {
        add_page(a, fw(profile::kSlays), fw(profile::kCruelBlow), t);
        target_up();
        hit_pg = fg->page_dropped ? -1 : fg->pages - 1;
        hit_page();
    }
    for (int k = 0; k < at.n && !at.slain; ++k) {
        if (at.image[k]) {
            // Secret: a mirror image took the blow - "lost an image"
            add_page(t, fw(profile::kLostImage));
            page_roll(take_roll(combat::RollKind::Hit, t, k));
            first = false;
            continue;
        }
        if (at.avoided[k]) {
            // A missile dodged: "Avoids it"
            add_page(t, fw(profile::kAvoidsIt));
            page_roll(take_roll(combat::RollKind::Hit, t, k));
            if (first && shot) page_fx_set(0, fly);
            first = false;
            continue;
        }
        if (!at.hits[k].hit) {
            if (each) {
                add_page(a, fw(at.backstab ? profile::kBackstabs : profile::kAttacksW), fw(profile::kMisses), t);
                page_roll(take_roll(combat::RollKind::Hit, t, k));
                if (shot) page_fx_set(0, fly);
                else page_sound(sound::kMiss);
                first = false;
            }
            continue;
        }
        const int dmg = at.hits[k].damage;
        snprintf(line, sizeof line, "%s%s%d%s%s", behind ? fw(profile::kFromBehind) : "", fw(profile::kHittingFor), dmg,
                 dmg == 1 ? fw(profile::kPointW) : fw(profile::kPointsW), fw(profile::kOfDamage));
        add_page(a, fw(at.backstab ? profile::kBackstabs : profile::kAttacksW), line, t);
        target_up();
        hit_pg = fg->page_dropped ? -1 : fg->pages - 1;
        if (at.hits[k].weapon && !fg->page_dropped) fg->page[fg->pages - 1].weapon = at.hits[k].weapon;
        page_roll(take_roll(combat::RollKind::Hit, t, k));
        page_roll(take_roll(combat::RollKind::Damage, t, k));
        hit_page();
        // What the hit did besides (poison, paralysis, a touch's damage, engulfing ...)
        // (the Dark Queen: the blow's page, then "lost a spell" - DQK.EXE 0x2522F)
        if (dmg > 0 && hit_pg >= 0 && q_lost_spell(t, true)) hit_pg = fg->pages - 1;
        for (int j = 0; j < at.n_ev; ++j)
            if (at.ev[j].hit == k) event_page(at.ev[j]);
    }
    if (first) {
        add_page(a, fw(at.backstab ? profile::kBackstabs : profile::kAttacksW), fw(profile::kMisses), t);
        page_roll(take_roll(combat::RollKind::Hit, t, 0xFE, true));      // (the last miss)
        if (shot) page_fx_set(0, fly);
        else page_sound(sound::kMiss);
    }
    // (swallowed whole: its own pages said it - "is killed" among them)
    bool swallowed = false;
    for (int j = 0; j < at.n_ev; ++j) swallowed = swallowed || at.ev[j].ev == combat::Ev::Swallowed;
    if (at.down && !swallowed) {
        down_page(t, hit_pg >= 0 && hit_pg == fg->pages - 1);
        if (!shot) baaz_weapon(a, t);
    }
}

// After an attack's pages: the turn ends unless attacks are left and the
// target went down (then pick another)
void after_attack(pic::Canvas& c)
{
    draw_fight(c);
    const combat::Fighter& f = fg->b.f[fg->cur];
    if (!f.up() || !f.size) {           // (down or gone meanwhile - free attacks, a zap)
        end_turn(c);
        return;
    }
    const bool left = f.attacks[0] > 0 || f.attacks[1] > 0;
    const bool target_down = f.target >= 0 && !fg->b.f[f.target].up();
    if (combat::standing(fg->b, 0) == 0 || combat::standing(fg->b, 1) == 0) {
        end_turn(c);
        return;
    }
    if (left && target_down) {
        if (f.quick || f.team()) {
            fg->st = FSt::Ai;
            fg->until = 1;                  // (no wait of the originals' here)
        } else {
            fight_menu(c);
        }
        return;
    }
    end_turn(c);
}

// Leaving an enemy's side: each enemy next to them now and not after the
// step gets its attacks (from behind)
void free_attacks(pic::Canvas& c, int i, int dir)
{
    combat::Battle& b = fg->b;
    combat::Fighter& f = b.f[i];
    const int my = f.team() ? 1 : 0;
    for (int e = 0; e < b.n; ++e) {
        combat::Fighter& en = b.f[e];
        if (!en.up() || !en.size || (en.team() ? 1 : 0) == my || !combat::adjacent(b, i, e)) continue;
        // Still next to them after the step?
        f.x += combat::dx(dir);
        f.y += combat::dy(dir);
        const bool still = combat::adjacent(b, i, e);
        f.x -= combat::dx(dir);
        f.y -= combat::dy(dir);
        if (still || !combat::free_attack_ok(b, fg->t, e, i)) continue;
        // None left: one (in slot 2 for those with no slot 1 attacks - the spiders; coab's facts)
        if (en.attacks[0] <= 0 && en.attacks[1] <= 0) en.attacks[en.rec[0x11C] == 0 ? 1 : 0] = 1;
        do_attack(c, e, i, true);
        if (!f.up()) return;
    }
}

// Guards next to the new square attack
void guard_attacks(pic::Canvas& c, int i)
{
    combat::Battle& b = fg->b;
    const int my = b.f[i].team() ? 1 : 0;
    for (int e = 0; e < b.n && b.f[i].up(); ++e) {
        combat::Fighter& en = b.f[e];
        if (!en.guarding || !en.up() || (en.team() ? 1 : 0) == my || !combat::adjacent(b, i, e)) continue;
        // (asleep, held, paralysed or stunned since it chose to guard: no
        // blow - Tom, 2026-11-01; the guard is lost with it)
        if (combat::helpless(b, en) || combat::stunned(b, en)) {
            en.guarding = false;
            continue;
        }
        en.guarding = false;
        // (with the attacks it has left: none, no blow - the original's)
        if (en.attacks[0] <= 0 && en.attacks[1] <= 0) continue;
        do_attack(c, e, i, false);
    }
}

// A blow in melee: a fighter's sweep of the weak ones next to them ("sweeps":
// one attack on each, and the turn is over), else the attack
bool sweep_next(pic::Canvas& c);
void melee(pic::Canvas& c, int a, int t)
{
    combat::Battle& b = fg->b;
    combat::Fighter& f = b.f[a];
    int sw[8];
    const int n = combat::adjacent(b, a, t) ? combat::sweep(b, a, t, sw, 8) : 0;
    if (!n) {
        do_attack(c, a, t, false);
        return;
    }
    f.swept = true;
    plat::log("[play] fighter %d sweeps %d\n", a, n);
    add_page(a, fw(profile::kSweeps));
    // (one blow now; the others each after the pages before it - the
    // original's order: hit, fall, skull, the icon gone, then the next -
    // Tom, 2026-11-06)
    fg->sweep_by = static_cast<int8_t>(a);
    fg->sweep_n = static_cast<uint8_t>(n);
    fg->sweep_at = 0;
    for (int k = 0; k < n; ++k) fg->sweep_t[k] = static_cast<int8_t>(sw[k]);
    sweep_next(c);
}

// A sweep's next blow, at the next one still standing; false: the sweep
// is over (the sweeper's attacks gone)
bool sweep_next(pic::Canvas& c)
{
    combat::Battle& b = fg->b;
    if (fg->sweep_by < 0) return false;
    combat::Fighter& f = b.f[fg->sweep_by];
    bool did = false;
    while (fg->sweep_at < fg->sweep_n && f.up() && f.size) {
        const int t = fg->sweep_t[fg->sweep_at++];
        if (!b.f[t].up()) continue;
        f.attacks[0] = 1;
        f.attacks[1] = 0;
        CYD_TEST_FX(SweepBlow, 0, fg->sweep_at);
        do_attack(c, fg->sweep_by, t, false);
        did = true;
        break;
    }
    f.attacks[0] = f.attacks[1] = 0;
    if (!did || fg->sweep_at >= fg->sweep_n) {
        fg->sweep_by = -1;
        fg->sweep_n = fg->sweep_at = 0;
    }
    return did;
}

void forget_spell(combat::Fighter& f, int sp);
bool fight_miscast(pic::Canvas& c, int sp);
void remember_spell(combat::Fighter& f, int sp);
void aim_exit(pic::Canvas& c);
void strike(pic::Canvas& c, int t);
void begin_targeting(pic::Canvas& c, int sp);
void cast_now(pic::Canvas& c, bool forgotten = false);
bool blast_asked(int sp);
void ask_blast_delay(pic::Canvas& c);
void blast_off(Fight::Blast& bl);
const combat::FightSpell* fight_spell(int sp);

// A monster's items before it changes one (copy on write): the copies of a
// group read its one list (v0.138.0's memory saving) until one shoots,
// uses a charge or readies another weapon - then that one gets a list of
// its own (each copy has its own items in the originals). False: no memory
// (the change goes into the shared list, as before)
// Another fighter reads f's list too
bool items_shared(const combat::Fighter& f)
{
    for (int j = 0; fg && j < fg->b.n; ++j)
        if (&fg->b.f[j] != &f && fg->b.f[j].items == f.items) return true;
    return false;
}

bool own_items(combat::Fighter& f)
{
    if (f.member >= 0 || f.monster < 0 || !ml || f.monster >= ml->n || !f.items) return true;
    combat::Monster& m = ml->m[f.monster];
    if (m.items && f.items == m.items) {
        if (!items_shared(f)) return true;
        // (its list is read by copies that readied the same: it becomes one
        // of theirs, and this one gets a list of its own below)
        for (int j = 0; j < fg->b.n; ++j) {
            const combat::Fighter& o = fg->b.f[j];
            if (&o == &f || o.items != m.items || o.monster < 0 || o.monster >= ml->n || ml->m[o.monster].items) continue;
            ml->m[o.monster].items = m.items;
            ml->m[o.monster].n_items = m.n_items;
            break;
        }
        m.items = nullptr;
    }
    const int n = f.n_items > 0 ? f.n_items : 0;
    const size_t sz = static_cast<size_t>(n > 0 ? n : 1) * items::kRecordSize;
    // (the arena's script part when it has room - nothing more on the heap;
    // off it before the arena goes: monster_items_off_arena)
    void* a = arena_take(sz, true);
    auto* p = static_cast<uint8_t(*)[items::kRecordSize]>(a ? a : malloc(sz));
    if (!p) {
        memlog::note("not enough memory: a monster's own items");
        return false;
    }
    if (n) memcpy(p, f.items, static_cast<size_t>(n) * items::kRecordSize);
    arena_free(m.items);
    m.items = p;
    m.n_items = static_cast<uint8_t>(n);
    f.items = p;
    return true;
}

// A fighter's items as a fight starts: a monster's own list once it has one,
// else its group's
void monster_items(combat::Fighter& f, const combat::Monster& m)
{
    combat::Group& g = *ml->g[m.group];
    f.items = m.items ? m.items : g.item;
    f.n_items = m.items ? m.n_items : g.n_items;
}

// A monster's item gone: off its own list (made from its group's first)
void remove_group_item(combat::Fighter& f, int k)
{
    if (!f.items || k < 0 || k >= f.n_items) return;
    own_items(f);
    uint8_t (*list)[items::kRecordSize] = f.items;
    const int n = f.n_items;
    for (int j = k; j + 1 < n; ++j) memcpy(list[j], list[j + 1], items::kRecordSize);
    memset(list[n - 1], 0, items::kRecordSize);
    f.n_items = static_cast<int16_t>(n - 1);
    if (ml && f.monster >= 0 && f.monster < ml->n && ml->m[f.monster].items == list)
        ml->m[f.monster].n_items = static_cast<uint8_t>(n - 1);
}

// Shots: `n` arrows / quarrels of the pile (one for each attack made), or
// the thrown weapon, go (the pile's count less n; none left - or a single
// one: it's gone)
void spend_ammo(int i, int k, int n)
{
    combat::Fighter& f = fg->b.f[i];
    if (k < 0 || k >= f.n_items || n < 1) return;
    if (f.member >= 0) {
        party::Character& ch = pt->m[f.member];
        const int left = ch.items[k][0x39];
        if (left > n) {
            ch.items[k][0x39] = static_cast<uint8_t>(left - n);
        } else {
            // A thrown weapon that also strikes (flags 4 and 0x10: a spear, a
            // hand axe ...) lands on the field: the treasure after the fight;
            // the rest (darts, javelins, flasks) are gone, as is one with
            // effect 0x89 (the original's test, listing ovr014:1BD4-1C62)
            const items::TypeInfo& ti = names->type(ch.items[k][0x2E]);
            const bool lands = (ti.flags & 0x14) == 0x14 && ch.items[k][0x3E] != 0x89;
            if (lands && fg->n_thrown < 6) {
                memcpy(fg->thrown[fg->n_thrown], ch.items[k], items::kRecordSize);
                fg->thrown[fg->n_thrown][0x34] = 0;
                ++fg->n_thrown;
            }
            rules::remove_item(ch, k);
        }
        f.n_items = ch.n_items;
        rules::recalc(ch, *names, d->facts);
        return;
    }
    const int left = f.items[k][0x39];
    if (left > n) {
        own_items(f);
        f.items[k][0x39] = static_cast<uint8_t>(left - n);
    } else {
        remove_group_item(f, k);
    }
}

// ---- Use in a fight: the item's spell goes off at once (no casting time),
// at the item's level; using it ends the turn (even when aborted)
void spend_item(int i, int sp)
{
    const int k = fg->use_item;
    fg->use_item = -1;
    combat::Fighter& f = fg->b.f[i];
    if (k < 0 || k >= f.n_items) return;
    if (f.member >= 0) {
        party::Character& ch = pt->m[f.member];
        if (fg->use_scroll) magic::scroll_used(ch, scroll_facts(), k, sp);
        else magic::used(ch, k);
        rules::recalc(ch, *names, d->facts);
        f.n_items = ch.n_items;
        return;
    }
    // A monster's: its own items (its group's until now)
    own_items(f);
    if (magic::use_charge(f.items[k])) remove_group_item(f, k);
}

// "NAME uses an item", "Item: NAME" on row 23
void announce_item(pic::Canvas& c, int i, int k)
{
    const combat::Fighter& f = fg->b.f[i];
    CYD_TEST_ACT(fg->b, i, UseItem, -1, k);
    char w1[16], nm[40], t[48];
    cw(profile::kUsesItem, w1, sizeof w1);
    add_page(i, w1);
    cw(profile::kItemColon, t, sizeof t);
    names->name(items::Item{f.items[k]}, nm, sizeof nm);
    strncat(t, nm, sizeof t - strlen(t) - 1);
    status_line(c, t);
}

void fight_use(int k, int sp, bool scroll, pic::Canvas& c)
{
    items_direct = false;
    screen = Screen::Fight;
    palette_fight(true);
    const int i = fg->cur;
    combat::Fighter& f = fg->b.f[i];
    if (f.member >= 0) f.n_items = pt->m[f.member].n_items;
    draw_fight(c);
    if (!f.can_use) {                   // (silenced, coughing: no items this round)
        fight_menu(c);
        return;
    }
    fg->use_item = k;
    fg->use_scroll = scroll;
    fg->spell = sp;
    fg->sp_n = 0;
    fg->sp_area = -1;
    const magic::ScrollReader rd =
        scroll && f.member >= 0 ? magic::scroll_reader(pt->m[f.member], mrules->tables) : magic::kReads;
    if (rd == magic::kOops || (rd == magic::kD100 && rng.roll(100, 1) > 75)) {
        char w1[12];
        cw(profile::kOops, w1, sizeof w1);
        add_page(i, w1);
        spend_item(i, sp);
        fg->spell = 0;
        say(c, FSt::Next);
        return;
    }
    const combat::FightSpell* fs = fight_spell(sp);
    const bool camp_only = spells::entry(mrules->tables, sp).when == 0;
    if (!fs || fs->does == combat::SpellDoes::NotYet || camp_only) {
        fg->use_item = -1;
        fg->spell = 0;
        fight_menu(c);
        if (camp_only) note(c, fw(profile::kCampOnly));
        else error(c, "Not in the engine yet.");
        return;
    }
    if (scroll) {
        begin_targeting(c, sp);
        return;
    }
    announce_item(c, i, k);
    say(c, FSt::UseAim);
}

// Treasures' own items in a fight (play.cpp tr_own_item): 0x11 - the
// effect on the user as outside fights, on a page; the turn ends. 0x0F -
// the jump (GAME.OVR 0x2DBF3, fights only): a square aimed at with spell
// 0x7F's aim (Dimension Door's row: aim 8, range 4 - its call [0xA22A](
// result, 0, 0x7F)), then tr_jump. (0x10's map: as outside fights, back to
// the items screen, nothing used, the turn goes on - play.cpp tr_own_item)
constexpr int kTrJumpAim = 0x7F;
void tr_fight_item(int k, pic::Canvas& c)
{
    items_direct = false;
    screen = Screen::Fight;
    palette_fight(true);
    const int i = fg->cur;
    combat::Fighter& f = fg->b.f[i];
    if (f.member >= 0) f.n_items = pt->m[f.member].n_items;
    draw_fight(c);
    if (!f.can_use || f.member < 0 || k < 0 || k >= f.n_items) {
        fight_menu(c);
        return;
    }
    const uint8_t* it = f.items[k];
    if (it[0x3E] == 0x0F) {
        fg->use_item = k;
        fg->use_scroll = false;
        fg->jump = true;
        begin_targeting(c, kTrJumpAim);
        return;
    }
    if (it[0x3E] != 0x11) {
        fight_menu(c);
        error(c, "Not in the engine yet.");
        return;
    }
    char t[64];
    party::Character& me = pt->m[f.member];
    const bool used = tr_effect_item(me, it, t, sizeof t);
    // (the page's name is the user's: the words after it)
    char nm[20];
    me.name(nm, sizeof nm);
    const size_t n = strlen(nm);
    if (t[0]) add_page(i, strncmp(t, nm, n) == 0 && t[n] == ' ' ? t + n + 1 : t);
    if (used) {
        fg->use_item = k;
        fg->use_scroll = false;
        spend_item(i, 0);
    }
    say(c, FSt::Next);
}

// The jump's square picked (GAME.OVR 0x2DBF3 after its aim): someone
// standing there (0x150:0x48) - "NAME cannot go there"; else the user's
// effect 0x9C (variant.tr_held) ended - "is free to move" -, the user moved
// there (0x150:0x75 / 0x4D off its square, 0x84 onto the new one) - "jumps".
// Either way the result is 1: a charge goes (0x2E0EF +0x2C5: the count's,
// else 0x3C's) and the turn is over. The original looks at nobody but who
// stands there - a wall or the field's dark squares too (Game Fixes: only a
// square the jumper can stand on, Dimension Door's rule)
void tr_jump(pic::Canvas& c)
{
    combat::Battle& b = fg->b;
    const int i = fg->cur;
    combat::Fighter& f = b.f[i];
    const int x = fg->aim_x, y = fg->aim_y;
    fg->jump = false;
    fg->spell = 0;
    const auto& v = d->prof->variant;
    char w[32];
    draw_fight(c);
    const bool moved = x >= 0 && y >= 0 &&
                       (rules::fixes() ? combat::teleport(b, fg->t, i, x, y) : combat::jump_to(b, i, x, y));
    if (moved) {
        // (moved first here, said after: the pages show it in its new square)
        if (combat::lose_aff(f, v.tr_held)) {
            ow(v.tr_item_words[2], w, sizeof w);
            add_page(i, w);
        }
        ow(v.tr_item_words[3], w, sizeof w);
        add_page(i, w);
        Fx r;
        r.kind = Fx::Redraw;
        page_fx_set(1, r);
        focus(i, 2);
    } else {
        ow(v.tr_item_words[1], w, sizeof w);
        add_page(i, w);
    }
    plat::log("[play] jump to %d,%d: %s\n", x, y, moved ? "jumps" : "cannot go there");
    spend_item(i, 0);
    f.attacks[0] = f.attacks[1] = 0;
    f.moves = 0;
    say(c, FSt::Next);
}

bool ai_controls(const combat::Fighter& f) { return f.quick || f.team() || f.rec[0xF7] >= 0x80; }

// A spell begun earlier this round goes off (lost if they were hurt)
void fire_pending(pic::Canvas& c)
{
    combat::Battle& b = fg->b;
    combat::Fighter& f = b.f[fg->cur];
    const int sp = f.spell;
    f.spell = 0;
    if (!f.can_cast) {
        if (!f.spell_n) forget_spell(f, sp);
        f.spell_n = 0;
        add_page(fg->cur, fw(profile::kLostSpell));
        say(c, FSt::Next);
        return;
    }
    if (fight_miscast(c, sp)) {
        if (f.spell_n) remember_spell(f, sp);
        f.spell_n = 0;
        return;
    }
    if (f.spell_n) {
        // The computer's: the targets it chose
        if (!load_magic()) {
            end_turn(c);
            return;
        }
        fg->spell = sp;
        fg->sp_n = 0;
        for (int k = 0; k < f.spell_n; ++k)
            if (b.f[f.spell_t[k]].size) fg->sp_t[fg->sp_n++] = f.spell_t[k];
        f.spell_n = 0;
        if (!fg->sp_n) {
            end_turn(c);
            return;
        }
        cast_now(c, true);
        return;
    }
    begin_targeting(c, sp);
    return;
}

void start_turn(pic::Canvas& c)
{
    combat::Battle& b = fg->b;
    combat::Fighter& f = b.f[fg->cur];
    CYD_TEST_ACT(b, fg->cur, TurnStart, -1, f.delay);
    if (!fg->fx_done) {                 // (a turn begins - not one resumed after a page)
        f.flee_last = 8;
        f.flee_back = 0;
    }
    f.received = f.turns = 0;
    f.guarding = false;
    fg->ai_checked = false;
    fg->ai_diced = false;
    fg->flee_told = false;
    fg->use_item = -1;
    fg->spell = 0;
    fg->jump = false;
    if (!f.team() || on_screen(fg->cur)) focus(fg->cur, 2);
    draw_fight(c);
    // Coughing (a stinking cloud's): no items and no spells this round
    const uint8_t cough = d->prof->fight.facts.cough;
    if (cough && f.has(cough) && !f.coughed) {
        f.coughed = true;
        f.can_use = false;
        f.can_cast = false;
        add_page(fg->cur, fw(profile::kIsCoughing));
        say(c, FSt::Resume);
        return;
    }
    // Effects at the turn's start (spell_facts.md 1.8): silenced (no
    // spells or items), fighting snakes or fumbling (the turn lost)
    if (!fg->fx_done) {
        fg->fx_done = true;                     // (once a turn, whatever was said before it)
        const combat::TurnFx tf = combat::turn_effects(b, fg->cur);
        if (tf == combat::TurnFx::Silenced) {
            add_page(fg->cur, fw(profile::kIsSilenced));
            say(c, FSt::Resume);
            return;
        }
        if (tf == combat::TurnFx::Snakes) {
            add_page(fg->cur, fw(profile::kFightingSnakes));
            say(c, FSt::Next);
            return;
        }
        if (tf == combat::TurnFx::Fumbling) {
            end_turn(c);
            return;
        }
        if (tf == combat::TurnFx::Confused) {
            add_page(fg->cur, fw(profile::kIsConfusedT));
            say(c, FSt::Next);
            return;
        }
        if (tf == combat::TurnFx::RunsAway || tf == combat::TurnFx::Berserk || tf == combat::TurnFx::Enraged ||
            tf == combat::TurnFx::Berzerk) {
            // (the turn goes on: fleeing, at the nearest one, or as ever)
            add_page(fg->cur, fw(tf == combat::TurnFx::RunsAway ? profile::kRunsAway
                                 : tf == combat::TurnFx::Berserk ? profile::kGoesBerserk
                                 : tf == combat::TurnFx::Berzerk ? profile::kGoesBerzerk
                                                                 : profile::kIsEnraged));
            say(c, FSt::Resume);
            return;
        }
        if (tf == combat::TurnFx::Suffocates) {
            // Engulfed too long
            add_page(fg->cur, fw(profile::kSuffocates));
            page_fx_set(1, skull_fx(fg->cur));
            draw_fight(c);
            say(c, FSt::Next);
            return;
        }
        // Pool's holds: the stirge drinks, the mantis bites the one it holds
        combat::Event hev[4];
        int bite = -1;
        bool go_on = false;
        rolls_clear();
        const int nh = combat::turn_holds(b, fg->cur, rng, hev, 4, &bite, &go_on);
        if (go_on) {
            // (Gateway's squid with the fixes: its squeeze, then its turn)
            if (!on_screen(fg->cur)) focus(fg->cur, 3);
            draw_fight(c);
            for (int k = 0; k < nh; ++k) event_page(hev[k]);
            draw_fight(c);
        } else if (nh > 0 || bite >= 0) {
            if (!on_screen(fg->cur)) focus(fg->cur, 3);
            draw_fight(c);
            for (int k = 0; k < nh; ++k) event_page(hev[k]);
            if (bite >= 0) {
                rolls_clear();
                const combat::Attack at = combat::bite_attack(b, fg->cur, bite, names, rng);
                attack_pages(fg->cur, bite, at, false, false, Fx{});
            }
            draw_fight(c);
            say(c, f.up() ? FSt::Resume : FSt::Next);
            return;
        }
    }
    // A spell begun earlier goes off now - the computer's (monsters, NPCs,
    // Quick) in Darkness, Treasures, Death Knights, the Dark Queen and
    // Champions only after its morale test and its items (their turn routine
    // sends a computer fighter straight to its AI, which fires the pending
    // spell after those: Death Knights GAME.OVR 0x9384 -> 0xA21B +0x141,
    // Darkness 0x9AC5 +0x15A, the Dark Queen DQK.EXE 0x2DF74, Champions
    // 0xB271 -> 0xC572, its pending spell at 0xC699 - round 11)
    if (f.spell && !(late_spell() && ai_controls(f))) {
        fire_pending(c);
        return;
    }
    if (f.quick || f.team() || f.rec[0xF7] >= 0x80) {
        fg->st = FSt::Ai;
        fg->until = plat::now_ms() + game_delay_ms();
        if (!fg->until) fg->until = 1;
        clear_menu_line(c);
        return;
    }
    fight_menu(c);
}

void next_turn(pic::Canvas& c)
{
    combat::Battle& b = fg->b;
    // (the last enemy gone - surrendered, fled, down - the round goes on:
    // those still to act have their turns, the Quick ones guard, then the
    // round's end asks "Continue Battle" - coab's MainCombatLoop; DOSBox,
    // the slums' orcs: Brother Sean's panel at the question)
    if (combat::standing(b, 0) == 0) {
        fg->cur = -1;
        fg->st = FSt::RoundEnd;
        fg->until = 1;
        return;
    }
    fg->cur = combat::next(b, rng);
    if (fg->cur < 0) {
        fg->st = FSt::RoundEnd;
        fg->until = 1;
        return;
    }
    fg->fx_done = false;
    start_turn(c);
}

void begin_round(pic::Canvas& c)
{
    combat::Event ev[8];
    rolls_clear();
    const int nev = combat::start_round(fg->b, rng, ev, 8);
    vm->set(0x7ECB, 0);                 // (the surprise word: cleared after the initiative, as the original)
    if (nev > 0) {
        // the hasted "ages" (the first round of their Haste), then the turns
        for (int k = 0; k < nev; ++k) event_page(ev[k]);
        draw_fight(c);
        say(c, FSt::Begin);
        return;
    }
    next_turn(c);
}

void round_rest(pic::Canvas& c);
void round_clouds(pic::Canvas& c);
void fight_hammer(int i);

void round_end(pic::Canvas& c)
{
    vm->advance_clock(1, 1);                // a round is a minute
    vm->take_minutes();
    // Everyone's effects a minute on; regeneration; a troll getting up
    combat::Event ev[8];
    const int nev = combat::tick(fg->b, ev, 8);
    for (int k = 0; k < nev; ++k) event_page(ev[k]);
    // Spiritual Hammer: back in hand each round while it lasts, gone after
    for (int k = 0; k < fg->b.party_size; ++k) fight_hammer(k);
    // Those standing in a poisonous cloud breathe it again
    for (int k = 0; k < fg->b.n; ++k) {
        const combat::Fighter& f = fg->b.f[k];
        if (f.up() && f.size && combat::in_poison(fg->b, f.x, f.y) && combat::breathe_poison(fg->b, k, rng)) poison_page(k);
        // (Darkness: and those in a blade barrier's knives are cut - R2)
        if (f.up() && f.size && combat::in_knives(fg->b, f.x, f.y)) knives_page(k);
        // (Champions: and those in a kapak's acid take its 1d6 - GAME.OVR 0x34F7F)
        if (f.up() && combat::on_acid(fg->b, k)) acid_page(k);
    }
    // A delayed blast fireball whose time has come
    for (auto& bl : fg->blast)
        if (bl.caster >= 0 && bl.rounds && --bl.rounds == 0) blast_off(bl);
    if (fg->pages) {
        draw_fight(c);
        say(c, FSt::Clouds);                // (then on with the round's end)
        return;
    }
    round_clouds(c);
}

// The clouds a round older: those that go clear ("The air clears a little...")
void round_clouds(pic::Canvas& c)
{
    const int gone = combat::clouds_round(fg->b);
    if (gone || (fg->b.acid_gone && d->w_acid[1][0])) {
        // "The air clears a little..." on the menu line for the game's delay
        // (Champions' acid pool: "The acid drains away..")
        draw_field(c);
        clear_menu_line(c);
        put(c, gone ? fw(profile::kAirClears) : d->w_acid[1], 0, text::kMenuRow, 10);
        fg->st = FSt::RoundRest;
        fg->until = plat::now_ms() + game_delay_ms();
        if (!fg->until) fg->until = 1;
        return;
    }
    round_rest(c);
}

void round_rest(pic::Canvas& c)
{
    const bool over = combat::end_round(fg->b);
    if (over && !in_demo && combat::standing(fg->b, 1) == 0 && combat::standing(fg->b, 0) > 1) {
        // No enemies left, more than one standing: "Continue Battle: Yes No"
        // (time to bandage and heal before the fight ends - the original's)
        fg->st = FSt::ContinueAsk;
        text::build(menu, fw(profile::kContinueBattle), rw(Data::kYesNo));
        menu.selected = 1;
        show_menu_line(c);
        return;
    }
    if (over) {
        end_fight(c);
        return;
    }
    if (combat::anyone_dying(fg->b)) {
        clear_menu_line(c);
        put(c, fw(profile::kTeammateDying), 0, text::kMenuRow, 15);
        fg->st = FSt::Begin;            // a pause, then the next round
        fg->until = plat::now_ms() + game_delay_ms() * 3;
        if (!fg->until) fg->until = 1;
        return;
    }
    begin_round(c);
}

// ---- The player's menu
void draw_fight(pic::Canvas& c);

// Darkness's menu (GAME.OVR 0x904E): one string of 12 choices on two rows
// - Move (moves left), Aim, Use (with items), Cast (a spell in memory, able
// to cast, not where game byte 0x02 forbids), Turn (a cleric, or a paladin
// past 2nd level, with undead in the fight it hasn't turned), Guard (no
// readied weapon that only shoots: range under 2, or type flags 0x14 both),
// Quick, then Delay, Bandage (someone dying), View, Speed, End - the ones
// not offered left out
bool dk_menu() { return d->prof->fight.words[profile::kFMenuDk] != 0; }

bool can_guard(const combat::Fighter& f)
{
    const int w = combat::weapon(f, *names);
    if (w < 0) return true;
    const items::TypeInfo& ti = names->type(f.items[w][0x2E]);
    return dk_menu() ? ti.range <= 1 || (ti.flags & 0x14) == 0x14 : (ti.flags & 0x04) != 0;
}

// Darkness's menus are one string each with the words not offered left out
// (its Pascal sets): word k of `all` when on[k] ('@' a row break kept)
void dk_words(const char* all, const bool* on, int n, char* words, size_t cap)
{
    size_t o = 0;
    int k = 0;
    const char* p = all;
    while (*p && k < n) {
        while (*p == ' ') ++p;
        if (*p == text::kRowBreak) {
            if (o && words[o - 1] == ' ') --o;
            if (o + 1 < cap) words[o++] = text::kRowBreak;
            ++p;
            continue;
        }
        const char* e = p;
        while (*e && *e != ' ' && *e != text::kRowBreak) ++e;
        if (on[k] && o + (e - p) + 2 < cap) {
            memcpy(words + o, p, static_cast<size_t>(e - p));
            o += static_cast<size_t>(e - p);
            words[o++] = ' ';
        }
        ++k;
        p = e;
    }
    if (o && words[o - 1] == ' ') --o;
    words[o] = 0;
}

bool kr_can_yell(const combat::Fighter& f);

void fight_menu_dk(char* words, size_t cap, const combat::Fighter& f, bool spells)
{
    bool on[13] = {};
    on[0] = f.moves > 1;
    on[1] = true;
    on[2] = f.n_items && f.can_use;
    on[3] = spells && f.can_cast && !no_magic_area();
    // (Darkness, R13: a cleric or a paladin over 2nd level, as many times
    // as undead were loaded for the fight)
    on[4] = combat::can_turn(fg->b, fg->t, f);
    on[5] = can_guard(f);
    on[6] = on[7] = on[9] = on[10] = on[11] = true;
    on[8] = combat::anyone_dying(fg->b);
    if (dkk() || hl()) {        // (the Dark Queen's string the same 13 words - DS 0x50B3)
        // (Death Knights, its GAME.OVR 0x04B +0xCCD: Yell 10th - a kender
        // while no one taunts; View Speed End after it)
        on[12] = true;
        on[9] = kr_can_yell(f);
        dk_words(fw(profile::kFMenuDk), on, 13, words, cap);
        return;
    }
    dk_words(fw(profile::kFMenuDk), on, 12, words, cap);
}

// Champions: a kender (its own race 5) may Yell while no one has (its
// GAME.OVR ovl 0x58 +0xA45: the taunter pointer DS 0x71A7 nil); Death
// Knights' the same (its Yell in the menu's string)
bool kr_can_yell(const combat::Fighter& f)
{
    return (dkk() || hl() || (krynn() && d->w_yell[0])) && f.member >= 0 && pt->m[f.member].ext[party::kExtRace] == 5 &&
           fg->b.taunter < 0 && d->prof->fight.facts.taunted;
}

// Yell (ovl 0xDB +0x28): each of the other side that is taunted says it
// "wants to kill" the kender; the kender's turn is over
void kr_yell(pic::Canvas& c)
{
    const int i = fg->cur;
    int out[24];
    const int n = combat::taunt(fg->b, i, rng, out, 24);
    char nm[20], t[48];
    fighter_name(fg->b.f[i], nm, sizeof nm);
    snprintf(t, sizeof t, "%s%s", d->w_wants, nm);
    for (int k = 0; k < n; ++k) add_page(out[k], t);
    fg->b.f[i].attacks[0] = fg->b.f[i].attacks[1] = 0;
    draw_fight(c);
    say(c, FSt::Next);
}

void fight_menu(pic::Canvas& c)
{
    const combat::Fighter& f = fg->b.f[fg->cur];
    // The one whose menu it is in view (v0.85.0: after Aim / View looked
    // elsewhere the view stayed there, the fighter off the screen - their
    // moves then seemed to do nothing)
    if (f.size && (f.x - fg->b.vx < 1 || f.x - fg->b.vx > 5 || f.y - fg->b.vy < 1 || f.y - fg->b.vy > 5)) {
        focus(fg->cur, 2);
        draw_fight(c);
    }
    char words[80];
    words[0] = 0;
    if (dk_menu()) {
        bool spells = false;
        for (int k = 0; k < magic::kListSize; ++k)
            if (f.rec[magic::kListAt + k] && !(f.rec[magic::kListAt + k] & 0x80)) spells = true;
        fight_menu_dk(words, sizeof words, f, spells);
        fg->st = FSt::Menu;
        text::build(menu, "", words);
        menu.selected = 0;
        show_menu_line(c);
        return;
    }
    if (f.moves > 1) strcat(words, fw(profile::kFMove));
    strcat(words, fw(profile::kFViewAim));
    if (f.n_items && f.can_use) strcat(words, fw(profile::kFUse));
    bool spells = false;
    for (int k = 0; k < magic::kListSize; ++k)
        if (f.rec[magic::kListAt + k] && !(f.rec[magic::kListAt + k] & 0x80)) spells = true;
    if (spells && f.can_cast && !no_magic_area()) strcat(words, fw(profile::kFCast));
    if (combat::can_turn(fg->b, fg->t, f)) strcat(words, fw(profile::kFTurn));
    // (Champions, GAME.OVR ovl 0x58 +0xA45: Yell for a kender while no one
    // taunts the enemy yet)
    if (kr_can_yell(f)) strcat(words, d->w_yell);
    strcat(words, fw(profile::kFQuickDone));
    fg->st = FSt::Menu;
    text::build(menu, "", words);
    menu.selected = 0;
    show_menu_line(c);
}

void move_prompt(pic::Canvas& c)
{
    char t[48];
    snprintf(t, sizeof t, "%s%d ", fw(profile::kMoveLeft), fg->b.f[fg->cur].moves / 2);
    if (hl()) {
        // (the Dark Queen: the line in its prompt box, then "Undo Done" -
        // DS 0x50F8, DOSBox)
        text::build(menu, t, d->w_undo_done);
        // (the original lights neither; the engine's rule: a menu on its
        // first word - CYD_MENU_HIGHLIGHT, navcheck N1 / N3)
        menu.selected = CYD_MENU_HIGHLIGHT ? 0 : -1;
        show_menu_line(c);
        return;
    }
    clear_menu_line(c);
    put(c, t, 0, text::kMenuRow, 15);
    menu_on = false;
}

void start_move(pic::Canvas& c)
{
    const combat::Fighter& f = fg->b.f[fg->cur];
    fg->move_x = f.x;
    fg->move_y = f.y;
    fg->move_left = f.moves;
    fg->st = FSt::Move;
    move_prompt(c);
}

// Leaving Move: what's left stays (under 2: none)
void end_move(pic::Canvas& c)
{
    combat::Fighter& f = fg->b.f[fg->cur];
    if (f.moves < 2) f.moves = 0;
    if (!f.up()) {
        end_turn(c);
        return;
    }
    fight_menu(c);
}

// A readied weapon that only shoots (a bow, a crossbow, a sling - not one
// that also strikes, like a dagger or a spear): no blows with it
bool pure_missile(const combat::Fighter& f)
{
    const int w = combat::weapon(f, *names);
    if (w < 0) return false;
    const items::TypeInfo& ti = names->type(f.items[w][0x2E]);
    return ti.range > 1 && !(ti.flags & 0x04);
}

bool enemy_next_to(int i)
{
    const combat::Battle& b = fg->b;
    for (int k = 0; k < b.n; ++k)
        if (k != i && b.f[k].up() && b.f[k].size && b.f[k].team() != b.f[i].team() && combat::adjacent(b, i, k))
            return true;
    return false;
}

// One step (or an attack that way) by the fighter whose turn it is
void try_step(pic::Canvas& c, int dir)
{
    combat::Battle& b = fg->b;
    const int i = fg->cur;
    combat::Fighter& f = b.f[i];
    bool edge;
    int blocker;
    const int cost = combat::step_cost(b, fg->t, i, dir, &edge, &blocker);
    if (blocker >= 0) {
        const combat::Fighter& o = b.f[blocker];
        if (f.attacks[0] <= 0 && f.attacks[1] <= 0) return;
        if (o.team() == f.team()) {
            fg->pending = blocker;
            fg->st = FSt::AllyAsk;
            text::build(menu, fw(profile::kAttackAlly), rw(Data::kYesNo));
            menu.selected = 1;
            show_menu_line(c);
            return;
        }
        if (pure_missile(f)) {          // (the original's "Not with that weapon")
            note(c, fw(profile::kNotWithWeapon));
            return;
        }
        f.target = blocker;
        melee(c, i, blocker);
        say(c, FSt::Turn);              // (after_attack)
        return;
    }
    if (edge) {
        fg->flee_dir = dir;
        fg->st = FSt::FleeAsk;
        text::build(menu, fw(profile::kFleeAsk), rw(Data::kYesNo));
        menu.selected = 1;
        show_menu_line(c);
        return;
    }
    if (cost == 0xFF || cost > f.moves) {
        note(c, fw(profile::kCantGo));
        return;
    }
    free_attacks(c, i, dir);
    if (!f.up()) {
        say(c, FSt::Turn);
        return;
    }
    // A free attack that left it held (paralysed, asleep) or stunned: the
    // original still takes the step (coab's move_step_away_attack, then the
    // step - only one out of the fight stops), AD&D's with the Game Fixes:
    // no step, the turn over (SPEC 12)
    if (rules::fixes() && (combat::helpless(b, f) || combat::stunned(b, f))) {
        draw_fight(c);
        say(c, FSt::Next);
        return;
    }
    // Held fast by a free attack (a hug, engulfed - its moves 0): no step
    // (the original steps only with moves left, and a step costing more
    // than is left leaves 0 - coab's move handler / sub_3E748)
    if (f.moves <= 0) {
        draw_fight(c);
        if (fg->pages) say(c, FSt::Move);
        else end_move(c);
        return;
    }
    combat::step(b, i, dir);
    sfx(sound::kStep);
    f.moves = static_cast<int16_t>(f.moves > cost ? f.moves - cost : 0);
    guard_attacks(c, i);
    cloud_step(i);
    if (fg->pages) {
        draw_fight(c);
        say(c, !f.up() ? FSt::Turn : (combat::helpless(b, f) || combat::stunned(b, f)) ? FSt::Next : FSt::Move);
        return;
    }
    if (!on_screen(i) || f.x - b.vx < 1 || f.x - b.vx > 5 || f.y - b.vy < 1 || f.y - b.vy > 5) focus(i, 2);
    draw_fight(c);
    if (f.moves < 2) {
        end_move(c);
        return;
    }
    move_prompt(c);
}

// ---- Aim: the candidates by distance
void aim_menu(pic::Canvas& c);
int spell_power(const combat::Fighter& f);

bool can_target(int t, int* dist)
{
    combat::Battle& b = fg->b;
    const int i = fg->cur;
    const combat::Fighter& f = b.f[i];
    if (fg->spell) {
        // A spell: anyone in its reach and in sight (themselves too)
        if (t < 0 || !b.f[t].size) return false;
        int sq = 0;
        const bool seen = t == i || combat::range(b, fg->t, i, t, false, &sq);
        if (dist) *dist = sq;
        const int pw = fg->use_item >= 0 ? combat::power_of(f, mrules->tables, fg->spell, true) : spell_power(f);
        return seen && sq <= spells::reach(mrules->tables, fg->spell, pw);
    }
    if (t == i || !b.f[t].up() || !b.f[t].size) return false;
    int sq;
    const bool seen = combat::range(b, fg->t, i, t, false, &sq);
    if (dist) *dist = sq;
    if (!seen) return false;
    int ammo, reach = combat::missile(b, f, &ammo);
    if (pure_missile(f)) {
        // Only with ammunition, and not with an enemy next to them (facts 2.4)
        if (reach <= 0 || enemy_next_to(i)) return false;
    } else if (combat::adjacent(b, i, t)) {
        reach = 1;
    }
    if (reach < 1) reach = 1;
    return sq <= reach && (f.attacks[0] > 0 || f.attacks[1] > 0);
}

void start_aim(pic::Canvas& c)
{
    combat::Battle& b = fg->b;
    const int i = fg->cur;
    fg->n_cand = 0;
    int dist[combat::kMaxFighters];
    for (int k = 0; k < b.n; ++k) {
        if (!b.f[k].up() || !b.f[k].size) continue;
        int sq = 0;
        combat::range(b, fg->t, i, k, true, &sq);
        // in order of distance
        int at = fg->n_cand++;
        while (at > 0 && dist[at - 1] > sq) {
            fg->cand[at] = fg->cand[at - 1];
            dist[at] = dist[at - 1];
            --at;
        }
        fg->cand[at] = k;
        dist[at] = sq;
    }
    fg->ci = fg->n_cand > 1 ? 1 : 0;        // the nearest other than themselves
    // (Darkness, DOSBox: its aim starts on the fighter itself, "range = 0")
    if (d->prof->fight.words[profile::kAimDk] && !fg->spell) fg->ci = 0;
    fg->manual = false;
    fg->st = FSt::Aim;
    aim_menu(c);
}

void aim_menu(pic::Canvas& c)
{
    fg->st = FSt::Aim;                  // (also back from "Abort Spell?" - No / Esc)
    combat::Battle& b = fg->b;
    const int t = fg->manual ? (b.who[fg->my][fg->mx] ? b.who[fg->my][fg->mx] - 1 : -1) : fg->cand[fg->ci];
    if (!fg->outline)
        fg->outline = fight_pic(d->prof->fight.marks.outline ? d->prof->fight.marks.outline : 0x19 + 0x80);  // (another try)
    if (t >= 0 || fg->manual) {
        // (the Manual square keeps the view with it - empty or not: Tom,
        // v0.104.2, the cursor went off the view)
        if (fg->manual) focus_at(fg->mx, fg->my, 2);
        else focus(t, 2);
        q_aim_unmarked = hl();
        draw_fight(c);
        q_aim_unmarked = false;
        if (!hl()) mark(c, fg->cur);
        if (t >= 0) draw_panel_for(c, t);
        // The box round what's aimed at
        const int ax = t >= 0 ? b.f[t].x : fg->mx, ay = t >= 0 ? b.f[t].y : fg->my;
        if (hl() && fg->cursor) {
            // (the Dark Queen: what's aimed at on the turn's grey square, as
            // the one whose turn it is - DOSBox)
            if (t >= 0 && b.f[t].size) {
                mark(c, t);
            } else {
                blit4(c, fg->cursor, kSq, kSq, kViewX + (ax - b.vx) * kSq, kViewY + (ay - b.vy) * kSq, false, true,
                      kViewX, kViewY, kViewX + 7 * kSq, kViewY + 7 * kSq);
            }
        } else if (fg->outline)
            blit4(c, fg->outline, kSq, kSq, kViewX + (ax - b.vx) * kSq, kViewY + (ay - b.vy) * kSq, false, true, kViewX,
                  kViewY, kViewX + 7 * kSq, kViewY + 7 * kSq);
    }
    int sq = 0;
    bool ok = t >= 0 && can_target(t, &sq);
    if (fg->spell && fg->manual && t < 0 && fg->sp_area >= 0) {
        // An area spell may go to an empty square in reach and sight
        int len;
        const bool seen = combat::path(b, fg->t, fg->cur, fg->mx, fg->my, false, &len);
        sq = len / 2;
        ok = seen && combat::tile(b, fg->t, fg->mx, fg->my)[0] != 0xFF &&
             sq <= spells::reach(mrules->tables, fg->spell, spell_power(b.f[fg->cur]));
    }
    char line[24];
    snprintf(line, sizeof line, "%s%d%s", fw(profile::kRangeIs), sq, fw(profile::kRangeEndDk));
    if (!hl()) status_line(c, t >= 0 ? line : "");
    char words[64];
    const char* prompt = fg->manual ? "" : fw(profile::kAimPrompt);
    if (d->prof->fight.words[profile::kAimDk]) {
        // Darkness (R15): "aim: " Next Prev Manual [Target] Center Exit; Manual:
        // "(use cursor keys) " [Target] Exit
        if (fg->manual) {
            const bool on[2] = {ok, true};
            dk_words(fw(profile::kAimManualDk), on, 2, words, sizeof words);
            prompt = fw(profile::kManualPromptDk);
        } else {
            const bool on[6] = {true, true, true, ok, true, true};
            dk_words(fw(profile::kAimDk), on, 6, words, sizeof words);
        }
    } else if (fg->manual) {
        snprintf(words, sizeof words, "%s%s", ok ? fw(profile::kFTarget) : "", fw(profile::kCenterExit));
    } else {
        snprintf(words, sizeof words, "%s%s%s", fw(profile::kNextPrevManual), ok ? fw(profile::kFTarget) : "",
                 fw(profile::kCenterExit));
    }
    if (hl()) {
        // (the Dark Queen: the prompt and the range in a line of their own
        // above the buttons - "AIM: RANGE = 5", DOSBox)
        char above[48];
        snprintf(above, sizeof above, "%s%s", prompt, t >= 0 ? line : "");
        text::build(menu, "", words);
        menu.selected = 0;
        show_menu_line(c);
        q_line_above(c, above);
        return;
    }
    text::build(menu, prompt, words);
    menu.selected = 0;
    show_menu_line(c);
}

void aim_target(pic::Canvas& c, int t)
{
    combat::Fighter& f = fg->b.f[fg->cur];
    status_line(c, "");
    if (fg->b.f[t].team() == f.team()) {
        fg->pending = t;
        fg->st = FSt::AllyAsk;
        text::build(menu, fw(profile::kAttackAlly), rw(Data::kYesNo));
        menu.selected = 1;
        show_menu_line(c);
        return;
    }
    f.target = t;
    strike(c, t);
    say(c, FSt::Turn);
}

// The blow or the shot at t: missiles - the readied ammunition (arrows /
// quarrels) or the thrown weapon - go when it isn't next to them
void strike(pic::Canvas& c, int t)
{
    combat::Fighter& f = fg->b.f[fg->cur];
    int ammo = -1;
    const bool shot = !combat::adjacent(fg->b, fg->cur, t) && combat::missile(fg->b, f, &ammo) > 0;
    if (shot) spend_ammo(fg->cur, ammo, do_attack(c, fg->cur, t, false, true, ammo));
    else melee(c, fg->cur, t);
}

void spell_target(pic::Canvas& c, int t);

void aim_tap(int x, int y, pic::Canvas& c)
{
    combat::Battle& b = fg->b;
    if (y < text::kMenuTapTop) {
        // A tap on the field: that square (Manual) / that fighter
        if (x < kViewX || x >= kViewX + 7 * kSq || y < kViewY || y >= kViewY + 7 * kSq) return;
        const int sx = b.vx + (x - kViewX) / kSq, sy = b.vy + (y - kViewY) / kSq;
        const int w = b.who[sy][sx];
        if (fg->manual) {
            fg->mx = sx;
            fg->my = sy;
        } else if (w) {
            for (int k = 0; k < fg->n_cand; ++k)
                if (fg->cand[k] == w - 1) fg->ci = k;
        }
        aim_menu(c);
        return;
    }
    const int t = fg->manual ? (b.who[fg->my][fg->mx] ? b.who[fg->my][fg->mx] - 1 : -1) : fg->cand[fg->ci];
    switch (text::key(menu, text::hit(menu, x / 8))) {
    case 'N': fg->ci = (fg->ci + 1) % (fg->n_cand ? fg->n_cand : 1); break;
    case 'P': fg->ci = (fg->ci + fg->n_cand - 1) % (fg->n_cand ? fg->n_cand : 1); break;
    case 'M':
        fg->manual = true;
        fg->mx = t >= 0 ? b.f[t].x : b.f[fg->cur].x;
        fg->my = t >= 0 ? b.f[t].y : b.f[fg->cur].y;
        break;
    case 'C':
        // Center: the aimed square in the middle of the view (a fighter or,
        // in Manual, any square - Tom, v0.104.2: it did nothing)
        if (fg->manual) focus_at(fg->mx, fg->my, 0);
        else if (t >= 0) focus(t, 0);
        break;
    case 'T':
        if (fg->spell) {
            spell_target(c, t);
            return;
        }
        if (t >= 0 && can_target(t, nullptr)) {
            aim_target(c, t);
            return;
        }
        break;
    case 'E': aim_exit(c); return;
    default: return;
    }
    aim_menu(c);
}

// Leaving the aim (Exit, Esc): a weapon's - back to the menu; a spell's
// (memorized or an item's) - "Abort Spell? Yes No" as the original (Yes:
// the spell or the use is gone, the turn over; No: aim again); picking one
// by one - cast with those picked (even none: the original)
void tr_jump(pic::Canvas& c);
void aim_exit(pic::Canvas& c)
{
    status_line(c, "");
    if (fg->jump) {
        // (Treasures' jump: the aim left - 0x2DBF3 makes the result 1, so the
        // charge goes and the turn is over, nothing asked or said)
        fg->jump = false;
        fg->spell = 0;
        spend_item(fg->cur, 0);
        combat::Fighter& f = fg->b.f[fg->cur];
        f.attacks[0] = f.attacks[1] = 0;
        f.moves = 0;
        draw_fight(c);
        end_turn(c);
        return;
    }
    if (fg->spell && (fg->sp_n > 0 || fg->pick_budget >= 0)) {
        cast_now(c);
        return;
    }
    if (fg->spell) {
        fg->st = FSt::AbortAsk;
        text::build(menu, fw(profile::kAbortSpell), rw(Data::kYesNo));
        menu.selected = 1;
        // (Pool's in yellow - ovr23 +0xEF8 passes 14)
        if (d->prof->game == games::Game::PoolOfRadiance) menu.prompt_fg = 14;
        show_menu_line(c);
        return;
    }
    fg->use_item = -1;
    draw_fight(c);
    fight_menu(c);
}

// ---- Done: "Guard Delay Quit Bandage Speed Exit"
void done_menu(pic::Canvas& c)
{
    const combat::Fighter& f = fg->b.f[fg->cur];
    char words[64];
    words[0] = 0;
    const int w = combat::weapon(f, *names);
    if (w < 0 || (names->type(f.items[w][0x2E]).flags & 0x04)) strcat(words, fw(profile::kFGuard));
    strcat(words, fw(profile::kDelayQuit));
    if (combat::anyone_dying(fg->b)) strcat(words, fw(profile::kFBandage));
    strcat(words, fw(profile::kSpeedExitW));
    fg->st = FSt::DoneMenu;
    text::build(menu, "", words);
    menu.selected = 0;
    show_menu_line(c);
}

void speed_menu(pic::Canvas& c)
{
    const int speed = vm->get(0x4BFC) & 0xFF;
    char prompt[24], words[32];
    snprintf(prompt, sizeof prompt, "%s%s%d%s", fw(profile::kGameSpeed), fw(profile::kSpeedOpenDk), speed,
             fw(profile::kSpeedClose));
    words[0] = 0;
    if (d->prof->fight.words[profile::kSpeedWordsDk]) {
        // Darkness (R15): "game speed: (n) " Faster Slower Exit
        const bool on[3] = {speed > 0, speed < 9, true};
        dk_words(fw(profile::kSpeedWordsDk), on, 3, words, sizeof words);
    } else {
        if (speed < 9) strcat(words, fw(profile::kFSlower));
        if (speed > 0) strcat(words, fw(profile::kFFaster));
        strcat(words, fw(profile::kFExit));
    }
    fg->st = FSt::Speed;
    text::build(menu, prompt, words);
    menu.selected = 0;
    hl_inherit();                       // (Darkness: GAME.OVR 0x983A - no place of its own)
    show_menu_line(c);
}

// ---- The computer's turn
// Everyone the computer plays - Quick members, NPCs and the monsters too
// (the original's computer turn readies weapons for all: coab's facts,
// AI_items_selection) - readies the weapon the games' rating picks; a
// two-handed one puts the shield away (a cursed shield: the weapon goes
// back), free hands take the best shield (its plus + 1). A monster's
// record is worked out again as the original's routine does (with its
// group's items). True: it changed.
bool ai_ready_weapon(int i)
{
    combat::Fighter& f = fg->b.f[i];
    const bool member = f.member >= 0 && f.member < pt->count;
    if (!member && (f.monster < 0 || !ml || !f.items)) return false;
    const int want = combat::choose_weapon(fg->b, i);
    if (want == combat::kKeep) return false;
    party::Character* tmp = nullptr;
    if (!member) {
        tmp = new (std::nothrow) party::Character;
        if (!tmp) return false;
        f.rec.read(tmp->rec);
        tmp->n_items = f.n_items < party::kMaxItems ? f.n_items : party::kMaxItems;
        memcpy(tmp->items, f.items, static_cast<size_t>(tmp->n_items) * items::kRecordSize);
        tmp->n_affects = f.n_aff ? (*f.n_aff < party::kMaxAffects ? *f.n_aff : party::kMaxAffects) : 0;
        if (tmp->n_affects) memcpy(tmp->affects, f.aff, static_cast<size_t>(tmp->n_affects) * party::kAffectSize);
    }
    party::Character& ch = member ? pt->m[f.member] : *tmp;
    auto recalc = [&]() {
        if (member) rules::recalc(ch, *names, d->facts);
        else rules::recalc_values(ch, *names, d->facts);
    };
    const int held = combat::weapon(f, *names);
    if (held >= 0) {
        ch.items[held][0x34] = 0;
        rules::worn(ch, held, false);
    }
    if (want >= 0) {
        ch.items[want][0x34] = 1;
        rules::worn(ch, want, true);
    }
    recalc();
    // The shield
    int shield = -1, best = -1, best_plus = 0;
    for (int k = 0; k < ch.n_items; ++k) {
        const items::TypeInfo& ti = names->type(ch.items[k][0x2E]);
        if (ti.slot != 1) continue;
        if (ch.items[k][0x34]) shield = k;
        const int plus = static_cast<int8_t>(ch.items[k][0x32]);
        if ((ch.rec[0x12B] & ti.classes) && plus + 1 > best_plus) {
            best = k;
            best_plus = plus + 1;
        }
    }
    if (ch.rec[0x185] > 2) {
        const int k = shield >= 0 && !ch.items[shield][0x36] ? shield : want;
        if (k >= 0) {
            ch.items[k][0x34] = 0;
            rules::worn(ch, k, false);
        }
    } else if (ch.rec[0x185] < 2 && best >= 0 && best != shield && !(shield >= 0 && ch.items[shield][0x36]) &&
               ch.rec[0x185] + names->type(ch.items[best][0x2E]).hands - (shield >= 0 ? names->type(ch.items[shield][0x2E]).hands : 0) <= 2) {
        if (shield >= 0) {
            ch.items[shield][0x34] = 0;
            rules::worn(ch, shield, false);
        }
        ch.items[best][0x34] = 1;
        rules::worn(ch, best, true);
    }
    recalc();
    if (member) {
        f.n_items = ch.n_items;
        f.items = ch.items;
    } else {
        // Back into the monster (its record, effects) and its items - its own
        // list once they differ from the group's (copy on write)
        f.rec.write(tmp->rec);
        const size_t ib = static_cast<size_t>(tmp->n_items) * items::kRecordSize;
        if (ib && memcmp(f.items, tmp->items, ib) != 0) {
            // (a copy of its group that readied the same already has the
            // list: read that one - copies changing one way share it)
            const combat::Monster& me = ml->m[f.monster];
            const bool mine = me.items && f.items == me.items;
            bool shared = false;
            for (int j = 0; !mine && j < fg->b.n && !shared; ++j) {
                const combat::Fighter& o = fg->b.f[j];
                if (j == i || o.monster < 0 || o.monster >= ml->n || ml->m[o.monster].group != me.group ||
                    o.items == ml->g[me.group]->item || o.n_items != tmp->n_items || memcmp(o.items, tmp->items, ib) != 0)
                    continue;
                f.items = o.items;
                shared = true;
            }
            if (!shared) {
                own_items(f);
                memcpy(f.items, tmp->items, ib);
            }
        }
        if (f.n_aff) {
            int n = tmp->n_affects < f.max_aff ? tmp->n_affects : f.max_aff;
            if (combat::Monster* m = f.rec.monster()) {
                if (!m->room_for_aff(n)) n = m->aff_cap;
                f.aff = m->aff;
            }
            *f.n_aff = n;
            if (n) memcpy(f.aff, tmp->affects, static_cast<size_t>(n) * party::kAffectSize);
        }
        delete tmp;
    }
    combat::recount_attacks(fg->b, i);      // (its attacks for the weapon now: coab reclac_attacks)
    return true;
}

void fight_turn_undead(pic::Canvas& c);
// Champions' computer turn's first dice (GAME.OVR 0xC5B0-0xC614): combat
// byte 0x15 at 1-4 stays unless a d4 comes up 1; otherwise (and then) a d8:
// 8 makes it a d2 + 4, else a d4. The last die's sides are kept - its Turn
// Undead's type limit (0xC772's [bp-6], the stack word that die's side
// count was pushed at)
void champ_ai_dice(combat::Fighter& f)
{
    const int v = f.ai_mood;
    if (v >= 1 && v <= 4 && rng.roll(4, 1) != 1) {
        f.ai_sides = 4;
        return;
    }
    if (rng.roll(8, 1) == 8) {
        f.ai_mood = static_cast<uint8_t>(rng.roll(2, 1) + 4);
        f.ai_sides = 2;
    } else {
        f.ai_mood = static_cast<uint8_t>(rng.roll(4, 1));
        f.ai_sides = 4;
    }
}

void ai_step(pic::Canvas& c)
{
    combat::Battle& b = fg->b;
    const int i = fg->cur;
    combat::Fighter& f = b.f[i];
    // (Champions: its dice first, once a turn - before the morale test)
    if (d->prof->game == games::Game::ChampionsOfKrynn && !fg->ai_checked && !fg->flee_told && !fg->ai_diced) {
        fg->ai_diced = true;
        champ_ai_dice(f);
    }
    // Quick party members bandage a dying friend first (at the turn's start)
    // (a spell begun earlier goes first, as it did at the turn's start).
    // Death Knights and the Dark Queen: later - in the move-and-attack step,
    // after morale, items, the pending spell, Turn, spells and the weapon
    // (below: GAME.OVR 0xAF23 in 0xAEE7; DQK.EXE 0x2E98B in 0x2E94D)
    auto bandage_now = [&]() {
        if (combat::anyone_dying(b)) CYD_TEST_ACT(b, i, Bandage, -1, 0);
        const int k = combat::bandage(b);
        if (k < 0) return false;
        add_page(k, fw(profile::kBandaged));
        f.delay = 0;
        say(c, FSt::Next);                // (the turn is over: not an attack)
        fg->cur = i;
        return true;
    };
    // (Champions too: its AI's move step 0xD270 bandages first - GAME.OVR
    // 0xD2A9, after the weapon at 0xC706; round 11)
    const bool late_bandage = dkk() || hl() || d->prof->game == games::Game::ChampionsOfKrynn;
    if (!late_bandage && !f.team() && !fg->ai_checked && !fg->flee_told && !f.spell && combat::anyone_dying(b) && bandage_now())
        return;
    // Morale (monsters, NPCs), once a turn (flee_told: done)
    if (!fg->ai_checked && !fg->flee_told) {
        // The holds it had (engulfing, hugging) let go, as the original does
        combat::release_holds(b, i);
        // Last turn's panic is over: the morale test is taken again (every
        // game's FleeCheck clears it first - coab's sub_3637F; Darkness ovr
        // 0xACBE, Secret 0xE240, Champions 0xD84F, Dark Queen 0x2EF38)
        if (f.panic) {
            f.panic = false;
            f.fleeing = false;
        }
        // One forced to flee - turned undead, fear - says so at each of its
        // turns, then runs: "is forced to flee" (every game's FleeCheck; Pool
        // ovr10 +0x10D7, DOSBox, Sokal Keep's undead). Its turn's checks go
        // on after the page: the items, a spell, Turn Undead - ovr10 +0xC8 on
        if (f.fleeing) {
            fg->flee_told = true;
            add_page(i, fw(profile::kForcedFlee));
            say(c, FSt::Ai);
            return;
        }
        fg->flee_told = true;
        // (one already fleeing takes no morale test: it neither panics again
        // nor surrenders - the original's FleeCheck, Pool ovr10 +0x10D7 /
        // coab's sub_3637F: the test is the other branch)
        b.morale_base = vm->get(0x7EC6) & 0xFF;
        const combat::Morale mo = f.fleeing ? combat::Morale::Fight : combat::morale(b, fg->t, i);
        if (mo == combat::Morale::Surrender) {
            // (the original's 0x644A7: "Surrenders" shown while it still
            // stands - its icon, its panel - then it's unconscious, HP 0, off
            // the field: `yielding`, at the pages' end. Tom's board: the
            // panel said unconscious beside "Surrenders")
            fg->yielding = static_cast<int8_t>(i);
            add_page(i, fw(profile::kSurrenders));
            // (Gateway's black dragon: its script counts it - area word 0x4D03)
            if (b.yielded && b.fx && b.fx->gw.surrender_word) {
                const uint16_t w = b.fx->gw.surrender_word;
                vm->set(w, static_cast<uint16_t>(vm->get(w) + 1));
            }
            b.yielded = false;
            focus(i, 3);
            draw_fight(c);
            draw_icon(c, i, false, true);
            say(c, FSt::Next);
            return;
        }
        if (mo == combat::Morale::Flee && !f.fleeing) {
            f.fleeing = true;
            f.panic = true;                 // (this turn only)
            fg->ai_checked = false;         // (its items and spells next - the original's order)
            fg->flee_told = true;
            add_page(i, fw(profile::kFleesPanic));
            say(c, FSt::Ai);
            return;
        }
    }
    if (!fg->ai_checked) {
        fg->ai_checked = true;
        // The computer's magic items (monsters, NPCs and Quick members), then
        // spells, then Turn Undead - one fleeing too: the original tries them
        // before it runs (Pool ovr10 +0x10F on, coab's PlayerQuickFight -
        // nothing there asks about the flight). Darkness's and Treasures'
        // computer never uses an item: its item routine, called before the
        // pending spell, Turn and spells (Darkness GAME.OVR 0x9D9A, Treasures
        // 0xABB4), is an empty body that answers no. Death Knights' (0xA4D3)
        // rolls its d7 rounds and walks the items but never keeps one (its
        // chosen-item pointer [bp-0x14] is only ever set to 0): no item
        // either; the Dark Queen's (DQK.EXE 0x2E105) an empty body too
        const bool ai_items = !darkness_files() && !hl();
        if (ai_items && f.n_items && f.can_use && combat::standing(b, f.team() ? 0 : 1) && load_magic()) {
            int tg[combat::kMaxFighters], nt = 0;
            const int k = combat::choose_item(b, fg->t, mrules->tables, i, *names, d->prof->fight.spells,
                                              d->prof->fight.n_spells, rng, tg, &nt);
            if (k >= 0) {
                fg->use_item = k;
                fg->use_scroll = false;
                fg->spell = magic::item_spell(f.items[k], pool_files());
                fg->sp_n = nt;
                for (int j = 0; j < nt; ++j) fg->sp_t[j] = tg[j];
                draw_fight(c);
                announce_item(c, i, k);
                cast_now(c);
                return;
            }
        }
        // Darkness / Treasures / Death Knights / the Dark Queen: the spell
        // begun earlier now - after the morale test and the items (start_turn
        // left it)
        if (f.spell && late_spell()) {
            fire_pending(c);
            return;
        }
        // Turn Undead when an enemy undead is in sight and the fighter may
        // turn (combat::can_turn) - Curse's before its spells (coab's facts
        // 5.1: a pending spell, Turn Undead, then a spell - the cleric or
        // the former one passed), Pool's after them (below)
        // Gateway's computer (GAME.OVR 0xD266, before its spells): not turned
        // yet and a cleric's skill level - no paladins (its menu's Turn has
        // them); Champions' (0xC772, before its spells): the menu's test
        // Champions' computer (0xC772): a cleric, or a former cleric's level
        // over record 0xD7 (the engine's 0xE6) - signed - and turns under the
        // undead loaded; its finder's type limit is a local never set there
        // ([bp-6]): it holds what the AI's last dice call left on the stack
        // at that place - the dice routine's pushed side count (the AI's
        // d4 / d8 / d2 at its start, GAME.OVR 0xC5C3-0xC601): 4, or 2 when its
        // d8 came up 8 (champ_ai_dice: f.ai_sides) - undead types up to it.
        // Darkness's (GAME.OVR 0x9D14) / Treasures' (0xAB2C): not fleeing,
        // the menu's test (skill levels, turns under the undead loaded);
        // Death Knights' (0xA46C) the menu's test; all three after a pending
        // spell, before their spells. Gateway's finder (GAME.OVR 0x17679):
        // undead types 1-12 only
        const bool dk_ai = fg->t.turn_cols == 11;      // (the Dark Queen's AI 0x2E078 too: not fleeing, the menu's test)
        auto ai_turns = [&]() {
            if (d->prof->game == games::Game::ChampionsOfKrynn) {
                if (!fg->t.turn_known || f.undead_turns >= b.undead_loaded ||
                    !(static_cast<int8_t>(f.rec[0x109]) > 0 || static_cast<int8_t>(f.rec[0x111]) > static_cast<int8_t>(f.rec[0xE6])))
                    return false;
            } else if (gateway() ? f.turned_undead || combat::skill_level(f.rec, 0) <= 0
                                 : !combat::can_turn(b, fg->t, f) || (dk_ai && fg->t.turn_rule != 3 && f.fleeing))
                return false;
            for (int e = 0; e < b.n; ++e) {
                const combat::Fighter& o = b.f[e];
                if (o.up() && o.size && !(o.fleeing && !o.panic) && (o.team() ? 1 : 0) != (f.team() ? 1 : 0) && o.rec[0xE9] &&
                    (!gateway() || o.rec[0xE9] < 13) &&
                    (d->prof->game != games::Game::ChampionsOfKrynn || static_cast<int8_t>(o.rec[0xE9]) <= f.ai_sides) &&
                    combat::range(b, fg->t, i, e, false, nullptr)) {
                    fg->cur = i;
                    fight_turn_undead(c);
                    return true;
                }
            }
            return false;
        };
        if ((d->prof->game == games::Game::CurseOfTheAzureBonds || gateway() ||
             d->prof->game == games::Game::ChampionsOfKrynn || dk_ai) && ai_turns())
            return;
        if ((f.team() || f.rec[0xF7] >= 0x80) && f.can_cast && !no_magic_area() && load_magic()) {
            int tg[combat::kMaxFighters], nt = 0;
            const int sp = combat::choose_spell(b, fg->t, mrules->tables, i, d->prof->fight.spells,
                                                d->prof->fight.n_spells, rng, tg, &nt);
            if (sp) {
                const int delay = spells::entry(mrules->tables, sp).delay / 3;
                if (delay == 0 && fight_miscast(c, sp)) return;
                forget_spell(f, sp);
                if (delay > 0) {
                    f.spell = sp;
                    f.spell_n = 0;
                    for (int k = 0; k < nt && k < 24; ++k) f.spell_t[f.spell_n++] = static_cast<uint8_t>(tg[k]);
                    f.delay = f.delay > delay ? delay : 1;
                    add_page(i, fw(profile::kBeginsCasting));
                    say(c, FSt::Begin);
                    return;
                }
                fg->spell = sp;
                fg->sp_n = nt;
                for (int k = 0; k < nt; ++k) fg->sp_t[k] = tg[k];
                cast_now(c, true);
                return;
            }
        }
        // (Pool: a cleric on Quick - or any computer fighter with cleric
        // levels - that hasn't turned yet turns undead when an enemy undead
        // is in sight, before fighting: ovr10 +0x203; DOSBox, Sokal Keep -
        // Brother Sean's first act)
        if (pool_files() && ai_turns()) return;
        // The best weapon for now (the panel shows it)
        if (ai_ready_weapon(i)) draw_panel_for(c, i);
        // Its special actions (breath, spit, gazes, rays, thrown lightning)
        if (!f.fleeing) {
            combat::Event ev[24];
            bool ends = false;
            rolls_clear();
            const int nev = combat::special(b, fg->t, i, rng, ev, 24, &ends);
            if (nev > 0) {
                if (!on_screen(i)) focus(i, 3);    // the one doing it in view
                draw_fight(c);
                draw_icon(c, i, true);
                for (int k = 0; k < nev; ++k) event_page(ev[k]);
                draw_fight(c);
                if (ends || !f.up()) f.attacks[0] = f.attacks[1] = 0;
                say(c, ends || !f.up() ? FSt::Next : FSt::Ai);
                return;
            }
        }
    } else if (combat::missile_in_melee(b, i)) {
        // A bow with an enemy next to them: another weapon, and the turn is over
        if (ai_ready_weapon(i)) draw_panel_for(c, i);
        end_turn(c);
        return;
    }
    // (Death Knights / the Dark Queen: a party member with moves left and a
    // foe to go for bandages a dying friend - its moves then 0, the turn over;
    // GAME.OVR 0xAEE7 +0x3C, DQK.EXE 0x2E94D +0x3B - each step it takes)
    if (late_bandage && !f.team() && f.moves > 0 && combat::anyone_dying(b) && combat::standing(b, 1) && bandage_now()) {
        f.moves = 0;
        return;
    }
    const combat::Plan p = combat::think(b, fg->t, i, rng);
    switch (p.act) {
    case combat::Act::Attack:
        if (p.missile) spend_ammo(i, p.ammo, do_attack(c, i, p.target, false, true, p.ammo));
        else melee(c, i, p.target);
        say(c, FSt::Turn);
        return;
    case combat::Act::Step: {
        const int cost = combat::step_cost(b, fg->t, i, p.dir, nullptr, nullptr);
        free_attacks(c, i, p.dir);
        // (held / stunned by a free attack: no step with the Game Fixes; held
        // fast - moves 0 - none at all: see try_step)
        if (f.up() && f.moves > 0 && !(rules::fixes() && (combat::helpless(b, f) || combat::stunned(b, f)))) {
            combat::step(b, i, p.dir);
            sfx(sound::kStep);
            f.moves = static_cast<int16_t>(f.moves > cost ? f.moves - cost : 0);
            guard_attacks(c, i);
            cloud_step(i);
        }
        if (on_screen(i) && !f.team()) focus(i, 2);
        draw_fight(c);
        if (fg->pages) {
            say(c, !f.up() ? FSt::Turn : (combat::helpless(b, f) || combat::stunned(b, f)) ? FSt::Next : FSt::Ai);
            return;
        }
        if (!f.up()) {
            end_turn(c);
            return;
        }
        // (the next step at the next tick: the originals' steps wait nothing -
        // their only fixed waits in a fight are Quick's 200 ms, an attack's
        // 100, the burst's 70 a frame, the skull's 10 and a flight's own;
        // v0.32.0 to v0.170.1 waited 150 ms a step on screen)
        fg->st = FSt::Ai;
        fg->until = 1;
        return;
    }
    case combat::Act::Guard:
        CYD_TEST_ACT(b, i, Guard, -1, 0);
        f.guarding = true;
        end_turn(c);
        return;
    case combat::Act::Flee: {
        const bool away = combat::flee(b, fg->t, i, rng);
        add_page(i, fw(away ? profile::kGotAway : profile::kEscapeBlocked));
        draw_fight(c);
        say(c, FSt::Next);
        return;
    }
    default:
        end_turn(c);
        return;
    }
}

// ---- Start and end
void fight_party_icons()
{
    for (int i = 0; i < pt->count; ++i) compose_icon(i, pt->m[i]);
}

bool solid_side(void*, int dir)
{
    const int x = d->gs.x, y = d->gs.y;
    return geo::wall(d->map, x, y, dir) != 0 && geo::door(d->map, x, y, dir) == 0;
}

// Pool's outdoor field: the place on its overland map under the party
// (area words 0x4BC3 / 0x4BC4 = x / y - the program's 0x49C3 / 0x49C4; the
// three wildernesses' maps side by side, 13 places each) - its flags
// (poolrad_combat_facts.md 1.2)
int pool_place_flags()
{
    const int y = vm->get(0x4BC4) & 0xFF;
    const int x = (vm->get(0x4BC3) & 0xFF) + 13 * (d->gs.script - d->prof->variant.overland_area);
    if (x < 0 || x >= 44 || y < 0) return 0;
    uint8_t place = 0;
    plat::File f;
    if (open_file(d->prof->program, f)) {
        plat::FileSource src(f);
        exepack::Info info;
        if (exepack::parse(src, info) != exepack::Status::Ok ||
            exepack::read(src, info, d->prof->create.ds_image + d->prof->variant.overland_map + y * 44 + x, &place, 1) !=
                exepack::Status::Ok)
            place = 0;
        f.close();
    }
    return combat::pool_terrain(place, (vm->get(0x4CB3) & 0xFF) == 0xFF);
}

// The arena's fight over (or never begun): those out of it back in -
// Pool's end of an arena fight (START.EXE ovr6 +0x7DB): every member okay
// or animated in the fight again, a dying one unconscious
void arena_back()
{
    if (!vm->arena()) return;
    for (int i = 0; i < pt->count; ++i) {
        uint8_t* r = pt->m[i].rec;
        if (r[0x195] == party::Okay || r[0x195] == party::Animated) r[0x196] = 1;
        if (r[0x195] == party::Dying) r[0x195] = party::Unconscious;
    }
}

void fight_start(pic::Canvas& c)
{
    // What only other screens use goes first (they load again when needed):
    // the most room for the fight's blocks
    end_magic();
    delete hw;
    hw = nullptr;
    if (!d->prof->fight.tables.ground) {
        // A game whose fights aren't in the engine yet (Pool while it's built)
        error(c, "Fights aren't in the engine for this game yet.");
        fight_clear_monsters();
        arena_back();
        vm->set(0x7EC7, 0x81);          // (no fight: as if the party fled - never a win)
        resume_after_note = true;
        return;
    }
    make_room(16, "the fight");
    if (!fg) fg = make_fight();
    if (!fg || !ml || !pt->count) {
        error(c, ml && pt->count ? no_memory("the fight") : "Not enough memory.");
        delete fg;
        fg = nullptr;
        fight_clear_monsters();
        arena_back();
        vm->set(0x7EC7, 0x81);          // (no fight: as if the party fled - never a win)
        resume_after_note = true;       // the script goes on once the error is tapped
        return;
    }
    anim_stop();
    // A level can be drained (Pool's undead): the experience each level
    // needs, from the rule tables (loaded for this, before the field's blocks)
    int32_t level_exp[8][13] = {};
    const combat::MonFx& mf = d->prof->fight.facts.mon;
    if ((mf.drain1 || mf.drain2) && !darkness_files() && load_rules(mrules)) {
        for (int k = 0; k < 8; ++k)
            for (int l = 2; l < 13; ++l) level_exp[k][l] = mrules->tables.exp_needed(k, l - 1);
        end_magic();
    }
    load_fight_words();
    // The fighters' blocks: as many as this fight has, and room for some to
    // join (combat::kSpareFighters)
    release_fighters(fg->fighters);
    fg->fighters = nullptr;
    // (Darkness: its summoning reserve's 10 too, loaded with the tables below)
    int n_fighters = pt->count + ml->n + combat::kSpareFighters + (d->prof->fight.tables.reserve ? 10 : 0);
    if (n_fighters > combat::kMaxFighters) n_fighters = combat::kMaxFighters;
    // (the field's tiles first: the 3D view's memory, then the script's
    // code, parked - the arena the fighters and pictures go in)
    const bool tiles = alloc_tiles();
    if (tiles && park_code()) arena.pictures = true;
    fg->fighters = make_fighters(n_fighters);
    const bool room = fg->fighters && tiles;
    const bool tables = room && load_fight_tables();
    // Pool's two names gnomes dodge (its GAME.OVR words), as Pascal names
    for (int k = 0; tables && k < 2; ++k) {
        const char* w = fw(profile::kDodge1 + k);
        size_t n = strlen(w);
        if (n > 15) n = 15;
        fg->t.dodge_names[k][0] = static_cast<uint8_t>(n);
        memcpy(&fg->t.dodge_names[k][1], w, n);
    }
    if (!tables) {
        error(c, room ? "The combat tables can't be read." : no_memory("the fight's field"));
        end_fight_art();
        delete fg;
        fg = nullptr;
        fight_clear_monsters();
        arena_back();
        vm->set(0x7EC7, 0x81);          // (no fight: as if the party fled - never a win)
        resume_after_note = true;       // the script goes on once the error is tapped
        return;
    }
    load_reserve(fg->t);
    combat::Battle& b = fg->b;
    b.~Battle();
    new (&b) combat::Battle();         // (in place: a temporary would take a lot of stack)
    b.f = fg->fighters;
    b.cap = n_fighters;
    fg->arena = b.duel = vm->arena();
    b.tables = &fg->t;
    memcpy(b.level_exp, level_exp, sizeof level_exp);
    combat::g_dk_drain = darkness_files() ? dk_drain_hook : nullptr;     // (its own drain: facts L1)
    b.indoors = vm->get(0x4BE6) != 0;
    // (Pool: the picture before the fight isn't come back to - its treasure's
    // Exit shows the 3D view, DOSBox)
    if (pool_files()) last_pic_id = -1;
    // (Dark Queen: indoors only under a roof - its forests are 3D maps
    // fought on WILDCOM's ground: q_load_tiles)
    if (hl() && geo::flags(d->map, d->gs.x, d->gs.y) < 0x80) b.indoors = false;
    if (d->prof->variant.tr) {
        b.weather = d->gs.area1[0x52];
        b.weather_free = 0x9E;
    }
    // Pool: only its wilderness areas are fought outdoors (the rest from the 3D map)
    const int over = d->prof->variant.overland_area;
    if (d->prof->variant.overland_map && (d->gs.script < over || d->gs.script > over + 2)) b.indoors = true;
    // (tables and chairs in rooms - a d10 a square of a room: Curse's builder
    // (coab sub_370D3), Secret's GAME.OVR 0xEC85, Gateway's, Champions',
    // Death Knights'; not Pool's ovr11, nor Darkness's 0xBD45 / Treasures'
    // 0xCC56 indoor builders - no dice at all)
    if (b.indoors) combat::build_indoors(b, fg->t, d->map, d->gs.x, d->gs.y, rng, !pool_files() && !d->prof->variant.vga);
    else if (d->prof->variant.overland_map) combat::build_outdoors_pool(b, pool_place_flags(), rng);
    else if (darkness_files() || hl()) {
        combat::build_outdoors_dk(b, fg->t, vm->get(0x33) != 0, rng);   // (its own: facts L9)
        plat::log("[fight] outdoors: river %d, region %d\n", vm->get(0x33), vm->get(0x3A));
    }
    else combat::build_outdoors(b, fg->t, vm->get(0x4CA1) & 0xFF, rng);
    // (Pool: the field's scattered pieces - ovr11 +0x0 at both builders'
    // ends; DOSBox: Valjevo's fights)
    if (pool_files()) combat::pool_debris(b, fg->t, d->gs.script, rng);
    load_tiles(b.indoors);
    // The fighters: the party, then the monsters
    for (int i = 0; i < pt->count; ++i) {
        party::Character& ch = pt->m[i];
        combat::Fighter& f = b.f[b.n++];
        f.rec = ch.rec;
        f.aff = ch.affects;
        f.n_aff = &ch.n_affects;
        f.items = ch.items;
        f.n_items = ch.n_items;
        f.member = i;
        f.icon = i;
        // (every fight starts without Quick - Tom, 2026-10-31: Pool's own
        // keeps it in record byte 0x198 for the next fight; not followed,
        // SPEC 12)
        f.quick = false;
        f.max_aff = party::kMaxAffects;
        f.flags2 = ch.ext[party::kExtFlags2];
        f.magic_res = ch.ext[party::kExtMagicRes];
        // (Death Knights' saves and caster levels too - GAME.OVR 0x30E0B /
        // 0x34E8C; a Champions cleric's god for Turn Undead)
        // (the Dark Queen's too: its saves DQK.EXE 0x85CC, caster levels 0xB616 -
        // a mage, rec 0xAA, with a robe 0x19D: game block 0x1F9 + robe)
        f.robe = (krynn_rules() || hl()) && ch.level(classes::MagicUser) > 0 ? ch.ext[party::kExtRobe] : 0;
        f.god = krynn_rules() || hl() ? ch.ext[party::kExtGod] : 0;
    }
    b.party_size = pt->count;
    // The original recalculates every combatant's values at the battle's
    // setup (coab's facts: SetupCombatActions; the listing's sub_380E0 calls
    // sub_66C20 for each): a monster's to-hit, AC, damage dice and movement
    // come from its record's base values and its readied items - the
    // record's own stored ones are not used
    // (the copies of a group share its record: the group's is worked out
    // once, then each copy with changes of its own from its own bytes - its
    // changes kept over the group's new record)
    party::Character* tmp = new (std::nothrow) party::Character;
    uint8_t* fresh = new (std::nothrow) uint8_t[party::kRecordSize];
    if (tmp && fresh) {
        auto recalc_with = [&](const uint8_t (*list)[items::kRecordSize], int n) {
            tmp->n_items = n < party::kMaxItems ? n : party::kMaxItems;
            if (tmp->n_items) memcpy(tmp->items, list, static_cast<size_t>(tmp->n_items) * items::kRecordSize);
            tmp->n_affects = 0;
            rules::recalc_values(*tmp, *names, d->facts);
        };
        auto recalc = [&](const combat::Group& g) { recalc_with(g.item, g.n_items); };
        for (int gi = 0; gi < ml->groups; ++gi) {
            combat::Group& g = *ml->g[gi];
            memcpy(tmp->rec, g.rec, sizeof tmp->rec);
            recalc(g);
            memcpy(fresh, tmp->rec, party::kRecordSize);
            for (int k = 0; k < ml->n; ++k) {
                combat::Monster& m = ml->m[k];
                if (m.group != gi || (!m.own && !m.n_ch)) continue;
                combat::Rec(&m).read(tmp->rec);
                if (m.items) recalc_with(m.items, m.n_items);      // (its own items, from a fight before)
                else recalc(g);
                m.n_ch = 0;
                m.base = fresh;
                combat::Rec(&m).write(tmp->rec);
                m.base = g.rec;
            }
            memcpy(g.rec, fresh, party::kRecordSize);
        }
    }
    delete tmp;
    delete[] fresh;
    for (int k = 0; k < ml->n && b.n < b.cap; ++k) {
        if (ml->reserve_from >= 0 && k >= ml->reserve_from) break;     // (the reserve waits: summon())
        combat::Monster& m = ml->m[k];
        combat::Fighter& f = b.f[b.n++];
        f.rec = combat::Rec(&m);
        f.aff = m.aff;
        f.n_aff = &m.n_aff;
        monster_items(f, m);
        f.monster = k;
        f.icon = 8 + m.group;
        f.quick = true;
        f.max_aff = combat::kMonsterAffects;
        f.flags2 = m.flags2;
        f.magic_res = m.magic_res;
    }
    b.fx = &d->prof->fight.facts;
    // Darkness's Turn Undead (R13): the undead its LOAD MONSTERs loaded (each
    // cleric turns as many times), the script's penalty (block +0x0E)
    if (fg->t.turn_cols == 11) {
        for (int k = 0; k < ml->n; ++k) {
            const combat::Rec r(&ml->m[k]);
            if (r[0xE9] && r[0x195] != 9) ++b.undead_loaded;
        }
        b.turn_minus = vm->get(0x0F) & 0xFF;
    }
    // (Champions, GAME.OVR 0xEC2: each undead its LOAD MONSTERs loaded)
    if (fg->t.turn_rule == 2)
        for (int k = 0; k < ml->n; ++k)
            if (combat::Rec(&ml->m[k])[0xE9]) ++b.undead_loaded;
    b.names = names;
    b.arrow = d->facts.arrow;
    b.quarrel = d->facts.quarrel;
    // The scripts' surprise word (coab's facts: area2 0x596): (team + 1) &
    // it - 1 the party, 2 the monsters; the original clears it with the
    // first round's initiative, so it counts for this fight only
    b.surprise = vm->get(0x7ECB) & 3;
    vm->set(0x7ECB, 0);
    side_bonuses(b);
    if (pool_files()) {
        combat::place_pool(b, fg->t, d->gs.dir & 6, vm->fight_distance(), b.indoors ? &d->map : nullptr, d->gs.x, d->gs.y);
        const int fd = d->gs.dir & 6;
        b.flee_dir = (fd - ((fd + 2) % 4) / 2 + 8) & 7;
    } else {
        combat::place(b, fg->t, d->gs.dir & 6, vm->fight_distance(), b.indoors ? solid_side : nullptr, nullptr,
                      darkness_files() ? vm->fight_sides() : 0);
        // (Champions, GAME.OVR 0xCFC4: a run's way from the party's facing
        // [0x6D87] as Pool's - round 11)
        if (fg->t.mood_known) {
            const int fd = d->gs.dir & 6;
            b.flee_dir = (fd - ((fd + 2) % 4) / 2 + 8) & 7;
        }
    }
    combat::battle_start(b, &rng);           // those invisible from the start
    // Their pictures
    fight_party_icons();
    for (int g = 0; g < ml->groups; ++g)
        if (g != ml->reserve_group) load_monster_icon(8 + g, ml->g[g]->icon);
    arena_free(fg->cursor);                  // (a fight started over another: nothing kept twice)
    arena_free(fg->skull);
    arena_free(fg->outline);
    const combat::Comspr& cs = d->prof->fight.marks;
    fg->cursor = fight_pic(cs.cursor ? cs.cursor : 0x19);
    fg->skull = fight_pic(cs.skull ? cs.skull : 11 + 0x80);
    fg->outline = fight_pic(cs.outline ? cs.outline : 0x19 + 0x80);
    // (Tom, 3.2", v0.85.5: Aim's box once didn't show - a picture that
    // couldn't be loaded goes in the Errors log)
    if (!fg->cursor || !fg->outline || !fg->skull) log_error(no_memory("the fight's cursor / target box"));
    focus(0, 0xFF);
    screen = Screen::Fight;
    palette_fight(true);
    fg->cur = -1;
    draw_fight(c);
    clear_menu_line(c);
    if (b.weather < 2) {
        // Treasures: "Battle in snow! / rain! Movement restricted." (ovl 0x68 +0x1E4C)
        char t[40];
        ow(d->prof->variant.tr_weather[b.weather], t, sizeof t);
        put(c, t, 0, text::kMenuRow, 10);
    } else put(c, fw(profile::kBattleBegins), 0, text::kMenuRow, 10);
    fg->st = FSt::Begin;
    fg->until = plat::now_ms() + game_delay_ms() * 3;
    if (!fg->until) fg->until = 1;
    plat::log("[play] a fight: %d against %d (%s%s)\n", combat::standing(b, 0), combat::standing(b, 1),
                  b.indoors ? "indoors" : "outdoors", b.weather == 0 ? ", snow" : b.weather == 1 ? ", rain" : "");
    sound_hold(true);                        // (Hit / Miss: the output ready all fight)
    char t[48];
    snprintf(t, sizeof t, "fight start, %d against %d%s", combat::standing(b, 0), combat::standing(b, 1),
             fg->tiles_in_world ? "" : " (walls not parked)");
    memlog::note(t);
}

// Back to exploring after the fight (and its treasure)
void fight_back_to_game(pic::Canvas& c)
{
    screen = Screen::Game;
    anim_stop();
    bigpic = -1;
    pic_shown = false;
    head_shown = body_shown = -1;
    // (Curse's wilderness map; Darkness's overland draws its own map - draw_view)
    if (!vm->get(0x4BE6) && !pool_overland() && !darkness_files() && !gw_wild() && !hl()) host->picture(0x79, 0xFF);
    draw_frame(c);
    draw_view(c);
    draw_panel(c);
    text::clear(c, text::kTextArea);
    dirty(0, pic::kScreenH);
}

// The effects that end with the fight, the party put back on its feet
void fight_cleanup_party(combat::Result r)
{
    for (int i = 0; i < pt->count; ++i) {
        party::Character& ch = pt->m[i];
        const uint8_t charm = d->prof->fight.facts.charm;
        for (int k = 0; charm && k < ch.n_affects; ++k)
            if (ch.affects[k][0] == charm) combat::uncharm(ch.rec, ch.affects[k]);     // back on the party's side
        ch.rec[0x197] = 0;                              // (whatever happened: the party's side after a fight)
        for (uint8_t t : d->prof->fight.ends)
            if (t) rules::remove_affects(ch, t);
        // Darkness's dragon fear (0x78, R17): the original keeps its -2 / -2
        // after the fight (no time, not in the list); with the fixes it ends
        // with the fight (Tom, 2026-10-23; SPEC 12)
        if (rules::fixes() && d->prof->fight.facts.dk.feared) rules::remove_affects(ch, d->prof->fight.facts.dk.feared);
        rules::keep_hammer(ch, *names, d->facts);       // (its effect ended: the hammer goes)
        // Animated dead: they collapse - dead, on their own side again
        const uint8_t anim = d->prof->fight.facts.sp.animated;
        for (int k = 0; anim && k < ch.n_affects; ++k)
            if (ch.affects[k][0] == anim) {
                ch.rec[0x197] = static_cast<uint8_t>(ch.affects[k][3] >> 4);
                rules::remove_affects(ch, anim);
                ch.rec[0x195] = party::Dead;
                ch.rec[0x196] = 0;
                ch.rec[0x1A4] = 0;
                ch.rec[0xE9] = 0;
                ch.rec[0x11A] = 0;
                ch.rec[0x1A5] = 12;
                if (fg && i < fg->b.n) ch.rec[0xF7] = fg->b.f[i].was_control;
                break;
            }
        uint8_t* rec = ch.rec;
        if (fg && fg->arena && pool_files()) {
            // Pool's arena (START.EXE ovr6 +0x7DB): every member okay or
            // animated in the fight again, a dying one unconscious - nothing
            // more (one unconscious stays so; a runner stays running)
            if (rec[0x195] == party::Okay || rec[0x195] == party::Animated) rec[0x196] = 1;
            if (rec[0x195] == party::Dying) rec[0x195] = party::Unconscious;
            continue;
        }
        if (fg && fg->arena && r != combat::Fled && vm->get(0x7EE6)) {
            // Curse's duel (ovr6 +0x7A9 / +0x804): a dying duelist, or one
            // unconscious with no hit points, up again with 1
            if (rec[0x195] == party::Dying || (rec[0x195] == party::Unconscious && rec[0x1A4] == 0)) {
                rec[0x195] = party::Okay;
                rec[0x1A4] = 1;
            }
        }
        if (r == combat::Lost && !(fg && fg->arena)) continue;
        if (rec[0x195] == party::Running) rec[0x195] = party::Okay;
        if (r == combat::Fled) continue;
        if (rec[0x195] == party::Dying) rec[0x195] = party::Unconscious;
        if (rec[0x195] == party::Unconscious && rec[0x1A4] > 0) rec[0x195] = party::Okay;
        rec[0x196] = rec[0x195] == party::Okay || rec[0x195] == party::Animated;
    }
}

void open_treasure(pic::Canvas& c);
void results_page(pic::Canvas& c);
void treasure_page(pic::Canvas& c);
bool npc_share_page(pic::Canvas& c);

// The experience a platinum piece is worth
int fight_plat_xp()
{
    return d->prof->fight.facts.plat_xp ? d->prof->fight.facts.plat_xp : 5;
}

// Darkness's script place 0x1F: no experience given and none shown
// (GAME.OVR 0x4A2C, 0x51DF)
bool no_exp_lines()
{
    return darkness_files() && vm->get(0x1F) != 0;
}

// Treasures' and Darkness's old age (Treasures' ovl 0x62 +0x3F0, Darkness's
// ovr 0x864B), when someone aged since the last look (haste, the clock):
// each member not dead past the race's greatest age dies of it - "The
// strain of this last battle has worn heavily on NAME.", "He / She breathes
// deeply one last time and crumples to the ground." (white, the panel's
// rows 14-21, a key each), then what the others do (yellow); within ten
// years of it: "NAME is looking very old". The steps queue in old[]: kind,
// member, the one who weeps.
void end_fight(pic::Canvas& c);
void old_age_next(pic::Canvas& c);
bool old_age_begin(pic::Canvas& c)
{
    combat::Battle& b = fg->b;
    fg->old_n = fg->old_i = 0;
    auto add = [&](int kind, int who, int other) {
        if (fg->old_n >= 12) return;
        fg->old[fg->old_n][0] = static_cast<int8_t>(kind);
        fg->old[fg->old_n][1] = static_cast<int8_t>(who);
        fg->old[fg->old_n++][2] = static_cast<int8_t>(other);
    };
    for (int i = 0; i < b.party_size && i < pt->count; ++i) {
        party::Character& ch = pt->m[i];
        if (ch.health() >= party::Dead) continue;
        const int race = party::darkness_byte(ch.rec, ch.ext, ch.n_items, 0xAD);
        const int most = race >= 0 && race < 8 ? d->tr_max_age[race] : 0;
        const int age = ch.rec[0x76] | ch.rec[0x77] << 8;
        if (most && age > most) {
            add(1, i, -1);
            add(2, i, -1);
            int seen = 0, who = -1;         // the witnesses: up and okay
            for (int k = 0; k < pt->count; ++k)
                if (k != i && pt->m[k].health() == party::Okay && pt->m[k].hp() > 0) {
                    ++seen;
                    who = k;
                }
            add(seen == 0 ? 3 : seen == 1 ? 4 : 5, i, who);
        } else if (most && age > most - 10) {
            add(0, i, -1);
        }
    }
    if (!fg->old_n) return false;
    fg->st = FSt::OldAge;
    old_age_next(c);
    return true;
}
void old_age_next(pic::Canvas& c)
{
    if (fg->old_i >= fg->old_n) {
        fg->old_n = 0;
        clear_panel(c, 14, 21);
        end_fight(c);
        return;
    }
    const int8_t* o = fg->old[fg->old_i++];
    const auto& v = d->prof->variant;
    char nm[20] = {}, wn[20] = {}, a[96], b2[48], t[160];
    pt->m[o[1]].name(nm, sizeof nm);
    if (o[2] >= 0) pt->m[o[2]].name(wn, sizeof wn);
    auto word = [&](int k, char* out, size_t cap) { ow(v.tr_age_words[k], out, cap); };
    uint8_t colour = 15;
    switch (o[0]) {
    case 0:
        word(0, a, sizeof a);
        snprintf(t, sizeof t, "%s %s", nm, a);
        colour = 10;
        break;
    case 1:
        word(1, a, sizeof a);
        word(2, b2, sizeof b2);
        snprintf(t, sizeof t, "%s%s%s", a, nm, b2);
        break;
    case 2: {
        const party::Character& ch = pt->m[o[1]];
        word(party::darkness_byte(ch.rec, ch.ext, ch.n_items, 0x166) ? 4 : 3, b2, sizeof b2);
        word(5, a, sizeof a);
        snprintf(t, sizeof t, "%s%s", b2, a);
        // status 8, "dies from age" (ovl 0xFB +0x25)
        pt->m[o[1]].rec[0x195] = party::Gone;
        plat::log("[fight] old age: member %d dies\n", o[1]);
        break;
    }
    case 3:
        word(6, t, sizeof t);
        colour = 14;
        break;
    case 4:
        word(7, a, sizeof a);
        snprintf(t, sizeof t, "%s%s", wn, a);
        colour = 14;
        break;
    default:
        word(8, a, sizeof a);
        word(9, b2, sizeof b2);
        snprintf(t, sizeof t, "%s%s%s", a, nm, b2);
        colour = 14;
        break;
    }
    clear_panel(c, 14, 21);
    panel_text(c, t, 14, colour, 21);
    clear_menu_line(c);
    put(c, d->press_key, 0, text::kMenuRow, 13);
}

// The fight is over: the outcome, experience, treasure
void end_fight(pic::Canvas& c)
{
    combat::Battle& b = fg->b;
    // (Champions: the weapons left in stone bodies back to their owners)
    for (int k = 0; k < fg->n_lost; ++k)
        if (fg->lost_who[k] < pt->count && rules::add_item(pt->m[fg->lost_who[k]], fg->lost[k])) {
            rules::recalc(pt->m[fg->lost_who[k]], *names, d->facts);
            plat::log("[play] member %d's weapon back from the stone\n", fg->lost_who[k]);
        }
    fg->n_lost = 0;
    if (d->prof->variant.tr_max_age && age_check && !in_demo) {
        age_check = false;
        if (old_age_begin(c)) return;
    }
    fg->out = combat::finish(b, ml ? ml->m : nullptr);
    combat::Result r = fg->out.result;
    // (Pool: a member's attacks left this round live in its record, 0x113 /
    // 0x114 - set at each round's start (ovr14 +0xD23), used up attack by
    // attack (+0x16A4), cleared when the turn ends early (+0x14E4) - and stay
    // there after the fight, in the saved game: the fighter's own count. The
    // round's start sets them for every fighter, down or not (ovr14 +0x0 ->
    // +0xD23, +0xE52), so a member down keeps the round's count unused -
    // dosbox-debug after Kuto's Well's lizardmen: the fallen 1, the standing
    // who had attacked 0)
    if (pool_files())
        for (int i = 0; i < b.party_size; ++i)
            if (uint8_t* rec = b.f[i].rec.whole()) {
                const combat::Fighter& f = b.f[i];
                int a1 = f.attacks[0], a2 = f.attacks[1];
                if (!f.up()) {
                    bool ranged = false;
                    a1 = combat::slot1_attacks(b, f, &ranged);
                    a2 = combat::attacks_this_round(rec[0x11D], b.round);
                }
                rec[0x19C] = static_cast<uint8_t>(a1);
                rec[0x19D] = static_cast<uint8_t>(a2);
            }
    // (Pool: a fight none of the enemies stayed in - every one ran off -
    // ends "The party has found Treasure!": its won flag DS 0x443A is set
    // only by a monster not running away - ovr6 +0x88 / +0x9D7; DOSBox,
    // Sokal Keep's undead all turned)
    bool none_beaten = false;
    if (pool_files()) {
        none_beaten = true;
        for (int i = b.party_size; i < b.n; ++i)
            if (b.f[i].team() && b.f[i].status() != party::Running) none_beaten = false;
    }
    if (in_demo && r == combat::Lost)
        // The demo's party is all NPCs (who don't count as the party's
        // survivors in a real game): it wins while one of them stands
        for (int i = 0; i < b.party_size; ++i)
            if (b.f[i].status() == party::Okay || b.f[i].status() == party::Animated) r = combat::Won;
    // The arena (CALL 0x8000 / 0x8001): Pool (START.EXE ovr6 +0x776) has
    // won when a fighter of the party's side is still in the fight and okay
    // - the duelist; Curse (ovr6 +0x618, its demo aside) when any member is
    // okay or animated (those sat out count: "You have won the duel." though
    // the duelist fell), fled when one runs and none is up. Neither ends the
    // game: a lost duel is no party destroyed
    const bool arena = fg->arena && !in_demo;
    const bool rolf = arena && vm->get(0x7EE6) != 0;
    if (arena) {
        bool won = false, ran = false;
        for (int i = 0; i < b.n; ++i) {
            const combat::Fighter& f = b.f[i];
            if (pool_files()) {
                if (!f.gone && f.up() && f.status() == party::Okay && !f.team()) won = true;
            } else if (i < b.party_size) {
                if (f.status() == party::Okay || f.status() == party::Animated) won = true;
                if (f.status() == party::Running) ran = true;
            }
        }
        r = won ? combat::Won : ran ? combat::Fled : combat::Lost;
        plat::log("[fight] the arena: %s\n", won ? "won" : ran ? "fled" : "lost");
    }
    int out = 0;
    for (int i = b.party_size; i < b.n; ++i)
        if (!b.f[i].gone && !b.f[i].up()) ++out;
    vm->set(0x7EC8, static_cast<uint16_t>(out));
    // (Pool's arena leaves the result word 0 - its 0x80 / 0x81 are set on
    // the other paths only)
    vm->set(0x7EC7, static_cast<uint16_t>(arena && pool_files() ? 0 : r));
    if (in_demo) {
        // The demo (demo_facts.md 5): no clean-up, experience, treasure or
        // results; the script goes on (all down: the demo ends)
        end_fight_art();
        fight_clear_monsters();
        palette_fight(false);
        ground->clear();
        delete fg;
        fg = nullptr;
        if (r == combat::Lost) {
            end_demo(c);
            return;
        }
        fight_back_to_game(c);
        clear_menu_line(c);
        waiting = false;
        handle(vm->resume());
        return;
    }
    if (r == combat::Won && arena && (rolf || !pool_files())) {
        // A duel's experience (Pool ovr6 +0x6, Curse +0x3D - any arena fight
        // there): the selected's hit dice x 100 to each in the fight, nothing
        // gathered from the beaten
        const int hd = pt->selected >= 0 && pt->selected < pt->count ? pt->m[pt->selected].rec[0xE5] : 0;
        int members = 0;
        for (int i = 0; i < b.party_size; ++i)
            if (b.f[i].up() && b.f[i].status() != party::Animated) ++members;
        fg->share = combat::award(b, hd * 100 * members, true);
        if (!members) fg->share = hd * 100;     // (Curse: shown though no one in the fight took it)
        plat::log("[fight] the duel's experience: %d\n", fg->share);
    } else
    // The beaten monsters' coins and items go to the treasure
    if (r == combat::Won) {
        for (int m = 0; m < 7; ++m) ground->money[m] += fg->out.money[m];
        // (Darkness: its script place 0x1E - GAME.OVR 0x4820)
        int dropped = 0;
        const int first_new = ground->n;
        if (vm->get(darkness_files() ? 0x1E : 0x7EE3) != 1)
            for (int i = b.party_size; i < b.n; ++i) {
                const combat::Fighter& f = b.f[i];
                if (f.gone || !f.team() || f.status() == party::Okay || f.status() == party::Running) continue;
                // (Pool: a LOAD MONSTER's copies - all but the first of its
                // group - hold their items the other way round: copied item by
                // item onto the front of the copy's list, ovr4 +0x5B7; DOSBox,
                // the monster leaders' gnolls)
                bool copy = false;
                if (pool_files() && ml && f.monster >= 0)
                    for (int j = 0; j < f.monster && !copy; ++j) copy = ml->m[j].group == ml->m[f.monster].group;
                for (int kk = 0; kk < f.n_items; ++kk) {
                    const int k = copy ? f.n_items - 1 - kk : kk;
                    // (Pool, GAME.OVR ovr6 +0x10A: an item of no value is left
                    // behind unless fewer than 8 were taken so far and 1d10 <= 3;
                    // one with a value is always taken - DOSBox: the kobolds'
                    // weapons stayed)
                    if (pool_files() && !items::Item{f.items[k]}.value() && (dropped >= 8 || rng.roll(10, 1) > 3)) continue;
                    ++dropped;
                    if (ground->n >= items::kMaxGround && pool_files() && ground->n > first_new) {
                        // (Pool: the list has no end, and the items taken last
                        // show first - ovr6 +0x1A0 puts each on the list's front;
                        // the engine's room for 64 keeps those: the fight's
                        // earliest one gives way, its experience still counts -
                        // DOSBox: the alerted goblin guards' 80-odd)
                        const uint8_t* old_one = ground->item[first_new];
                        if (static_cast<int8_t>(old_one[0x32]) > 0) ground->lost_plus += static_cast<int8_t>(old_one[0x32]);
                        memmove(ground->item[first_new], ground->item[first_new + 1],
                                static_cast<size_t>(ground->n - first_new - 1) * items::kRecordSize);
                        --ground->n;
                    }
                    if (ground->n >= items::kMaxGround) {
                        // (no room: its experience still counts)
                        if (static_cast<int8_t>(f.items[k][0x32]) > 0) ground->lost_plus += static_cast<int8_t>(f.items[k][0x32]);
                        continue;
                    }
                    memcpy(ground->item[ground->n], f.items[k], items::kRecordSize);
                    ground->item[ground->n][0x34] = 0;
                    ++ground->n;
                }
            }
        int total = fg->out.exp + combat::money_exp(ground->money, fight_plat_xp());
        for (int k = 0; k < ground->n; ++k) {
            const int plus = static_cast<int8_t>(ground->item[k][0x32]);
            if (plus > 0) total += 400 * plus;
        }
        total += 400 * ground->lost_plus;
        fg->share = combat::award(b, total, !no_exp_lines());
        plat::log("[fight] won: monsters' experience %d, coins %d %d %d %d %d %d %d, %d items; %d, each %d\n", fg->out.exp,
                  ground->money[0], ground->money[1], ground->money[2], ground->money[3], ground->money[4], ground->money[5],
                  ground->money[6], ground->n, total, fg->share);
        // The party's own thrown weapons: there to take back (no experience for them)
        for (int k = 0; k < fg->n_thrown && ground->n < items::kMaxGround; ++k)
            memcpy(ground->item[ground->n++], fg->thrown[k], items::kRecordSize);
    } else {
        fg->share = 0;
        if (r == combat::Fled) ground->clear();
    }
    fight_cleanup_party(r);
    // (Pool: the first member is the selected one after a fight - ovr6
    // +0x1296; the treasure's Detect is then theirs)
    if (pool_files() && pt->count) pt->selected = 0;
    // Left behind (a flight): the fallen stay
    if (r == combat::Fled)
        for (int i = pt->count - 1; i >= 0; --i)
            if (pt->m[i].health() != party::Okay && pt->m[i].health() != party::Animated) leave_party(i);
    for (int i = 0; i < pt->count; ++i) rules::recalc(pt->m[i], *names, d->facts);
    end_fight_art();
    fight_clear_monsters();
    palette_fight(false);
    // The result screen
    c.clear(0);
    layout::outer(c, d->tables, d->frame_tiles);
    if (arena) {
        // "You have won the duel." / "You have lost the duel." (Curse: "...
        // the fight.", its result word 0x80; fled: "The party has fled."),
        // "The duelist receives N" "experience points." - Pool ovr6 +0x95A
        put(c, fw(r == combat::Won ? profile::kDuelWon : r == combat::Fled ? profile::kHasFled : profile::kDuelLost), 1, 3, 10);
        char t[48];
        snprintf(t, sizeof t, "%s%d", fw(profile::kDuelistReceives), r == combat::Won ? fg->share : 0);
        put(c, t, 1, 5, 10);
        put(c, fw(profile::kExpPoints), 1, 7, 10);
        press_enter_line(c);
        fg->st = FSt::Results;
    } else if (r == combat::Lost && d->prof->variant.the_end) {
        // (Pool's: its title, the rejoicing in a window from row 5, the key
        // line magenta - ovr6 +0x15B9, DOSBox)
        char t[24];
        ow(d->prof->variant.the_end, t, sizeof t);
        put(c, t, 19 - static_cast<int>(strlen(t)) / 2, 1, 15);       // (its window 1-38)
        text::Writer tw;
        text::begin(tw, c, fw(profile::kRejoice), text::Region{2, 5, 37, 22}, 10, true);
        text::step(tw, c, d->font, -1);
        clear_menu_line(c);
        put(c, fw(profile::kPressAnyKey), 0, text::kMenuRow, 13);
        fg->st = FSt::Destroyed;
    } else if (r == combat::Lost) {
        put(c, fw(profile::kRejoice), 1, 3, 10);
        clear_menu_line(c);
        put(c, fw(profile::kPressAnyKey), 0, text::kMenuRow, 15);
        fg->st = FSt::Destroyed;
    } else {
        fg->res_word = static_cast<uint8_t>(r == combat::Fled ? profile::kHasFled
                                            : none_beaten ? profile::kFoundTreasure : profile::kHasWon);
        if (!npc_share_page(c)) results_page(c);
    }
    fg->until = 0;
    dirty(0, pic::kScreenH);
}

// A fight's "The party has won!" ... "experience points." page
void results_page(pic::Canvas& c)
{
    c.clear(0);
    layout::outer(c, d->tables, d->frame_tiles);
    put(c, fw(fg->res_word), 1, 3, 10);
    char t[48];
    if (!no_exp_lines()) {
        snprintf(t, sizeof t, "%s%d", fw(profile::kEachReceives), fg->share);
        put(c, t, 1, 5, 10);
        put(c, fw(profile::kExpPoints), 1, 7, 10);
    }
    press_enter_line(c);
    fg->st = FSt::Results;
    dirty(0, pic::kScreenH);
}

// Pool: the hired NPCs take their share of the coins before the results
// page, a fight's or the treasure's alone (GAME.OVR ovr6 +0x12E8, called by
// the treasure step +0x1559 after the experience is shared; DOSBox: Valjevo's
// offering box with the arena's warrior along). Shares: an NPC (0xF7 >= 0x80)
// who is okay counts its share byte 0xF8 & 7, everyone else 1; of each coin
// (copper ... jewellery) above 0 the NPCs take coins / shares, the quotient
// cut to its low byte (the original keeps it in a byte - Game Fixes: all
// of it; SPEC 12), times their shares. A page names each NPC who took
// ("<name> takes and hides his share.") when any coin was there.
bool npc_share_page(pic::Canvas& c)
{
    if (!pool_files() || !ground || !fw(profile::kNpcShare)[0]) return false;
    int npc = 0, total = 0;
    for (int i = 0; i < pt->count; ++i) {
        const uint8_t* r = pt->m[i].rec;
        if (r[0xF7] > 0x7F && pt->m[i].health() == party::Okay) {
            npc += r[0xF8] & 7;
            total += r[0xF8] & 7;
        } else {
            ++total;
        }
    }
    if (npc <= 0 || total <= 0) return false;
    bool any = false;
    for (int k = 0; k < 7; ++k) {
        if (ground->money[k] <= 0) continue;
        int each = ground->money[k] / total;
        if (!rules::fixes()) each &= 0xFF;
        ground->money[k] -= each * npc;
        any = true;
    }
    if (!any) return false;
    c.clear(0);
    layout::outer(c, d->tables, d->frame_tiles);
    int row = 0;
    for (int i = 0; i < pt->count; ++i) {
        const uint8_t* r = pt->m[i].rec;
        if (r[0xF7] <= 0x7F || pt->m[i].health() != party::Okay || !r[0xF8]) continue;
        char t[64];
        pt->m[i].name(t, 20);
        snprintf(t + strlen(t), sizeof t - strlen(t), "%s", fw(profile::kNpcShare));
        text::Writer tw;
        text::begin(tw, c, t, text::Region{5, static_cast<int8_t>(5 + row), 34, 22}, 10, true);
        text::step(tw, c, d->font, -1);
        row += 2;
    }
    plat::log("[fight] the NPCs take their share: coins %d %d %d %d %d %d %d\n", ground->money[0], ground->money[1],
              ground->money[2], ground->money[3], ground->money[4], ground->money[5], ground->money[6]);
    press_enter_line(c);
    fg->st = FSt::NpcShare;
    dirty(0, pic::kScreenH);
    return true;
}

// After the results: the treasure, or straight back to the script
void fight_done(pic::Canvas& c)
{
    const bool lost = fg->st == FSt::Destroyed;
    vm->set(0x7F70, 0);
    vm->set(0x7F71, 0);
    vm->set(0x7F72, 0);
    vm->set(0x7EE3, 0);
    vm->set(0x7EE6, 0);
    if (darkness_files())
        // Darkness's own (GAME.OVR 0x5B03: its block's bytes 0, 0x1B-0x1E,
        // 0x2E - script places 1, 0x1C-0x1F, 0x2F)
        for (uint16_t a : {0x01, 0x1C, 0x1D, 0x1E, 0x1F, 0x2F}) vm->set(a, 0);
    delete fg;
    fg = nullptr;
    if (lost) {
        // The party is gone: back to the party menu
        while (pt->count) leave_party(pt->count - 1);
        screen = Screen::PartyMenu;
        draw_party_menu(c);
        // (Pool: its THE END screen's key ends the game - back to DOS, the
        // viewer here; ovr6 +0x1631, DOSBox)
        if (d->prof->variant.the_end) exit_wanted = true;
        return;
    }
    fight_back_to_game(c);
    // (Pool: the treasure's menu after every fight not lost, "View Pool
    // Exit" when there's nothing - ovr6 +0x1569 -> +0xECD; DOSBox: the arena)
    if (ground->any_money() || ground->n || pool_files()) {
        open_treasure(c);
        return;
    }
    clear_menu_line(c);
    waiting = false;
    handle(vm->resume());
}

// COMBAT with no monsters: the treasure the script set out
void fight_treasure_only(pic::Canvas& c)
{
    // (Pool: the first member selected here too - its treasure step, ovr6
    // +0x1527, runs +0x11AC (+0x1296) first, fight or none: DOSBox, Sokal
    // Keep's hammer)
    if (pool_files() && pt->count) pt->selected = 0;
    if (!ground->any_money() && !ground->n) {
        waiting = false;
        handle(vm->resume());
        return;
    }
    make_room(16, "the fight");
    if (!fg) fg = make_fight();
    if (!fg) {
        open_treasure(c);
        return;
    }
    load_fight_words();
    int total = combat::money_exp(ground->money, fight_plat_xp());
    for (int k = 0; k < ground->n; ++k) {
        const int plus = static_cast<int8_t>(ground->item[k][0x32]);
        if (plus > 0) total += 400 * plus;
    }
    total += 400 * ground->lost_plus;
    // (no fighters: the party shares as the games do - the survivors).
    // The party's fighter blocks (since v0.86.1 a fight's fighters are its
    // own block: a fresh Battle has none - the treasure step crashed on it)
    release_fighters(fg->fighters);
    fg->fighters = new (std::nothrow) combat::Fighter[pt->count > 0 ? pt->count : 1];
    if (!fg->fighters) {
        delete fg;
        fg = nullptr;
        open_treasure(c);
        return;
    }
    combat::Battle& b = fg->b;
    b.~Battle();
    new (&b) combat::Battle();         // (in place: a temporary would take 10 KB of stack)
    b.f = fg->fighters;
    b.cap = pt->count > 0 ? pt->count : 1;
    for (int i = 0; i < pt->count; ++i) {
        b.f[i].rec = pt->m[i].rec;
        b.f[i].member = i;
    }
    b.n = b.party_size = pt->count;
    fg->share = combat::award(b, total, !no_exp_lines());
    fg->treasure_only = true;
    screen = Screen::Fight;
    plat::log("[fight] treasure: experience %d, each %d\n", total, fg->share);
    if (!npc_share_page(c)) treasure_page(c);
}

// The treasure's "The party has found Treasure!" page (no fight)
void treasure_page(pic::Canvas& c)
{
    c.clear(0);
    layout::outer(c, d->tables, d->frame_tiles);
    char t[48];
    if (fw(profile::kFoundTreasure2)[0]) {
        // Secret: the experience first, "The party has found " "treasure!" under it
        snprintf(t, sizeof t, "%s%s", fw(profile::kFoundTreasure), fw(profile::kFoundTreasure2));
        put(c, t, 1, 10, 10);
    } else {
        put(c, fw(profile::kFoundTreasure), 1, 3, 10);
    }
    if (!no_exp_lines()) {
        snprintf(t, sizeof t, "%s%d", fw(profile::kEachReceives), fg->share);
        put(c, t, 1, 5, 10);
        put(c, fw(profile::kExpPoints), 1, 7, 10);
    }
    press_enter_line(c);
    fg->st = FSt::Results;
    dirty(0, pic::kScreenH);
}

// ---- Ticks and taps
void fight_tick(uint32_t now, pic::Canvas& c)
{
    if (!fg || !fg->until || static_cast<int32_t>(now - fg->until) < 0) return;
    fg->until = 0;
    switch (fg->st) {
    case FSt::Begin: begin_round(c); break;
    case FSt::Ai: ai_step(c); break;
    case FSt::RoundEnd: round_end(c); break;
    case FSt::RoundRest:
        clear_menu_line(c);
        round_rest(c);
        break;
    case FSt::Pages:
        if (fg->fx_on) {
            if (!fx_step(c)) {
                fx_end(c);
                page_fx(c);
            }
            break;
        }
        if (fg->at < fg->pages) {
            show_page(c);
            break;
        }
        page_over(c);
        fg->pages = fg->at = 0;
        fg->ptext_used = 0;
        clear_panel(c, 10, 21);
        if (fg->cur >= 0) draw_panel_for(c, fg->cur);
        if (fg->yielding >= 0) {
            // (off the field; its panel stays as it was - the original
            // clears only the message rows)
            combat::Fighter& y = fg->b.f[fg->yielding];
            fg->yielding = -1;
            y.rec[0x195] = party::Unconscious;
            y.rec[0x196] = 0;
            y.size = 0;
            combat::occupancy(fg->b);
            draw_field(c);
        }
        // A sweep's next blow and its pages, then on as before
        if (fg->sweep_by >= 0 && sweep_next(c)) {
            say(c, fg->after);
            break;
        }
        switch (fg->after) {
        case FSt::Next:
            end_turn(c);
            break;
        case FSt::Begin:
            status_line(c, "");
            next_turn(c);                   // (their delay stays: the spell comes later)
            break;
        case FSt::Ai:
            fg->st = FSt::Ai;
            fg->until = 1;
            break;
        case FSt::Move:
            fg->st = FSt::Move;
            draw_fight(c);
            move_prompt(c);
            break;
        case FSt::UseAim:
            begin_targeting(c, fg->spell);
            break;
        case FSt::Clouds:
            round_clouds(c);
            break;
        case FSt::Resume:
            start_turn(c);                  // (the turn goes on after its message)
            break;
        default:
            after_attack(c);
            break;
        }
        break;
    default: break;
    }
}


// ---- Casting in a fight
// Cast: "NAME's Spells in Memory" (the list screen); a camp spell: "Camp
// Only Spell"; one with a casting time: "Begins Casting" - it goes off when
// their turn comes round again this round, unless they're hurt first ("lost
// a spell"). Then the targets as the spell's aim says (themselves; one to
// four picked with Aim's Target; an area round a fighter or a square:
// Manual), "Casts a Spell" with "Spell: NAME" on row 23, and what it did:
// "is Blessed", "takes 7 points of damage from Fire", "falls asleep" ...

int spell_power(const combat::Fighter& f)
{
    // An item's spell: the item's level (6 - the monsters' own, 12)
    if (fg->use_item >= 0 && fg->spell && mrules) return combat::power_of(f, mrules->tables, fg->spell, true);
    // (the caster's own: the same level the spell is cast at - Secret's
    // paladin rule, a class-changed human's skill levels)
    if (fg->spell && mrules) return combat::power_of(f, mrules->tables, fg->spell);
    const combat::Rec r = f.rec;
    const int cl = r[0x109], pa = r[0x10C], ra = r[0x10D], mu = r[0x10E];
    if (cl == 0 && mu == 0 && pa < 9 && ra < 8) return 6;
    return cl > mu ? cl : mu;
}

void forget_spell(combat::Fighter& f, int sp)
{
    for (int k = 0; k < magic::kListSize; ++k)
        if (f.rec[magic::kListAt + k] == sp) {
            f.rec[magic::kListAt + k] = 0;
            return;
        }
}

// The Robe of Vermin's effect on the one casting (a d2 of 1): the spell
// miscasts as it goes off - "Casts a Spell" / "Spell: NAME" and nothing
// else, the spell kept in memory, the turn over (listing ovr023:1403; in a
// fight the cast's words are "Casts a Spell", not "miscasts")
bool fight_miscast(pic::Canvas& c, int sp)
{
    combat::Fighter& f = fg->b.f[fg->cur];
    const uint8_t vermin = d->prof->fight.facts.items.vermin;
    if (!vermin || !f.has(vermin) || rng.roll(2, 1) != 1) return false;
    char t[40];
    snprintf(t, sizeof t, "%s%s", fw(profile::kSpellColon), spell_name(sp));
    status_line(c, t);
    add_page(fg->cur, fw(profile::kCastsASpell));
    fg->spell = 0;
    f.moves = f.attacks[0] = f.attacks[1] = 0;
    say(c, FSt::Next);
    return true;
}
// A spell back in memory (the computer's, taken out when chosen, miscast)
void remember_spell(combat::Fighter& f, int sp)
{
    for (int k = 0; k < magic::kListSize; ++k)
        if (!f.rec[magic::kListAt + k]) {
            f.rec[magic::kListAt + k] = static_cast<uint8_t>(sp);
            return;
        }
}

const combat::FightSpell* fight_spell(int sp)
{
    return combat::fight_spell(d->prof->fight.spells, d->prof->fight.n_spells, sp);
}

void fight_cast_menu(pic::Canvas& c)
{
    combat::Fighter& f = fg->b.f[fg->cur];
    if (f.member < 0 || !load_magic()) return;
    uint8_t ids[84];
    const int n = magic::in_memory(pt->m[f.member], mrules->tables, false, ids, 84);
    if (!n) return;
    pt->selected = f.member;
    sl.learning = sl.scrolls = sl.choosing = false;
    sl.casting = true;
    sl.fighting = true;
    sl.viewing = false;
    build_lines(ids, n);
    screen = Screen::SpellList;
    draw_spells(c);
}

// The fire shield's flame: "flame type: Hot Cold"; leaving it: "Abort spell? Yes No"
void flame_menu(pic::Canvas& c)
{
    fg->st = FSt::FlameAsk;
    text::build(menu, fw(profile::kFlameType), fw(profile::kHotCold));
    menu.selected = 0;
    hl_inherit();                       // (Darkness: GAME.OVR 0x2E555)
    show_menu_line(c);
}

void begin_targeting(pic::Canvas& c, int sp)
{
    if (!load_magic()) {
        end_turn(c);
        return;
    }
    const spells::Entry e = spells::entry(mrules->tables, sp);
    fg->spell = sp;
    fg->sp_n = 0;
    fg->sp_area = -1;
    fg->pick_budget = -1;
    fg->aim_x = fg->aim_y = -1;
    const int aim = e.aim & 0x0F;
    const combat::FightSpell* bfs = fight_spell(sp);
    // ("flame type: Hot Cold" - only in a game whose program has the question:
    // Champions' has neither words, and the empty menu waited for ever - the
    // fight fuzzer, 2026-11-01; there the spell goes as the computer's)
    if (bfs && bfs->does == combat::SpellDoes::FireShield && !fg->b.f[fg->cur].quick && !fg->b.f[fg->cur].team() &&
        d->prof->fight.words[profile::kFlameType] && d->prof->fight.words[profile::kHotCold]) {
        // "flame type: Hot Cold"
        fg->sp_t[fg->sp_n++] = fg->cur;
        flame_menu(c);
        return;
    }
    if (aim == 0) {
        fg->sp_t[fg->sp_n++] = fg->cur;
        cast_now(c);
        return;
    }
    if (e.targets == spells::kParty) {
        // Their whole side
        const bool mine = fg->b.f[fg->cur].team();
        for (int k = 0; k < fg->b.n; ++k)
            if (fg->b.f[k].up() && fg->b.f[k].size && fg->b.f[k].team() == mine) fg->sp_t[fg->sp_n++] = k;
        cast_now(c);
        return;
    }
    if (aim >= 8 && aim <= 14) {
        fg->sp_need = 1;
        fg->sp_area = e.aim & 7;
    } else if (aim == 5) {
        // One by one until 2+ pass the budget (Faerie Fire: the level, by
        // size; else 2d4, by Hit Dice - never shown, as in the original)
        const combat::FightSpell* afs = fight_spell(sp);
        fg->pick_size = afs && afs->does == combat::SpellDoes::Faerie;
        fg->pick_budget = fg->pick_size ? combat::power_of(fg->b.f[fg->cur], mrules->tables, sp) : rng.roll(4, 2);
        fg->sp_need = combat::kMaxFighters;
    } else if (aim == 15) {
        fg->sp_need = 1;
    } else {
        fg->sp_need = (e.aim & 3) + 1;
    }
    start_aim(c);
    // The first shown: the nearest enemy for a spell against them, else the nearest other
    const combat::FightSpell* fs = fight_spell(sp);
    const bool hostile = fs && fs->does != combat::SpellDoes::Ours && fs->does != combat::SpellDoes::Heal &&
                         fs->does != combat::SpellDoes::Haste && fs->does != combat::SpellDoes::Prayer &&
                         fs->does != combat::SpellDoes::Mirror && fs->does != combat::SpellDoes::CureBlind &&
                         (fs->does != combat::SpellDoes::Affect || e.targets == spells::kCombat);
    fg->ci = -1;
    for (int k = 0; k < fg->n_cand && fg->ci < 0; ++k)
        if (fg->cand[k] != fg->cur && (!hostile || fg->b.f[fg->cand[k]].team() != fg->b.f[fg->cur].team())) fg->ci = k;
    for (int k = 0; k < fg->n_cand && fg->ci < 0; ++k)
        if (fg->cand[k] != fg->cur) fg->ci = k;
    if (fg->ci < 0) fg->ci = 0;
    aim_menu(c);
}

void fight_spell_chosen(int sp, pic::Canvas& c)
{
    sl.fighting = false;
    screen = Screen::Fight;
    draw_fight(c);
    if (!sp) {
        fight_menu(c);
        return;
    }
    combat::Fighter& f = fg->b.f[fg->cur];
    const spells::Entry e = spells::entry(mrules->tables, sp);
    if (e.when == 0) {
        fight_menu(c);
        note(c, fw(profile::kCampOnly));
        return;
    }
    const combat::FightSpell* fs = fight_spell(sp);
    if (!fs || fs->does == combat::SpellDoes::NotYet) {
        fight_menu(c);
        error(c, "Not in the engine yet.");
        return;
    }
    const int delay = e.delay / 3;
    if (delay > 0) {
        f.spell = sp;
        f.delay = f.delay > delay ? delay : 1;
        f.moves = f.attacks[0] = f.attacks[1] = 0;     // (the turn is the casting's now)
        clear_menu_line(c);
        add_page(fg->cur, fw(profile::kBeginsCasting));
        fg->after = FSt::Turn;
        fg->st = FSt::Pages;
        fg->at = 0;
        show_page(c);
        // (the turn ends without its delay going: next_turn after the page)
        fg->after = FSt::Begin;
        return;
    }
    if (fight_miscast(c, sp)) return;
    begin_targeting(c, sp);
}

void spell_target(pic::Canvas& c, int t)
{
    combat::Battle& b = fg->b;
    if (t < 0 && !(fg->manual && fg->sp_area >= 0)) return;
    if (t >= 0 && !can_target(t, nullptr) && !(fg->manual && fg->sp_area >= 0)) return;
    const combat::FightSpell* bfs = fight_spell(fg->spell);
    if (bfs && bfs->does == combat::SpellDoes::Bolt) {
        // A bolt: from the target on, away from the caster
        const int x = t >= 0 ? b.f[t].x : fg->mx, y = t >= 0 ? b.f[t].y : fg->my;
        fg->aim_x = x;
        fg->aim_y = y;
        fg->sp_n = combat::bolt_line(b, fg->t, fg->cur, x, y, 7, fg->sp_t, combat::kMaxFighters);
        if (!fg->sp_n) fg->sp_t[fg->sp_n++] = t >= 0 ? t : fg->cur;
        cast_now(c);
        return;
    }
    if (fg->sp_area >= 0) {
        // The area's centre: the fighter or the square
        const int x = t >= 0 ? b.f[t].x : fg->mx, y = t >= 0 ? b.f[t].y : fg->my;
        fg->aim_x = x;
        fg->aim_y = y;
        int in[combat::kMaxFighters];
        const int n = combat::in_area(b, fg->t, x, y, fg->sp_area, in, combat::kMaxFighters);
        fg->sp_n = 0;
        for (int k = 0; k < n; ++k) fg->sp_t[fg->sp_n++] = in[k];
        if (!fg->sp_n) fg->sp_t[fg->sp_n++] = t >= 0 ? t : fg->cur;     // (an empty square: no one)
        if (blast_asked(fg->spell)) {
            ask_blast_delay(c);                 // (Secret's Delayed Blast Fireball: when)
            return;
        }
        cast_now(c);
        return;
    }
    for (int k = 0; k < fg->sp_n; ++k)
        if (fg->sp_t[k] == t) {
            note(c, fw(profile::kAlreadyTargeted));
            return;
        }
    fg->sp_t[fg->sp_n++] = t;
    if (fg->sp_n >= fg->sp_need || fg->sp_n >= combat::kMaxFighters ||
        (fg->pick_budget >= 0 && combat::picks_done(b, fg->sp_t, fg->sp_n, fg->pick_budget, fg->pick_size))) {
        cast_now(c);
        return;
    }
    aim_menu(c);
}

// What a spell did, as pages: a burst on each one hurt, sparkles on each
// one it works on, the skull on the fallen. `each`: a spell aimed at
// several picked one by one flies to each (with its sound) after the first.
void spell_pages(int caster, const combat::SpellLine* l, int n, const combat::FightSpell& fs, bool each)
{
    char t[48];
    int flown[combat::kMaxFighters], n_flown = 0;
    if (fg->sp_n > 0) flown[n_flown++] = fg->sp_t[0];
    for (int k = 0; k < n; ++k) {
        const combat::Fighter& f = fg->b.f[l[k].who];
        const int before = fg->pages;
        switch (l[k].did) {
        case combat::Did::Word:
            ow(fs.word, t, sizeof t);
            if (!t[0]) break;           // (no word: nothing said - Secret's Feeblemind, GAME.OVR 0x37F63)
            add_page(l[k].who, t);
            if (fs.does != combat::SpellDoes::Poison) page_fx_set(1, hit_fx(l[k].who, true));
            break;
        case combat::Did::Damage: {
            damage_text(l[k].amount, fs.kind, t, sizeof t);
            add_page(l[k].who, t);
            page_fx_set(1, hit_fx(l[k].who, false));
            break;
        }
        case combat::Did::Unaffected: add_page(l[k].who, fw(profile::kIsUnaffectedS)); break;
        case combat::Did::Word2:
            ow(fs.word2, t, sizeof t);
            if (!t[0]) break;           // (no word for it: nothing said)
            add_page(l[k].who, t);
            page_fx_set(1, hit_fx(l[k].who, true));
            break;
        case combat::Did::Word3:
            ow(fs.word3, t, sizeof t);
            if (!t[0]) break;           // (no word for it: nothing said)
            add_page(l[k].who, t);
            page_fx_set(1, hit_fx(l[k].who, true));
            break;
        case combat::Did::Word4:
            ow(fs.word4, t, sizeof t);
            if (!t[0]) break;           // (no word for it: nothing said)
            add_page(l[k].who, t);
            page_fx_set(1, hit_fx(l[k].who, true));
            break;
        case combat::Did::Risen: {
            // Back on its feet: "gets back up" (the party's side) / "stands up and grins"
            add_page(l[k].who, fw(f.team() ? profile::kStandsUp : profile::kGetsBackUp));
            Fx r;
            r.kind = Fx::Redraw;
            page_fx_set(1, r);
            break;
        }
        case combat::Did::Misses: add_page(caster, fw(profile::kMisses), nullptr, l[k].who); break;
        case combat::Did::LostImage: add_page(l[k].who, fw(profile::kLostImage)); break;
        case combat::Did::Fallen:
            if (fg->pages > 0 && !fg->page_dropped) fg->page[fg->pages - 1].fx[1] = skull_fx(l[k].who);
            break;
        case combat::Did::Healed: {
            char w2[24];
            cw(f.hp() >= f.hp_max() ? profile::kFullyHealed : profile::kPartlyHealed, w2, sizeof w2);
            add_page(l[k].who, w2);
            page_fx_set(1, hit_fx(l[k].who, true));
            break;
        }
        case combat::Did::Down: {
            const combat::Down dn = combat::down_state(f);
            if (hl()) q_fall(l[k].who, dn);
            else if (dn == combat::Down::Dead) add_page(l[k].who, fw(profile::kIsKilled));
            else if (dn == combat::Down::Dying) add_page(l[k].who, fw(profile::kGoesDownS), fw(profile::kIsDying));
            else add_page(l[k].who, fw(profile::kGoesDownS));
            page_fx_set(1, skull_fx(l[k].who));
            bozak_burst(l[k].who);
            break;
        }
        }
        // Its rolls (CYD_COMBAT_ROLLS): to hit (a touch), the save, the damage
        // as rolled - two at most, the save and damage before a touch's roll
        if (fg->rolls && fg->pages > before) {
            const combat::Did dd = l[k].did;
            if (dd == combat::Did::Damage || dd == combat::Did::Unaffected || dd == combat::Did::Word ||
                dd == combat::Did::Word2 || dd == combat::Did::Word3 || dd == combat::Did::Word4 ||
                dd == combat::Did::Misses || dd == combat::Did::LostImage) {
                const combat::Roll* hit = take_roll(combat::RollKind::Hit, l[k].who, 0xFF);
                const combat::Roll* sv = dd == combat::Did::Misses ? nullptr : take_roll(combat::RollKind::Save, l[k].who, 0xFE);
                const combat::Roll* dm = dd == combat::Did::Misses ? nullptr : take_roll(combat::RollKind::Damage, l[k].who, 0xFE);
                if (sv && dm) hit = nullptr;
                page_roll(hit);
                page_roll(sv);
                page_roll(dm);
            }
        }
        // The spell flies on to the next one picked
        if (each && fg->pages > before && l[k].who != caster) {
            bool seen = false;
            for (int j = 0; j < n_flown; ++j) seen = seen || flown[j] == l[k].who;
            if (!seen && n_flown < combat::kMaxFighters) {
                flown[n_flown++] = l[k].who;
                Fx e = fly_fx(caster, l[k].who, combat::spell_flight(5, 30));
                e.sound = sound::kSpell;
                fg->page[before].fx[0] = e;
            }
        }
        // (the Dark Queen: a caster hurt loses the spell under way - res 0x8F04;
        // Feeblemind's "loses intellect" too, DQK.EXE 0x53F17)
        if (l[k].did == combat::Did::Damage && fg->pages > before) q_lost_spell(l[k].who, false);
        if (l[k].did == combat::Did::Word && fs.does == combat::SpellDoes::Feeble) q_lost_spell(l[k].who, false);
    }
}

// Spiritual Hammer: a party fighter with its effect holds the hammer ("Gains
// an item" when it's new); without it the hammer goes
void fight_hammer(int i)
{
    combat::Fighter& f = fg->b.f[i];
    if (f.member < 0 || f.member >= pt->count) return;
    party::Character& ch = pt->m[f.member];
    const int before = ch.n_items;
    const bool gained = rules::keep_hammer(ch, *names, d->facts);
    if (gained) add_page(i, fw(profile::kGainsItem));
    if (gained || ch.n_items != before) rules::recalc(ch, *names, d->facts);
    f.items = ch.items;
    f.n_items = ch.n_items;
}

// ---- Secret's Delayed Blast Fireball (secret_facts_2.md 26): the caster
// is asked "Blast Delay in segments (1-50)? "; (n + 5) / 10 rounds, 0 at
// once; else "Sets a delayed blast fireball." and when the rounds are past
// (the end of a round) "Delayed Blast Fireball goes off" at the square
// aimed at: those in its area then, the caster's level d6 + level
bool blast_asked(int sp)
{
    return secret_files() && sp == 0x73 && fg->use_item < 0 && fw(profile::kBlastDelay)[0];
}

void ask_blast_delay(pic::Canvas& c)
{
    snprintf(take_prompt, sizeof take_prompt, "%s", fw(profile::kBlastDelay));
    input_engine = true;
    engine_ask = EngineAsk::BlastDelay;
    input_prompt = take_prompt;
    input_max = 2;
    input_len = 0;
    input_buf[0] = 0;
    input_mode = Input::Number;
    draw_input(c);
}

void blast_delay(int n, pic::Canvas& c)
{
    if (n < 0) n = 0;
    if (n > 50) n = 50;
    const int rounds = (n + 5) / 10;
    combat::Battle& b = fg->b;
    int slot = -1;
    for (int k = 0; k < 4; ++k)
        if (fg->blast[k].caster < 0) slot = k;
    if (rounds == 0 || slot < 0 || fg->aim_x < 0) {
        cast_now(c);
        return;
    }
    const int i = fg->cur;
    combat::Fighter& f = b.f[i];
    const int sp = fg->spell;
    fg->spell = 0;
    forget_spell(f, sp);
    char t[40];
    snprintf(t, sizeof t, "%s%s", fw(profile::kSpellColon), spell_name(sp));
    status_line(c, t);
    draw_fight(c);
    add_page(i, fw(profile::kCastsASpell));
    add_page(i, fw(profile::kSetsBlast));
    auto& bl = fg->blast[slot];
    bl.caster = static_cast<int8_t>(i);
    bl.x = static_cast<uint8_t>(fg->aim_x);
    bl.y = static_cast<uint8_t>(fg->aim_y);
    bl.area = static_cast<uint8_t>(fg->sp_area > 0 ? fg->sp_area : 0);
    bl.rounds = static_cast<uint8_t>(rounds);
    bl.pw = static_cast<uint8_t>(combat::power_of(f, mrules->tables, sp, false));
    fg->aim_x = fg->aim_y = -1;
    f.attacks[0] = f.attacks[1] = 0;
    say(c, FSt::Next);
}

void spell_pages(int caster, const combat::SpellLine* l, int n, const combat::FightSpell& fs, bool each);

// Darkness's Monster Summoning (R10; GAME.OVR 0x2FFE8): the reserve's
// monsters out of the fight, in order, at most 2 a cast - each next to a
// fighter (the list's first, then the next once its 8 squares are taken:
// N, NE, E ...) on a free square of plain ground (3D 0x17, else 0x45),
// on the caster's side, up with its full hit points: "gates in"; none:
// "summons is not answered"
void summon(int caster, const combat::FightSpell& fs)
{
    combat::Battle& b = fg->b;
    int placed = 0;
    if (ml && ml->reserve_from >= 0) {
        const uint8_t plain = (vm->get(0x22) & 0xFF) ? 0x17 : 0x45;
        int anchor = 0, dir = 0;
        for (int k = ml->reserve_from; k < ml->n && placed < 2; ++k) {
            combat::Monster& m = ml->m[k];
            const combat::Rec r(&m);
            if (r[0x195] != 9) continue;
            int x = -1, y = -1;
            // (the original tests the one square - GAME.OVR 0x30139 - and a
            // bigger monster's other squares may lie on someone or a wall;
            // with Game Fixes every square it takes is free plain ground)
            const int size = (r[0xDE] & 7) ? (r[0xDE] & 7) : 1;
            int sx[4] = {}, sy[4] = {};
            const int n_sq = rules::fixes() ? combat::squares(size, sx, sy) : 1;
            auto free_at = [&](int xx, int yy) {
                for (int q = 0; q < n_sq; ++q) {
                    const int px = xx + sx[q], py = yy + sy[q];
                    if (px < 0 || py < 0 || px >= combat::kW || py >= combat::kH || b.who[py][px] || b.ground[py][px] != plain)
                        return false;
                }
                return true;
            };
            while (x < 0 && anchor < b.n) {
                const combat::Fighter& a = b.f[anchor];
                for (; a.size && dir < 8 && x < 0; ++dir) {
                    const int xx = a.x + combat::dx(dir), yy = a.y + combat::dy(dir);
                    if (!free_at(xx, yy)) continue;
                    x = xx;
                    y = yy;
                }
                if (x < 0) {
                    ++anchor;
                    dir = 0;
                }
            }
            if (x < 0) break;
            combat::Fighter* f = combat::add_fighter(b);
            if (!f) break;
            if (!ml->reserve_icon) {
                load_monster_icon(8 + ml->reserve_group, ml->g[ml->reserve_group]->icon);
                ml->reserve_icon = true;
            }
            f->rec = r;
            f->aff = m.aff;
            f->n_aff = &m.n_aff;
            monster_items(*f, m);
            f->monster = static_cast<int16_t>(k);
            f->icon = static_cast<int16_t>(8 + m.group);
            f->quick = true;
            f->max_aff = combat::kMonsterAffects;
            f->flags2 = m.flags2;
            f->magic_res = m.magic_res;
            r[0x195] = party::Okay;
            r[0x196] = 1;
            r[0x1A4] = r[0x78];
            r[0x197] = b.f[caster].rec[0x197];
            f->x = static_cast<int16_t>(x);
            f->y = static_cast<int16_t>(y);
            f->size = static_cast<int16_t>(size);
            f->facing = b.f[caster].facing;
            combat::occupancy(b);
            char t[48];
            ow(fs.word, t, sizeof t);
            add_page(b.n - 1, t);
            ++placed;
        }
    }
    if (!placed) {
        char t[48];
        ow(fs.word2, t, sizeof t);
        add_page(caster, t);
    }
    Fx e;
    e.kind = Fx::Redraw;
    page_fx_set(1, e);
}

// Darkness's area spells with a burst that sounds again (R16)
bool burst_spell(int sp)
{
    for (uint8_t s : d->prof->sound.burst_spells)
        if (s && s == sp) return true;
    return false;
}

void blast_off(Fight::Blast& bl)
{
    combat::Battle& b = fg->b;
    const combat::FightSpell* fs = fight_spell(0x73);
    const int caster = bl.caster;
    bl.caster = -1;
    if (!fs || caster >= b.n) return;
    int who[combat::kMaxFighters];
    const int n = combat::in_area(b, fg->t, bl.x, bl.y, bl.area, who, combat::kMaxFighters);
    const int first = fg->pages;
    add_page(-1, fw(profile::kBlastGoesOff));
    combat::SpellLine lines[24];
    b.aim_x = bl.x;
    b.aim_y = bl.y;
    rolls_clear();
    const int m = n ? combat::cast(b, mrules->tables, caster, 0x73, *fs, who, n, rng, lines, 24, bl.pw) : 0;
    b.aim_x = b.aim_y = -1;
    spell_pages(caster, lines, m, *fs, false);
    if (fg->pages > first) {
        Fx e;
        e.kind = Fx::Fly;
        e.fl = combat::spell_flight(5, 30);
        e.x0 = e.x1 = bl.x;
        e.y0 = e.y1 = bl.y;
        e.sound = sound::kFireball;
        fg->page[first].fx[0] = e;
    }
}

void cast_now(pic::Canvas& c, bool forgotten)
{
    if (fg->jump) {
        tr_jump(c);
        return;
    }
    combat::Battle& b = fg->b;
    const int i = fg->cur;
    combat::Fighter& f = b.f[i];
    const int sp = fg->spell;
    fg->spell = 0;
    const combat::FightSpell* fs = fight_spell(sp);
    const bool from_item = fg->use_item >= 0;
    if (!from_item) status_line(c, "");
    if (!fs) {
        end_turn(c);
        return;
    }
    int pw = 0;
    draw_fight(c);
    const int first_page = fg->pages;
    if (from_item) {
        pw = combat::power_of(f, mrules->tables, sp, true);
    } else {
        // The spell leaves their memory; "Casts a Spell", "Spell: NAME"
        if (!forgotten) forget_spell(f, sp);
        char t[40];
        snprintf(t, sizeof t, "%s%s", fw(profile::kSpellColon), spell_name(sp));
        status_line(c, t);
        add_page(i, fw(profile::kCastsASpell));
    }
    draw_icon(c, i, true);
    // Where it flies: the square aimed at, else the first one it's for
    int ax = fg->aim_x, ay = fg->aim_y;
    if (ax < 0 && fg->sp_n > 0) {
        ax = b.f[fg->sp_t[0]].x;
        ay = b.f[fg->sp_t[0]].y;
    }
    fg->aim_x = fg->aim_y = -1;
    const spells::Entry se = spells::entry(mrules->tables, sp);
    const int aim = se.aim & 0x0F;
    const bool each = aim >= 1 && aim <= 4 && se.targets != spells::kParty;
    if (fs->does == combat::SpellDoes::Defoliate && ax >= 0)
        fg->sp_n = combat::cone(b, fg->t, i, ax, ay, 3, 1, fg->sp_t, combat::kMaxFighters);   // 3 squares, 1 ray
    if ((fs->does == combat::SpellDoes::Cone || fs->does == combat::SpellDoes::Fear) && ax >= 0) {
        // A cone from the caster through the square aimed at: Fear 3 rays of 6 squares, Cone of Cold 2 of (level + 1) / 2
        const int lv = from_item ? pw : combat::power_of(f, mrules->tables, sp, false);
        const bool fear = fs->does == combat::SpellDoes::Fear;
        const int reach = fear ? 6 : ((lv + 1) / 2 > 1 ? (lv + 1) / 2 : 1);
        fg->sp_n = combat::cone(b, fg->t, i, ax, ay, reach, fear ? 3 : 2, fg->sp_t, combat::kMaxFighters);
    }
    if ((fs->does == combat::SpellDoes::Cloud || fs->does == combat::SpellDoes::Poison) && ax >= 0) {
        // A cloud on the field (Stinking Cloud 2 x 2 from the square aimed at, Cloudkill 3 x 3 round it) for
        // the caster's level in rounds; those in it breathe it
        const bool poison = fs->does == combat::SpellDoes::Poison;
        const int rounds = from_item ? pw : combat::power_of(f, mrules->tables, sp, false);
        if (combat::lay_cloud(b, fg->t, ax, ay, rounds, poison)) {
            add_page(i, fw(poison ? profile::kPoisonCloud : profile::kNoxiousCloud));
            Fx e;
            e.kind = Fx::Redraw;
            page_fx_set(1, e);
        }
        fg->sp_n = combat::cloud_fighters(b, ax, ay, fg->sp_t, combat::kMaxFighters, poison);
    }
    if (fs->does == combat::SpellDoes::Blade && ax >= 0) {
        // Darkness's Blade Barrier (R2): Stinking Cloud's 2 x 2 squares of
        // knives for 3 rounds a level; those in it get cut (a save at the cast)
        const int lv = from_item ? pw : combat::power_of(f, mrules->tables, sp, false);
        if (combat::lay_cloud(b, fg->t, ax, ay, 3 * lv, false, true)) {
            Fx e;
            e.kind = Fx::Redraw;
            page_fx_set(1, e);
        }
        fg->sp_n = combat::cloud_fighters(b, ax, ay, fg->sp_t, combat::kMaxFighters, false);
    }
    if (fs->does == combat::SpellDoes::Summon) {
        // Darkness's Monster Summoning (R10): the fight's reserve
        summon(i, *fs);
        if (from_item) spend_item(i, sp);
        f.attacks[0] = f.attacks[1] = 0;
        say(c, FSt::Next);
        return;
    }
    if (fs->does == combat::SpellDoes::Teleport) {
        // Dimension Door: to the square aimed at, within 3 squares a level, when it can stand there
        const int lv = from_item ? pw : combat::power_of(f, mrules->tables, sp, false);
        int len = 0;
        combat::path(b, fg->t, i, ax, ay, true, &len);
        if (ax >= 0 && len / 2 <= 3 * lv && combat::teleport(b, fg->t, i, ax, ay)) {
            char said[48];
            ow(fs->word, said, sizeof said);
            add_page(i, said);
            Fx r;
            r.kind = Fx::Redraw;
            page_fx_set(1, r);
            focus(i, 2);
        }
        if (from_item) spend_item(i, sp);
        f.attacks[0] = f.attacks[1] = 0;
        f.moves = 0;
        say(c, FSt::Next);
        return;
    }
    combat::SpellLine lines[24];
    b.aim_x = ax;
    b.aim_y = ay;
    rolls_clear();
    const int n = combat::cast(b, mrules->tables, i, sp, *fs, fg->sp_t, fg->sp_n, rng, lines, 24, pw);
    b.aim_x = b.aim_y = -1;
    spell_pages(i, lines, n, *fs, each);
    // Restoration: the levels themselves; Spiritual Hammer: the hammer
    if (fs->does == combat::SpellDoes::Restore)
        for (int k = 0; k < n; ++k) {
            const combat::Fighter& rf = b.f[lines[k].who];
            if (lines[k].did == combat::Did::Word && rf.member >= 0 && rf.member < pt->count)
                create::restore(pt->m[rf.member], mrules->tables);
        }
    if (fs->does == combat::SpellDoes::Hammer) fight_hammer(i);
    // Darkness's Energy Drain (R3): one level from the one touched, the drain's own words
    if (fs->does == combat::SpellDoes::EnergyDrain && fg->sp_n > 0 && b.f[fg->sp_t[0]].up()) {
        combat::Event ev[8];
        int nev = 0;
        rolls_clear();
        combat::drain(b, fg->sp_t[0], 1, ev, 8, &nev, -1);
        for (int k = 0; k < nev; ++k) event_page(ev[k]);
    }
    // Strength, items and effects changed: the party's values again
    if (fs->does == combat::SpellDoes::Enlarge || fs->does == combat::SpellDoes::Reduce ||
        fs->does == combat::SpellDoes::GiantStrength ||
        fs->does == combat::SpellDoes::RemoveCurse || fs->does == combat::SpellDoes::Dispel ||
        fs->does == combat::SpellDoes::Restore)
        for (int k = 0; k < pt->count; ++k) rules::recalc(pt->m[k], *names, d->facts);
    // The spell flies with its first page, with its sound: the fireball's
    // roar, the bolt's crackle (its own picture), else the spell's
    if (fg->pages > first_page) {
        const bool bolt = sp == d->prof->sound.lightning_spell;
        Fx e;
        e.kind = ax >= 0 ? (b.n_bolt > 0 ? Fx::Bolt : Fx::Fly) : Fx::None;
        e.fl = combat::spell_flight(bolt && b.n_bolt == 0 ? 6 : 5, bolt && b.n_bolt == 0 ? 50 : 30);
        e.x0 = static_cast<uint8_t>(f.x);
        e.y0 = static_cast<uint8_t>(f.y);
        e.x1 = static_cast<uint8_t>(ax < 0 ? f.x : ax);
        e.y1 = static_cast<uint8_t>(ay < 0 ? f.y : ay);
        e.sound = static_cast<uint8_t>(sp == d->prof->sound.fireball_spell ? sound::kFireball
                                       : bolt                              ? sound::kLightning
                                                                           : sound::kSpell);
        fg->page[first_page].fx[0] = e;
        // Darkness: an area spell's burst plays the fireball's effect again
        // where it lands (R16)
        if (ax >= 0 && burst_spell(sp) && fg->page[first_page].fx[1].kind == Fx::None &&
            !fg->page[first_page].fx[1].sound)
            fg->page[first_page].fx[1].sound = sound::kFireball;
    }
    if (from_item) spend_item(i, sp);
    f.attacks[0] = f.attacks[1] = 0;
    say(c, FSt::Next);
}

// Turn: "NAME turns undead...", then each "is turned" / "Is destroyed", or
// "Nothing Happens..."
void fight_turn_undead(pic::Canvas& c)
{
    combat::Battle& b = fg->b;
    const int i = fg->cur;
    add_page(i, fw(profile::kTurnsUndead));
    int out[16];
    const int n = combat::turn_undead(b, fg->t, i, rng, out, 16);
    for (int k = 0; k < n; ++k) {
        if (out[k] >= 1000) {
            add_page(out[k] - 1000, fw(profile::kIsDestroyed));
            page_fx_set(1, skull_fx(out[k] - 1000));
        } else {
            add_page(out[k], fw(profile::kIsTurned));
            page_fx_set(1, hit_fx(out[k], true));
        }
    }
    if (!n) add_page(-1, fw(profile::kNothingHappens));
    draw_fight(c);
    say(c, FSt::Next);
}

// The fight menu's choices (by their keys): Curse's menu, and Darkness's
// first row
void fight_menu_key(pic::Canvas& c, char key)
{
    combat::Fighter& f = fg->b.f[fg->cur];
    switch (key) {
    case 'M': start_move(c); break;
    case 'V':
        if (f.member >= 0) {
            pt->selected = f.member;
            palette_fight(false);
            view_character(c);
        }
        break;
    case 'A':
        fg->spell = 0;              // (a weapon's aim)
        fg->use_item = -1;
        start_aim(c);
        break;
    case 'Q':
        f.quick = true;
        fg->st = FSt::Ai;
        fg->until = plat::now_ms() + 200;
        CYD_TEST_FX(Quick, 200, 0);
        clear_menu_line(c);
        break;
    case 'D': done_menu(c); break;
    case 0: break;
    case 'Y':
        if (kr_can_yell(f)) kr_yell(c);
        break;
    case 'C': fight_cast_menu(c); break;
    case 'T': fight_turn_undead(c); break;
    case 'U':
        // The items screen (Use there; Exit comes back here)
        if (f.member >= 0 && pt->m[f.member].n_items) {
            pt->selected = f.member;
            palette_fight(false);
            view_from = Screen::Fight;
            items_direct = true;
            open_items(c);
        }
        break;
    default: break;
    }
}

// Curse's Done menu (Guard Delay Quit Bandage Speed Exit) - Darkness's on
// its menu's second row
void done_key(pic::Canvas& c, char key)
{
    combat::Fighter& f = fg->b.f[fg->cur];
    switch (key) {
    case 'G':
        CYD_TEST_ACT(fg->b, fg->cur, Guard, -1, 0);
        f.guarding = true;
        add_page(fg->cur, fw(profile::kGuarding));
        f.delay = 0;
        say(c, FSt::Next);            // (the turn is over: not an attack)
        break;
    case 'D':
        f.delay = 1;
        status_line(c, "");
        next_turn(c);
        break;
    case 'Q': end_turn(c); break;
    case 'B': {
        CYD_TEST_ACT(fg->b, fg->cur, Bandage, -1, 0);
        const int k = combat::bandage(fg->b);
        if (k >= 0) {
            add_page(k, fw(profile::kBandaged));
            f.delay = 0;
            say(c, FSt::Next);            // (the turn is over: not an attack)
        }
        break;
    }
    case 'S': speed_menu(c); break;
    case 'E': fight_menu(c); break;
    case 'A':
        // (Champions: its Done menu's Auto - Curse's Quick)
        f.quick = true;
        fg->st = FSt::Ai;
        fg->until = plat::now_ms() + 200;
        CYD_TEST_FX(Quick, 200, 0);
        clear_menu_line(c);
        break;
    default: break;
    }
}

void fight_tap(int x, int y, pic::Canvas& c)
{
    if (!fg) return;
    const int col = x / 8;
    if (note_until) {
        redraw_menu(c);
        if (fg->st == FSt::Move) move_prompt(c);
        return;
    }
    switch (fg->st) {
    case FSt::More:
        page_more(c);
        return;
    case FSt::Pages:
        if (fg->fx_on) {                // a tap cuts the pictures short and moves on
            fx_end(c);
            fg->fx_at = 2;
        }
        if (fg->until) fg->until = 1;   // a tap moves on
        return;
    case FSt::OldAge:
        old_age_next(c);
        return;
    case FSt::NpcShare:
        if (fg->treasure_only) treasure_page(c);
        else results_page(c);
        return;
    case FSt::Results:
    case FSt::Destroyed:
        if (fg->treasure_only) {
            fg->treasure_only = false;
            delete fg;
            fg = nullptr;
            fight_back_to_game(c);
            open_treasure(c);
            return;
        }
        fight_done(c);
        return;
    case FSt::Ai: {
        // A tap takes the party back from Quick (the original's Space): every
        // player character runs by the player again; one acting now gets
        // the menu at once
        bool any = false;
        for (int k = 0; k < fg->b.n; ++k) {
            combat::Fighter& o = fg->b.f[k];
            if (o.member < 0 || o.rec[0xF7] >= 0x80 || o.status() == party::Animated || !o.quick || o.fleeing)
                continue;
            o.quick = false;
            any = true;
        }
        if (any && fg->cur >= 0 && fg->b.f[fg->cur].member >= 0 && !fg->b.f[fg->cur].quick && fg->b.f[fg->cur].up())
            fight_menu(c);
        return;
    }
    case FSt::Menu: {
        // (the Dark Queen's two rows of buttons from y 177)
        const bool q2 = hl() && text::two_rows(menu);
        if (y < (q2 ? 176 : text::kMenuTapTop)) return;
        combat::Fighter& f = fg->b.f[fg->cur];
        if (dk_menu()) {
            // Darkness's two rows: Guard, Delay, Bandage, Speed, End (as
            // Curse's Done menu's Quit) are on it; View comes with them
            const int row = q2 ? (y < 188 ? text::kMenuRow - 1 : text::kMenuRow) : y / 8;
            const int item = text::two_rows(menu) ? text::hit_row(menu, col, row) : text::hit(menu, col);
            const char k = text::key(menu, item);
            if (k == 'G' || k == 'D' || k == 'B' || k == 'S' || k == 'E') {
                done_key(c, k == 'E' ? 'Q' : k);
                return;
            }
            if (k == 'V' && f.member >= 0) {
                pt->selected = f.member;
                palette_fight(false);
                view_character(c);
                return;
            }
            if (k == 'V') return;
            fight_menu_key(c, k);
            return;
        }
        fight_menu_key(c, text::key(menu, text::hit(menu, col)));
        return;
    }
    case FSt::Move: {
        if (y >= text::kMenuTapTop) {
            // (the Dark Queen's Undo / Done buttons)
            if (hl() && text::key(menu, text::hit(menu, col)) == 'U') {
                fight_back(c);
                return;
            }
            if (hl() && text::hit(menu, col) < 0) return;
            end_move(c);
            return;
        }
        if (x < kViewX || x >= kViewX + 7 * kSq || y < kViewY || y >= kViewY + 7 * kSq) return;
        const combat::Fighter& f = fg->b.f[fg->cur];
        const int sx = fg->b.vx + (x - kViewX) / kSq, sy = fg->b.vy + (y - kViewY) / kSq;
        // The direction from the fighter's nearest square to the tapped one
        int fx = f.x, fy = f.y;
        if (f.size == 3 || f.size == 4) fx = sx > f.x ? f.x + 1 : f.x;
        if (f.size == 2 || f.size == 4) fy = sy > f.y ? f.y + 1 : f.y;
        const int dir = combat::direction(fx, fy, sx, sy);
        if (dir < 8) try_step(c, dir);
        return;
    }
    case FSt::Aim: aim_tap(x, y, c); return;
    case FSt::DoneMenu:
        if (y < text::kMenuTapTop) return;
        done_key(c, text::key(menu, text::hit(menu, col)));
        return;
    case FSt::Speed: {
        if (y < text::kMenuTapTop) return;
        int speed = vm->get(0x4BFC) & 0xFF;
        const uint16_t high = static_cast<uint16_t>(vm->get(0x4BFC) & 0xFF00);
        switch (text::key(menu, text::hit(menu, col))) {
        case 'S': if (speed < 9) ++speed; break;
        case 'F': if (speed > 0) --speed; break;
        case 'E': done_menu(c); return;
        default: return;
        }
        vm->set(0x4BFC, static_cast<uint16_t>(high | speed));
        speed_menu(c);
        return;
    }
    case FSt::AllyAsk: {
        if (y < text::kMenuTapTop) return;
        const char k = text::key(menu, text::hit(menu, col));
        if (k != 'Y' && k != 'N') return;
        if (k == 'Y' && fg->pending >= 0) {
            vm->set(0x7F33, 1);
            // NPCs on the party's side turn on it
            for (int i = 0; i < fg->b.n; ++i) {
                combat::Fighter& o = fg->b.f[i];
                if (o.rec[0xF7] >= 0x80 && o.status() == party::Okay && !o.team()) {
                    o.rec[0x197] = 1;
                    o.target = -1;
                }
            }
            fg->b.f[fg->cur].target = fg->pending;
            strike(c, fg->pending);
            fg->pending = -1;
            say(c, FSt::Turn);
            return;
        }
        fg->pending = -1;
        fight_menu(c);
        return;
    }
    case FSt::FlameAsk: {
        if (y < text::kMenuTapTop) return;
        const char k = text::key(menu, text::hit(menu, col));
        if (k != 'H' && k != 'C') return;
        fg->b.flame = k == 'H' ? 1 : 2;
        cast_now(c);
        return;
    }
    case FSt::FlameAbort: {
        if (y < text::kMenuTapTop) return;
        const char k = text::key(menu, text::hit(menu, col));
        if (k == 'N') {
            flame_menu(c);
            return;
        }
        if (k != 'Y') return;
        // The spell is gone (an item's use) and the turn over
        if (fg->use_item >= 0) spend_item(fg->cur, fg->spell);
        else forget_spell(fg->b.f[fg->cur], fg->spell);
        fg->spell = 0;
        fg->use_item = -1;
        add_page(fg->cur, fw(profile::kSpellAborted));
        draw_fight(c);
        say(c, FSt::Next);
        return;
    }
    case FSt::AbortAsk: {
        if (y < text::kMenuTapTop) return;
        const char k = text::key(menu, text::hit(menu, col));
        if (k == 'N') {
            aim_menu(c);
            return;
        }
        if (k != 'Y') return;
        if (fg->use_item >= 0) spend_item(fg->cur, fg->spell);
        else forget_spell(fg->b.f[fg->cur], fg->spell);
        fg->spell = 0;
        fg->use_item = -1;
        fg->b.f[fg->cur].spell = 0;
        add_page(fg->cur, fw(profile::kSpellAborted));
        draw_fight(c);
        say(c, FSt::Next);
        return;
    }
    case FSt::ContinueAsk: {
        if (y < text::kMenuTapTop) return;
        const char k = text::key(menu, text::hit(menu, col));
        if (k != 'Y' && k != 'N') return;
        clear_menu_line(c);
        fg->b.no_action = fg->b.round + 15;
        fg->continuing = k == 'Y';
        if (k == 'Y') begin_round(c);
        else end_fight(c);
        return;
    }
    case FSt::FleeAsk: {
        if (y < text::kMenuTapTop) return;
        const char k = text::key(menu, text::hit(menu, col));
        if (k == 'N') {
            fight_menu(c);
            return;
        }
        if (k != 'Y') return;
        const int i = fg->cur;
        const bool away = combat::flee(fg->b, fg->t, i, rng);
        add_page(i, fw(away ? profile::kGotAway : profile::kEscapeBlocked));
        draw_fight(c);
        say(c, FSt::Next);            // (the turn is over: not an attack)
        return;
    }
    default: return;
    }
}

// Esc in a fight: Move undoes the move, menus go back
bool fight_back(pic::Canvas& c)
{
    if (!fg) return false;
    switch (fg->st) {
    case FSt::Move: {
        combat::Battle& b = fg->b;
        combat::Fighter& f = b.f[fg->cur];
        const int x = f.x, y = f.y;
        f.x = fg->move_x;
        f.y = fg->move_y;
        combat::occupancy(b);
        bool clear = true;
        int sx[4], sy[4];
        const int n = combat::squares(f.size, sx, sy);
        for (int k = 0; k < n; ++k)
            if (b.who[f.y + sy[k]][f.x + sx[k]] != fg->cur + 1) clear = false;
        if (!clear) {
            f.x = x;
            f.y = y;
            combat::occupancy(b);
            end_turn(c);
            return true;
        }
        f.moves = fg->move_left;
        focus(fg->cur, 2);
        draw_fight(c);
        fight_menu(c);
        return true;
    }
    case FSt::Aim:
        aim_exit(c);
        return true;
    case FSt::DoneMenu: fight_menu(c); return true;
    case FSt::Speed: done_menu(c); return true;
    case FSt::AllyAsk:
    case FSt::FleeAsk: fight_menu(c); return true;
    case FSt::AbortAsk: aim_menu(c); return true;
    case FSt::ContinueAsk:
        clear_menu_line(c);
        end_fight(c);                   // (Esc: No)
        return true;
    case FSt::FlameAsk:
        fg->st = FSt::FlameAbort;
        text::build(menu, fw(profile::kAbortSpellQ), fw(profile::kYesNoF));
        menu.selected = 1;
        show_menu_line(c);
        return true;
    case FSt::FlameAbort: flame_menu(c); return true;
    default: return true;               // (no leaving a fight)
    }
}

// The pad in a fight: steps (Move) - 8 north, 7 / 9 north-west / north-east,
// 4 / 6 west / east, 2 south
bool fight_moving() { return fg && screen == Screen::Fight && fg->st == FSt::Move; }
void fight_move_prompt(pic::Canvas& c) { move_prompt(c); }

bool fight_way(int dir, pic::Canvas& c);

bool fight_act(Act a, pic::Canvas& c)
{
    if (!fg || (fg->st != FSt::Move && !(fg->st == FSt::Aim && fg->manual))) return false;
    int dir = 8;
    switch (a) {
    case Act::Forward: dir = 0; break;
    case Act::TurnLeft: dir = 7; break;
    case Act::TurnRight: dir = 1; break;
    case Act::StepLeft: dir = 6; break;
    case Act::StepRight: dir = 2; break;
    case Act::TurnAround: dir = 4; break;
    default: return false;
    }
    return fight_way(dir, c);
}

// A direction (0-7) in Move or Manual aim
bool fight_way(int dir, pic::Canvas& c)
{
    if (!fg || dir < 0 || dir > 7 || (fg->st != FSt::Move && !(fg->st == FSt::Aim && fg->manual))) return false;
    if (fg->st == FSt::Aim) {
        // Manual aim: the pad moves the square (the original's cursor keys)
        static const int kDx[8] = {0, 1, 1, 1, 0, -1, -1, -1}, kDy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
        const int x = fg->mx + kDx[dir], y = fg->my + kDy[dir];
        if (x >= 0 && x < combat::kW && y >= 0 && y < combat::kH) {
            fg->mx = x;
            fg->my = y;
        }
        aim_menu(c);
        return true;
    }
    try_step(c, dir);
    return true;
}

// After View Character in a fight
void fight_after_view(pic::Canvas& c)
{
    if (!fg) return;
    screen = Screen::Fight;
    palette_fight(true);
    combat::Fighter& f = fg->b.f[fg->cur];
    if (f.member >= 0) {
        party::Character& ch = pt->m[f.member];
        rules::recalc(ch, *names, d->facts);
        f.n_items = ch.n_items;
        compose_icon(f.member, ch);
        combat::recount_attacks(fg->b, fg->cur);       // (another weapon readied: its attacks)
    }
    draw_fight(c);
    fight_menu(c);
}

// ---- The treasure: "View Take Pool Share Exit" over the exploring screen
// (the shop's screen: Take - "Take: Money Items Exit" when both lie there -,
// Pool, Share as in a shop; the items: "Items: " and Take, newest first;
// leaving with treasure left: "There is still treasure left.  Do you want to
// go back and claim your treasure?" Yes No - what's left is lost)
bool treasure = false;                  // the shop screen is the treasure's
bool vault = false;                     // ... and Secret's vault (below)
bool treasure_on() { return treasure; }
bool vault_on() { return vault; }
void treasure_off() { treasure = vault = false; }

// A treasure word (GAME.OVR, read when shown)
const char* tw(int i)
{
    static char buf[4][48];
    static int k = 0;
    char* out = buf[k++ & 3];
    out[0] = 0;
    const uint32_t at = d->prof->fight.words[i];
    OvrWords src;
    if (!at || !src.open()) return out;
    read_word(src, at, out, 48);
    return out;
}

int detect_spell();
void treasure_menu(pic::Canvas& c);

// Detect: the spell cast as in camp - out of memory, its effect on the caster
void treasure_detect(pic::Canvas& c)
{
    party::Character* ch = pt->sel();
    const int sp = detect_spell();
    if (!ch || !sp || !load_magic()) return;
    magic::remove(*ch, sp, hl());
    const int pw = spells::power(*ch, mrules->tables, sp);
    spells::add_affect(*ch, d->prof->items.detect[0], spells::lasts(mrules->tables, sp, pw), pw, false);
    // "NAME casts" / the spell, "NAME is affected"
    char nm[20], w1[16], t[48];
    ch->name(nm, sizeof nm);
    cw(profile::kCasts, w1, sizeof w1);
    text::clear(c, text::kTextArea);
    snprintf(t, sizeof t, "%s %s", nm, w1);
    put(c, t, 1, 18, 10);
    put(c, spell_name(sp), 1, 19, 10);
    snprintf(t, sizeof t, "%s %s", nm, tw(profile::kIsAffectedW));
    put(c, t, 1, 21, 10);
    dirty_rows(17, 22);
    names->detect = true;
    // (Pool: the lines go after the game's delay - DOSBox: gone at the next screen)
    if (d->prof->game == games::Game::PoolOfRadiance) text_wipe_at = plat::now_ms() + 3 * game_delay_ms();
    treasure_menu(c);
}

void treasure_take(pic::Canvas& c)
{
    if (ground->any_money() && ground->n) {
        char prompt[12], words[24];
        snprintf(prompt, sizeof prompt, "%s", tw(profile::kTakeColon));
        snprintf(words, sizeof words, "%s", tw(profile::kMoneyItems));
        if (darkness_files()) {
            sw(profile::kDkTakeColon, prompt, sizeof prompt);
            sw(profile::kDkMoneyItems, words, sizeof words);
        }
        text::build(menu, prompt, words);
        menu.selected = 0;
        hl_first();                     // (Darkness: GAME.OVR 0x5480 sets the first)
        show_menu_line(c);
        return;
    }
    if (ground->any_money()) open_take(c);
    else if (ground->n) open_loot(c);
}

void draw_loot(pic::Canvas& c)
{
    c.clear(0);
    layout::outer(c, d->tables, d->frame_tiles);
    plist.row0 = 1;
    plist.row1 = 22;
    plist.col0 = 1;
    plist.n = ground->n;
    if (d->prof->variant.loot_heading) {
        // (Pool: "Loot" at column 17 of row 1, the items from row 4 - DOSBox)
        char w[8];
        ow(d->prof->variant.loot_heading, w, sizeof w);
        put(c, w, 17, 1, 15);
        plist.row0 = 4;
    }
    char prompt[12], what[8];
    snprintf(prompt, sizeof prompt, "%s", tw(profile::kItemsColon));
    snprintf(what, sizeof what, "%s", tw(profile::kTakeW));
    if (darkness_files()) {
        snprintf(prompt, sizeof prompt, "%s", d->w_items);         // ("items : " "Take")
        sw(profile::kDkTakeKey, what, sizeof what);
    }
    draw_list(c, prompt, what);
}

void open_loot(pic::Canvas& c)
{
    screen = Screen::Loot;
    plist = PickList{};
    draw_loot(c);
}

void take_loot(int i, pic::Canvas& c)
{
    party::Character* ch = pt->sel();
    if (!ch || i < 0 || i >= ground->n) return;
    const int at = ground->n - 1 - i;
    if (rules::too_heavy(*ch, ground->item[at], *names, d->facts)) {
        note(c, d->w_over);
        return;
    }
    rules::add_item(*ch, ground->item[at]);
    rules::recalc(*ch, *names, d->facts);
    for (int k = at; k + 1 < ground->n; ++k) memcpy(ground->item[k], ground->item[k + 1], items::kRecordSize);
    --ground->n;
    if (!ground->n) {
        screen = Screen::Shop;
        draw_shop(c);
        return;
    }
    draw_loot(c);
}

// Detect on the treasure screen (curse_finish_facts.md 4): the selected
// member's first memorized Detect Magic (in the list's order); 0 none
int detect_spell()
{
    const party::Character* ch = pt->sel();
    const uint8_t* ds = d->prof->items.detect;
    if (!ch || !ch->rec[0x196] || !ds[0]) return 0;
    for (int k = 0; k < magic::kListSize; ++k) {
        const int s = ch->rec[magic::kListAt + k];
        if (s && !(s & 0x80) && (s == ds[1] || s == ds[2] || s == ds[3])) return s;
    }
    return 0;
}

void treasure_menu(pic::Canvas& c)
{
    char words[48];
    char a[40], b2[16];
    if (darkness_files()) {
        // Darkness (facts_shops.md 4, 5): one string each - the treasure's
        // "View Take Pool Share Detect Exit" (Take with something there,
        // Share with coins, Detect with Detect Magic memorized), the vault's
        // "View Take Pool Money Items Exit" (Money / Items: the selected's to
        // put in it); the highlight keeps its place
        const party::Character* ch = pt->sel();
        const bool any = ground->any_money() || ground->n;
        unsigned mask;
        if (vault) {
            const bool coins = ch && (ch->money(4) || ch->money(5) || ch->money(6));
            mask = 1u | (any ? 2u : 0u) | 4u | (coins ? 8u : 0u) | (ch && ch->n_items ? 16u : 0u) | 32u;
        } else {
            mask = 1u | (any ? 2u : 0u) | 4u | (ground->any_money() ? 8u : 0u) | (detect_spell() ? 16u : 0u) | 32u;
        }
        sw(vault ? profile::kDkVaultMenu : profile::kDkTreasureMenu, a, sizeof a);
        keep_words(a, mask, words, sizeof words);
        text::build(menu, "", words);
        menu.selected = 0;
        hl_own(kDkTreasureMain);        // (GAME.OVR 0x5585 / 0x3261: its own place, from View)
        show_menu_line(c);
        return;
    }
    if (vault && d->prof->vault.menu6) {
        // Gateway's vault (GAME.OVR overlay 0x4E7F +0x21C, DOSBox): "Deposit
        // [Withdraw] [Vault] [Pool] [Share] Exit" - Withdraw with coins or
        // items in it, Vault when the one selected has items, Pool when the
        // party has coins, Share with coins in it
        bool party_coins = false;
        for (int i = 0; i < pt->count; ++i)
            if (any_money(pt->m[i])) party_coins = true;
        const party::Character* ch = pt->sel();
        const bool on[6] = {true, ground->any_money() || ground->n > 0, ch && ch->n_items > 0, party_coins,
                            ground->any_money(), true};
        words[0] = 0;
        OvrWords src;
        uint32_t at = d->prof->vault.menu6;
        const bool open = src.open();
        for (int k = 0; k < 6 && open; ++k) {
            char w[16];
            read_word(src, at, w, sizeof w);
            at += 1 + static_cast<uint32_t>(strlen(w));             // (the next Pascal string)
            if (on[k]) strlcat(words, w, sizeof words);
        }
        text::build(menu, "", words);
        menu.selected = 0;
        show_menu_line(c);
        return;
    }
    if (gateway() && !vault) {
        // Gateway: "View [Take] [Pool] [Share] [Detect] Exit" - Take with
        // something there, Pool when the party has coins, Share with coins
        // there, Detect with Detect Magic memorized (GAME.OVR 0x6B85 +0x12DE)
        bool party_coins = false;
        for (int i = 0; i < pt->count; ++i)
            if (any_money(pt->m[i])) party_coins = true;
        const unsigned mask = 1u | (ground->any_money() || ground->n ? 2u : 0u) | (party_coins ? 4u : 0u) |
                              (ground->any_money() ? 8u : 0u) | (detect_spell() ? 16u : 0u) | 32u;
        keep_words(tw(profile::kViewTakePoolShare), mask, words, sizeof words);
        text::build(menu, "", words);
        menu.selected = 0;
        show_menu_line(c);
        return;
    }
    if (vault) {
        // "View Take Pool Exit" (Take with something there)
        const auto& v = d->prof->vault;
        ow(v.view, words, sizeof words);
        if (ground->any_money() || ground->n) {
            ow(v.take, a, sizeof a);
            strlcat(words, a, sizeof words);
        }
        ow(v.pool_exit, a, sizeof a);
        strlcat(words, a, sizeof words);
        text::build(menu, "", words);
        menu.selected = 0;
        show_menu_line(c);
        return;
    }
    if (ground->any_money() || ground->n) {
        snprintf(a, sizeof a, "%s", tw(ground->any_money() ? profile::kViewTakePoolShare : profile::kViewTakePool));
        // " Detect Exit" with items there and a Detect Magic memorized by the one selected
        snprintf(b2, sizeof b2, "%s", tw(ground->n && detect_spell() ? profile::kDetectExit : profile::kExitSp));
        snprintf(words, sizeof words, "%s%s", a, b2);
    } else {
        snprintf(words, sizeof words, "%s", tw(profile::kViewPoolExit));
    }
    text::build(menu, "", words);
    menu.selected = 0;
    show_menu_line(c);
}

void open_treasure(pic::Canvas& c)
{
    dk_own[kDkTreasureMain] = 0;
    treasure = true;
    temple = false;
    screen = Screen::Shop;
    text::clear(c, text::kTextArea);
    dirty_rows(17, 22);
    if (pool_files() && !vault) {
        // (Pool's treasure screen, state 6: main_frame(1) - the frame with
        // the 3D view's, the left box cleared round the chest, the
        // wilderness map too; DOSBox)
        draw_frame(c);
    }
    if (d->prof->variant.treasure_pic && !vault) draw_treasure_pic();
    if (pt->count) draw_party(c, 17);
    if ((gateway() || d->prof->game == games::Game::PoolOfRadiance) && !vault) {
        c.fill(17 * 8, 15 * 8, 22 * 8, 8, 0);       // (Gateway's treasure: no place and time - DOSBox; Pool's too)
        dirty_rows(15, 15);
    }
    treasure_menu(c);
}

// ---- Secret's vault (secret_facts_2.md 24; GAME.OVR 0x50B2): the scripts'
// Deposit / Withdraw set the area word 0x4CFC and COMBAT opens it - the
// treasure's screen ("View Take Pool Exit", under the script's picture)
// over what VAULT.DAT in the save folder keeps (one vault for every saved
// game): 7 coin counts (4 bytes each: copper ... jewellery), then the
// items, 67 bytes each (Secret's .STF). View's Deposit (in Drop's place)
// puts coins or an item in it, Take takes them out, Pool puts the party's
// coins in; leaving writes the file. The engine keeps up to kMaxGround
// items there (the original: as many as its memory takes).
void vault_read_dk()
{
    const auto& v = d->prof->vault;
    char path[200], name[16];
    save_path(v.file, path, sizeof path);
    if (!plat::exists(path)) {
        snprintf(name, sizeof name, v.slot_file, vault_slot);
        save_path(name, path, sizeof path);
    }
    plat::File f = plat::open(path, "r");
    if (!f) return;
    uint8_t b[items::kRecordSize];
    // (Darkness: 3 counts - platinum gems jewelry; Gateway: 7 - copper on)
    const int nc = v.coins == 3 ? 3 : 7, first = nc == 3 ? 4 : 0;
    if (f.read(b, nc * 4) == nc * 4)
        for (int m = 0; m < nc; ++m) {
            const uint32_t x = b[m * 4] | b[m * 4 + 1] << 8 | b[m * 4 + 2] << 16 | static_cast<uint32_t>(b[m * 4 + 3]) << 24;
            ground->money[first + m] = x > 0x7FFFFFFF ? 0x7FFFFFFF : static_cast<int>(x);
        }
    while (ground->n < items::kMaxGround && f.read(b, 63) == 63) {
        memcpy(ground->item[ground->n], b, items::kRecordSize);
        ++ground->n;
    }
    f.close();
    for (int i = 0, j = ground->n - 1; i < j; ++i, --j) {
        uint8_t t[items::kRecordSize];
        memcpy(t, ground->item[i], sizeof t);
        memcpy(ground->item[i], ground->item[j], sizeof t);
        memcpy(ground->item[j], t, sizeof t);
    }
}

void vault_read()
{
    ground->clear();
    if (d->prof->vault.item_size == 63) {
        vault_read_dk();
        return;
    }
    char path[200];
    save_path(d->prof->vault.file, path, sizeof path);
    plat::File f = plat::open(path, "r");
    if (!f) return;
    uint8_t b[party::kSecretItemSize];
    if (f.read(b, 28) == 28)
        for (int m = 0; m < 7; ++m) {
            const uint32_t v = b[m * 4] | b[m * 4 + 1] << 8 | b[m * 4 + 2] << 16 | static_cast<uint32_t>(b[m * 4 + 3]) << 24;
            ground->money[m] = v > 0x7FFFFFFF ? 0x7FFFFFFF : static_cast<int>(v);
        }
    while (f.read(b, sizeof b) == static_cast<int>(sizeof b)) {
        // (an item of type 0x49 is followed by as many records as its
        // count, 0x39 - the original's loader reads them; none in the game)
        if (b[0x2E] == 0x49)
            for (int k = 0; k < b[0x39]; ++k) {
                uint8_t skip[party::kSecretItemSize];
                f.read(skip, sizeof skip);
            }
        if (ground->n >= items::kMaxGround) continue;
        memcpy(ground->item[ground->n], b, items::kRecordSize);
        items::fix_item(ground->item[ground->n]);
        name_from_words(ground->item[ground->n]);
        ++ground->n;
    }
    f.close();
    // The file's first item is the list's first line: the ground's last
    for (int i = 0, j = ground->n - 1; i < j; ++i, --j) {
        uint8_t t[items::kRecordSize];
        memcpy(t, ground->item[i], sizeof t);
        memcpy(ground->item[i], ground->item[j], sizeof t);
        memcpy(ground->item[j], t, sizeof t);
    }
}

bool vault_write()
{
    char path[200];
    save_path(d->prof->vault.file, path, sizeof path);
    if (!plat::exists(d->save_dir)) plat::mkdir(d->save_dir);
    plat::File f = plat::open(path, "w");
    if (!f) return false;
    uint8_t b[party::kSecretItemSize] = {};
    const auto& vf = d->prof->vault;
    const int first = vf.coins == 3 ? 4 : 0;                // (Darkness: platinum gems jewelry)
    for (int m = 0; m < vf.coins; ++m) {
        const uint32_t v = ground->money[first + m] > 0 ? static_cast<uint32_t>(ground->money[first + m]) : 0;
        for (int k = 0; k < 4; ++k) b[m * 4 + k] = static_cast<uint8_t>(v >> (8 * k));
    }
    bool ok = f.write(b, vf.coins * 4u) == vf.coins * 4u;
    const size_t isz = vf.item_size ? vf.item_size : sizeof b;
    for (int i = ground->n - 1; ok && i >= 0; --i) {        // (the list's order)
        memset(b, 0, sizeof b);
        memcpy(b, ground->item[i], items::kRecordSize);
        items::fix_item(b, true);                   // (the original's bytes in the files)
        ok = f.write(b, isz) == isz;
    }
    f.close();
    return ok;
}

void open_vault(pic::Canvas& c)
{
    vault_read();
    vault = true;
    menu.selected = 0;
    open_treasure(c);
}

bool can_part_with(int i, pic::Canvas& c);
void back_to_items(pic::Canvas& c);

// Secret's vault: an item put in it (View - Items - Deposit, the item put
// away first; no question)
void deposit_item(int i, pic::Canvas& c)
{
    party::Character& ch = *pt->sel();
    if (!can_part_with(i, c)) return;
    if (ground->n >= items::kMaxGround) {
        note(c, d->w_over);
        return;
    }
    // (the list's last line: the ground's first)
    memmove(ground->item[1], ground->item[0], static_cast<size_t>(ground->n) * items::kRecordSize);
    memcpy(ground->item[0], ch.items[i], items::kRecordSize);
    ++ground->n;
    rules::remove_item(ch, i);
    rules::recalc(ch, *names, d->facts);
    back_to_items(c);
}
