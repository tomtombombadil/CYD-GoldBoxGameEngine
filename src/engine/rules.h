// The AD&D rules the games apply to a character (Curse first): the values
// worked out from stats and readied items, money, carrying.
//
// Plain C++ (no Arduino), host-tested in tools/host_tests/test_dax.cpp.
// Rules learned from coab (Curse of the Azure Bonds); own code.
#pragma once

#include <cstdint>

#include "create.h"
#include "items.h"
#include "party.h"

namespace rules {

// Per-game item facts the rules need (from the profile)
struct ItemFacts {
    uint8_t arrow, quarrel;
    uint8_t elf_bonus[10];
    // Secret's weapon part (GAME.OVR 0x38151) gives that +1 to anyone with
    // one of those weapons - its program has no race test there (an
    // original's mistake: applied only with fixes() off)
    bool elf_bonus_any_race = false;
    // Effects in the stats: Strength, the giant strength potion's, Enlarge,
    // Friends, Feeblemind (0: none)
    uint8_t strength_fx = 0, giant_fx = 0, enlarge_fx = 0, friends_fx = 0, feeble_fx = 0;
    uint8_t friends_max = 0;            // Friends' Charisma at most this (Pool 25; 0: no limit)
    // The Strength effect over 18 makes 18/xx for a fighter by current levels
    // only (Darkness / Treasures GAME.OVR 0x3318E / 0x3CF1C: 0x153-0x155);
    // Secret's counts former levels too
    bool str_fighter_now = false;
    // Spiritual Hammer: its effect, the item's type and name words
    uint8_t hammer_fx = 0, hammer_type = 0, hammer_word = 0, hammer_word2 = 0;
    uint8_t hammer_code = 0;            // its byte 0x3E (Curse 0xA0, Pool 0x89; 0: 0xA0)
    // Constitution: the healing effect at 20 and up (0: none), and each
    // class's last level with a hit die (the program's table) - a change of
    // Constitution in use rebuilds the hit points with them
    uint8_t con_regen_fx = 0;
    uint8_t max_hd[8] = {};
    // Berserk (the Berserker sword's effect): given, its handler makes the
    // wearer computer-run (quick) and control 0xB3 (an NPC's 0xB2); taken
    // away, control 0xB3 back to 0 and the party's side
    uint8_t berserk_fx = 0;
    // Secret of the Silver Blades: the +1 to hit with elf_bonus's weapons
    // for every race (the original's; AD&D's elves alone when fixes()),
    // armour's +3 movement only for magic armour, and the Boots of Speed's
    // effect doubling the movement
    bool    light_magic_armour = true, sell_secret = false;
    // (profile.h items.cloak_slot / belt_slot: Darkness's and Treasures'
    // cloaks, Treasures' slot 5 - a bonus of their own, gone with magic armour)
    uint8_t cloak_slot = 0, belt_slot = 0;
    // Sell: a pile's price - Curse's family: every pile count x value / 20
    // (their arrows' test can never hold - Curse GAME.OVR 0x2984E, Pool
    // 0x22D57, Gateway 0x2FF90, Champions 0x295BC); sell_secret: arrows /
    // quarrels count x value / 20, others count x value, a pile of type
    // sell_keep (Secret's 0x49, GAME.OVR 0x29D6E) one's price
    uint8_t sell_keep = 0;
    uint8_t speed_fx = 0;
    // A race's own weapon (the Dark Queen's kender and the hoopak): the
    // engine race, the item type, its to-hit bonus (type 0: none)
    uint8_t race_weapon[3] = {};
    // The Dark Queen: with no weapon readied its routine (DQK resident
    // 0x9BF6) adds nothing - the THAC0 without Strength's to-hit
    bool unarmed_no_str_hit = false;
    // Champions: coin 0 (its record's 0xEB, here money place 2) weighs
    // nothing in the recalculation (GAME.OVR 0x37484 adds coins 1-6 to the
    // weight) though pooling and sharing move the weight with it (-1: none)
    int8_t weightless_coin = -1;
};

// The stats as they stand (the games' recalculation; spell_facts.md and
// coab's facts): from each stat's own value, readied items that work on
// stats (third effect byte 0x80 + code: 5 giant strength, 8 a +1 to one
// stat under 18, 2 dexterity, 6 / 10 / 12 / 13 others) and the effects
// (Strength, Enlarge, the potion's, Friends, Feeblemind). Bytes 0x10 +
// 2 i hold each stat's own value and 0x11 + 2 i the one in use; for the
// 18/xx percentage the other way round (0x1D own, 0x1C in use).
void stats(party::Character& c, const ItemFacts& f);

// Strength as one number: 3-17, 18 = 18, 19-23 = 18/01-50, /51-75,
// /76-90, /91-99, /00; 19-25 = 24-30
int strength_group(const party::Character& c);
// Strength's damage bonus (for those with the Strength flag 0x125) from a record
int strength_damage(const uint8_t* rec);
// (the same from its three bytes: 0x125 uses Strength, 0x11 Strength in use, 0x1C its 18/xx)
int strength_damage(int uses_strength, int str_full, int str00);
// Extra weight carried before slowing down (can be negative)
int max_encumbrance(const party::Character& c);
int dex_ac_bonus(const party::Character& c);

// Works out what the games keep up to date from stats and readied items:
// encumbrance, movement, AC, to-hit, damage dice / bonus, hands in use,
// attack level (into the record, as the games store them).
void recalc(party::Character& c, const items::Names& names, const ItemFacts& f);
// The same without the stats (the original's routine itself, which every
// combatant - monsters too - goes through at a battle's setup: coab's facts,
// the listing's sub_380E0 / sub_66C20)
void recalc_values(party::Character& c, const items::Names& names, const ItemFacts& f);

// ---- Locked doors (door_facts.md): door state 2 locked, 3 not pickable
// Bash: each member in party order (everyone, as the original) tries by
// the Strength in use until one breaks it. A not-pickable door: those too
// weak for it make *bash_gone true (Bash is offered no more until a step).
bool bash_door(const party::Party& p, int state, create::Dice& d, bool* bash_gone);
// Pick (locked doors only): each okay member rolls d100 against Open Locks
// (record 0xEB) until one opens it
bool pick_lock(const party::Party& p, create::Dice& d);
// Pick is offered: someone has a thief level that counts (the thief level,
// or a former one their new class has passed)
bool has_thief(const party::Party& p);
// Knock is offered: the first member with Knock (0x1F) memorized; -1 none
int knock_member(const party::Party& p, int knock_spell);

// Money: worth in gold (copper 1, silver 10, electrum 100, gold 200,
// platinum 1000 copper; gems and jewellery not counted), and paying
// `gold` from the coins, making change as the games do. round: half a
// gold piece rounds up (Pool's training: (copper + 100) / 200)
int gold_worth(const int money[7], bool round = false);
int gold_worth(const party::Character& c, bool round = false);
void pay(int money[7], int gold);
void pay(party::Character& c, int gold);
// Gateway's CALL 0x80F (its GAME.OVR seg 0xCB entry 0x75 with 3 - the
// rates DS 0x104C: 10 copper a silver, 10 silver an electrum, 2 electrum a
// gold, 5 gold a platinum, these coins' worths): each kind from copper to
// gold, while there's a coin of the next kind's worth, traded up - its
// scripts then pay in platinum
void trade_up(party::Character& c);

// Game Fixes (Tom, 2026-10-31): every coin is worth its copper (AD&D: 1 pp
// = 5 gp = 10 ep = 50 sp = 500 cp - the rates above), so a price is paid
// from any coins that together are worth it, with exact change, and the
// money is then held in the highest coins (fewer coins, less weight; gems
// and jewellery as they are). For the games with the five coins (Pool,
// Curse, Secret, Gateway) - the front end and the script machine decide
// when (the originals pay from the coins as they are).
long copper_worth(const int money[7]);
long copper_worth(const party::Character& c);
// The coins (0-4) worth `copper`, in the highest coins (each at most 65535)
void coins_of(long copper, int money[7]);
// The coins made the highest worth the same; never more coins than before.
// The character's carried weight (0x187) goes down by the coins saved.
void coins_up(int money[7]);
void coins_up(party::Character& c);
// `gold` paid: from the character's coins, what they can't cover from
// `other`'s (the counter; nullptr: none); both left in the highest coins.
// False (nothing paid) when the two together aren't worth it.
bool pay_any(party::Character& c, int* other, int gold);

// Coins: the party's player characters put all theirs in the pool; the
// pool shared out (each coin kind from jewellery down: equal shares, one
// more of the rest each while it lasts, then what's left to whoever can
// still carry it; the remainder stays in the pool). Encumbrance follows.
int max_load(const party::Character& c);       // 1500 + the strength allowance
void pool(party::Party& p, int money[7]);
void share(party::Party& p, int money[7]);

// True if the character can't take the item (16 items, or too heavy).
bool too_heavy(party::Character& c, const uint8_t* item, const items::Names& names, const ItemFacts& f, bool slots = true);
// Adds an item to a character (false: no room); an item the original got
// wrong comes in fixed when that's wanted (items::fix_item)
bool add_item(party::Character& c, const uint8_t* item);
// Every item of the character made as wanted (items::fix_item; original:
// the original's bytes, for the files) - a readied one put away and readied
// again around the change, so its effect comes or goes. True: any changed
// (recalc after).
bool fix_items(party::Character& c, bool original = false);

// A character's items (Items: Drop, Trade, Halve, Join)
void remove_item(party::Character& c, int i);
// An item that works while readied (its effect byte 0x3E 0x80 and up;
// items::passive says what each code does in this game), readied (`on`)
// or put away: Effect - the effect in 0x3D is theirs while readied (for
// good: no time); Alignment - keyed to an alignment (0x3D & 0x0F): the
// wrong one can't ready it (put away again) and takes 0x3D >> 4 damage;
// OgrePower (Pool) - Strength 18/00 through the strength effect while
// readied ("is stronger" when it's more); NeedGiant (Pool, the boulder) -
// put away again below Strength 19 ("Must have Giant Strength"). The stat
// items work through stats(). False when the item didn't stay readied;
// `says` what to tell the player.
enum class WornSays : uint8_t { Nothing, Stronger, NeedGiant };
bool worn(party::Character& c, int i, bool on, WornSays* says = nullptr);
// Pool of Radiance keeps a character's Strength and Charisma as they stand
// in the record, and in a strength effect's data the Strength it replaced
// (poolrad_combat_facts.md 3.2: the one in force; the others, data + 0x80,
// the Strength they'd give) and in Friends' the Charisma it replaced. The
// engine keeps the character's own values and each effect's gift (as
// Curse's): pool_stats_in turns a Pool character just read (record,
// effects) into that, pool_stats_out the other way for writing - the
// record's Strength 18/xx and Charisma (`pool`, a Pool record from
// party::to_pool) and the effects (`fx`, n_affects x kAffectSize bytes).
void pool_stats_in(party::Character& c, const ItemFacts& f);
void pool_stats_out(const party::Character& c, const ItemFacts& f, uint8_t* pool, uint8_t* fx);
// The game's berserk effect for worn() (0: none; set as the game opens)
void set_berserk_fx(uint8_t type);
// The game's rules for worn(): the data of an item's effect (Curse 0xFF:
// Dispel Magic leaves it; Pool 12), the strength effect Ogre Power gives
// (Pool 0x26; 0: none)
void set_worn_facts(uint8_t effect_data, uint8_t strength_fx);
// Spiritual Hammer (spell_facts.md 0x1C): while they have its effect, the
// hammer is in their items, readied (a new one when it's gone and there's
// room: +1, "Gains an item" - true); without it, the hammer goes
bool keep_hammer(party::Character& c, const items::Names& names, const ItemFacts& f);
// Halve: a pile of n becomes n - n/2 and a new pile of n/2 (not readied);
// false when it can't (one of it, or 16 items already)
bool halve(party::Character& c, int i);
// Join: the same items in other piles go onto pile i (255 a pile at most);
// how many piles were joined in
int join(party::Character& c, int i, bool secret = false);
// Auto Join (CYD_AUTO_JOIN): the item added onto a pile of c's that Join would make one of it with -
// true when it went there whole (nothing new in the item list)
bool pile_onto(party::Character& c, const uint8_t* item, bool secret = false);
// Sell (a shop): what the shop gives, in gold - half the value; piles of
// arrows / quarrels count each one, other piles count / 20 of each
int sell_value(const uint8_t* item, const ItemFacts& f);

// ---- The temple (Curse; costs and effect ids from the game's code, via the profile)
enum Cure : uint8_t {
    kCureBlindness, kCureDisease, kCureLight, kCureSerious, kCureCritical, kHealCure, kNeutralizePoison, kRaiseDead,
    kRemoveCurse, kStoneToFlesh, kCures
};
struct CureFacts {
    uint16_t cost[kCures];            // gold
    uint8_t  blinded, disease[8], feeblemind, poisoned, slow_poison, poison_damage, animate_dead, curse;
    // Cause Disease's course (combat.h SpellFx): the disease, weakness, sickness; helpless
    uint8_t  ill, weak, sick, helpless;
    // Secret / Darkness / Treasures: their poison damage effect, run out,
    // renews this one instead of itself (0x1A, the dwarves' to-hit effect - the
    // originals' mistake: one point of damage, then none; GAME.OVR 0x112AE /
    // 0x1AE7B / 0x2096F); 0 = itself. rules::fixes(): itself (SPEC 12)
    uint8_t  poison_renew;
    // The temple's Raise Dead (rules::apply_cure, the generic temple): which
    // original's routine - 0 none, 1 Curse / Champions (GAME.OVR 0x51AA /
    // 0x5880: a point of Constitution IN USE only, a cut of the maximum hit
    // points at Con 14-16, at 17+ a fighter's or one past its hit dice),
    // 2 PoolRad (0x4B22: its Constitution itself; at 17+ fighters only),
    // 3 Gateway (0x607E: as Curse, at 17+ a fighter or a former fighter)
    uint8_t  temple_raise;
};
// Removes a character's effects of a type; how many
int remove_affects(party::Character& c, uint8_t type);
// Slow Poison's clock as `minutes` pass outside fights (spell_facts.md
// 0x1A): a hit point every 10 minutes (down to 1); when Slow Poison runs
// out while they're still poisoned, they die ("dies from poison": true).
// Call before the effects' minutes are taken (magic::tick_affects).
bool poison_clock(party::Character& c, int minutes, const CureFacts& f);
// Cause Disease's course as `minutes` pass outside fights (coab's facts,
// listing ovr013:1038 / 10C2): the disease running out starts weakness (60
// minutes) and sickness (10), each renewing itself until cured - weakness:
// Strength in use - 1 ("is weakened"), at 3 helpless; sickness: a hit point
// while over 1, else helpless. What happened (the last of each); call before
// magic::tick_affects, as poison_clock.
struct DiseaseStep {
    int  weakened = 0, hurt = 0;        // how many times
    bool helpless = false;
};
DiseaseStep disease_clock(party::Character& c, int minutes, const CureFacts& f);
// Constitution 20 and up (its effect, given by stats()): a hit point back
// each time 60 minutes run out (curse_finish_facts.md 5.4). How many came
// back ("is fully healed" / "is partially healed"). Call before
// magic::tick_affects, as poison_clock.
int con_regen(party::Character& c, int minutes, const ItemFacts& f);
// Pool's mummy rot (poolrad_combat_facts.md 4) as `minutes` pass outside
// fights: each time the rot's 30 days run out "Rots..." - Charisma 2 less
// (3 at least) and a month less to go (its data's high nibble); with none
// left "Dies rots away" (gone). The disease's 10-day clock starts again
// each time it runs out. Call before magic::tick_affects.
struct RotStep {
    int  rots = 0;
    bool dies = false;
};
RotStep rot_clock(party::Character& c, int minutes, uint8_t rot, uint8_t clock, int shift = 4);
// An effect whose end kills (the Dark Queen's Gas Spore disease 0xB9 -
// DQK.EXE 0x51CA5 "dies from disease"): true when it ran out in these minutes
bool death_clock(party::Character& c, int minutes, uint8_t type);
// Pool's Manual of Bodily Health (poolrad_combat_facts.md 3.2, handlers
// 0x04 / 0x07) as `minutes` pass: the reading running out "starts to train"
// - 30 days of training (its effect); that running out "is hardier":
// Constitution + 1; at 20 and up the healing every hour (f.con_regen_fx),
// else the original's hit points (ovr13+0233: the hit points over the
// rolled ones shared out by the levels - a fighter's (Con - 15) a level,
// others' 2 over 15, 1 at 15 -, added to the most and the current; none at
// 17 and up for those without fighter levels). Call before
// magic::tick_affects, as poison_clock.
struct ManualStep {
    bool trains = false, hardier = false;
};
ManualStep manual_clock(party::Character& c, int minutes, uint8_t reading, uint8_t training, const ItemFacts& f);
// Whether the cure does anything for them (else the temple asks "cast cure
// anyway?"; wounds always do)
bool needs_cure(const party::Character& c, Cure cure, const CureFacts& f);
// The cure's work (paid for): wounds healed by the dice (light 1d8,
// serious 2d8+1, critical 3d8+3, Heal all but 1d4 and the blindness,
// diseases, feeblemind), effects removed, the dead raised (1 HP), a curse
// lifted (the effect, else the first cursed item comes off), stone to flesh
void apply_cure(party::Character& c, Cure cure, const CureFacts& f, create::Dice& d);
// Hit points back (up to the most), for the living, unconscious or dying;
// outside a fight the dying come round and the unconscious wake (able to
// fight again). False if they can't be healed (dead, stoned ...).
bool heal(party::Character& c, int amount);
// An effect whose holders heal by none of rules::heal (the Dark Queen's 0xD7,
// its heal routine's flag 0 - DQK.EXE 0x48F:0x25F7); 0 none. Set at a game's start
void set_no_heal(uint8_t type);

// ---- Appraising gems and jewellery (the games' tables; gold). Secret's
// are Curse's halved (div 2: its gems 5 ... 2500, its jewels' ranges and
// bases halved - secret_facts_2.md 17)
int gem_value(int d100, int div = 1);
int jewel_value(int d100, create::Dice& d, int div = 1, int span41 = 0);   // span41: 41-50's random span (0: 2500 / div)

// A shop's price for an item: its value moved by the shop's price factor
// (script word 0x7F6D: 1 = 1/16 ... 0x10 normal ... 0x80 = x8)
int price(const uint8_t* item, int factor);

// The originals' rule mistakes fixed (Tom's Game Fixes - Item Fixes until
// v0.139.0): where a game's program never applies a rule AD&D has (e.g.
// Secret's phase lists leaving an effect out), the engine applies it while
// that fix's switch is on and does as the original when it is off. Since
// v0.209.0 (Tom, 2026-11-17: "each a switch") every fix is its own switch
// (rules::Fix); each place says which and is listed in SPEC section 12.
enum Fix : uint8_t {
    // Every game
    kFixSaves,          // Saving Throws: no wrap below 0, no save worse for a level gained
    kFixThief,          // Thief Skills: no wrap below 0%
    kFixHeld,           // Held Mid-Step: held / stunned by a free attack, no step
    kFixBolt,           // Bounced Bolt Length: exactly its range
    kFixHpLevel,        // Hit Points a Level: at least 1, a Constitution penalty counted signed (Secret-, Krynn-type, Queen)
    kFixHarm,           // Harm: never wraps the hit points round (the games with Harm: Secret, Darkness, Treasures, Knights, Queen)
    // PoolRad (Curse too where said)
    kFixPRace,          // Race Checks: the scripts see the race (Podol Plaza, the Temple of Bane)
    kFixPSpell,         // Who Has It Memorized: the script's spell search
    kFixPTrain56,       // Training New Spells: spell 56's slots
    kFixPShare,         // NPCs' Coin Share: not cut to 255
    kFixPLoad,          // Loaded Game's Area: a load keeps the block's state
    kFixPCharm,         // Charm After a Fight: ends with it; NPCs' 255 lost levels none
    kFixPClock,         // Script Clock: the script's CLOCK moves the clock
    kFixHealDead,       // Healing the Dead: the computer cures its own side's bodies only (Curse too)
    // Curse
    kFixCFormer,        // Former Clerics: turn undead by their cleric level
    // Secret (Darkness / Treasures / Knights where said)
    kFixSBark,          // Barkskin: its own effect, no Heal
    kFixPoison,         // Poison: a point every 10 minutes (Secret, Darkness, Treasures)
    kFixDiff,           // Difficulty To-Hit (Secret, Knights)
    kFixSOwn,           // Secret's Own List: elves' +1, dwarves' / gnomes' -4, Con saves, blink, displacement, periapt, 18/xx
    kFixPalCast,        // Paladins' Casting: by the paladin's level (Secret, Darkness, Treasures)
    // Darkness (Treasures / Krynn where said)
    kFixRaceAdj,        // Race Adjustments at creation (Darkness, Champions, Knights)
    kFixClericXp,       // Cleric's Experience Bonus from Wisdom 16 (Secret, Darkness, Treasures)
    kFixRemCurse,       // Remove Curse takes off the curse (Darkness, Treasures, Knights, Queen)
    kFixPwBlind,        // Power Word Blind: its time, not for good (Darkness, Treasures)
    kFixDragonFear,     // Dragon Fear ends with the fight (Darkness, Treasures)
    kFixBulette,        // Bulette's Leap: one round in four (Darkness, Treasures)
    kFixSummon,         // Monster Summoning: room for big monsters
    kFixVault,          // New Game's Vault: emptied (Darkness, Treasures, Gateway)
    // Treasures
    kFixTBoots,         // Jumping Boots: land on any free square in reach
    // Gateway
    kFixGHolds,         // Stirge and Squid: their holds hurt
    kFixGSounds,        // Missing Sounds: magic hit and sling, not the miss
    // Champions (Knights / Queen where said)
    kFixThiefRace,      // Thief Skills by Race (Champions, Knights)
    kFixKSaves,         // Knights' Saves: the right row
    kFixBozak,          // Bozak Stun ends with the fight
    kFixPalHp,          // Paladins' Hit Points (Champions, Knights)
    kFixStench,         // The Stench hurts the victims (Champions, Knights, Queen)
    // Death Knights
    kFixKFeeble,        // Feeblemind: the spell's own effect, not a drain
    kFixKHeal,          // Heal Cures Feeblemind
    // The Dark Queen
    kFixQAnemone,       // Anemone squirts its target
    kFixQDamage,        // DAMAGE Saves: a script's damage allows its save
    kFixQHooks,         // To-Hit Hooks: Invisible to Animals and the like
    kFixQItemDmg,       // Dawnshine's Damage: the item effect's damage counts
    kFixQSaves18,       // Saves Past 18th Level keep improving
    kFixQWis13,         // Wisdom 13: the multi-class cleric rule fires
    // (added at the end: the settings keep a bit a fix)
    kFixTrainAll,       // Training Every Class: a multi-class trains each ready class, no experience another class needs lost (Curse, Gateway)
    kFixHp255,          // Hit Points Past 255: training's maximum stops at 255 (Darkness, Treasures)
    kFixImportHp,       // Imported Hit Points: a PoolRad character's Constitution hit points survive a Constitution change (Curse)
    kFixPoisonSave,     // Poison Save Bonus: the Constitution bonus lowers the poison number (the originals raise it; all but PoolRad)
    kFixCount
};
static_assert(kFixCount <= 64, "the fixes' bits");
// Every fix on (true) or off - the tests' and As Original's way; the
// Settings' own choice: set_fixes_off
void set_fixes(bool on);
// The fixes switched off (bit k = Fix k); the rest on
void set_fixes_off(uint64_t off);
uint64_t fixes_off();
bool fix(Fix f);
// What the Settings show of a fix: its key's words, the games it touches
// (bit = games::Game), what it does
struct FixInfo {
    Fix         id;
    const char* name;
    uint16_t    games;
    const char* help;
};
const FixInfo& fix_info(int k);         // k < kFixCount, in the order above
// The AD&D 1E Rules (Tom, 2026-11-17): rules of the 1st edition books the
// originals left out - not mistakes in their code, so not Game Fixes. Each
// rule its own switch (a bit; all on by default); off: as the original.
enum Adnd : uint16_t {
    kAdndThrown   = 1,      // Thrown Weapons: a hurled weapon takes Dexterity's missile bonus to hit and
                            // no Strength (DMG p.64); a weapon both melee and thrown (dagger, hand axe,
                            // spear) Strength in melee, Dexterity to hit and no Strength when thrown
    kAdndRaise    = 2,      // Raising the Dead: every raising a point of Constitution and nothing more (PHB p.12)
    kAdndMissile  = 4,      // Magic Missile: (level + 1) / 2 missiles (PoolRad's own: level / 2)
    kAdndMultiHp  = 8,      // Multi-Class Hit Points: each class's, added up and divided by the classes
    kAdndItems    = 0x10,   // Magic Items: PoolRad's Flame Tongue as AD&D's
                            // (0x20: Coin Exchange until v0.209.1 - an Enhancement since, rules::kEnhCoins)
    kAdndAge      = 0x40,   // Age with Time: a year older for each year of game time
    kAdndPwk      = 0x80,   // Power Word Kill: by hit points, 120 in all
    kAdndProtEvil = 0x100,  // Protection from Evil: -2 to hit for evil attackers; the 10' radius wards those near
    kAdndDrain    = 0x200,  // Level Drain & Restoration: DMG p.119 / PHB p.53 (create::adnd_drain / adnd_restore)
    kAdndAll      = 0x3DF,
};
constexpr int kAdndCount = 9;
void set_adnd(uint16_t bits);
uint16_t adnd_bits();
bool adnd(uint16_t rule);
struct AdndInfo {
    uint16_t    bit;
    const char* name;
    uint16_t    games;                  // (bit = games::Game; the games it changes)
    const char* help;
};
const AdndInfo& adnd_info(int k);       // k < kAdndCount
// The Enhancements the engine itself applies (Tom, 2026-11-17: additions to
// the games' play, not rules - each a switch of the Enhancements page; the
// front end's own enhancements are play::Options)
enum Enh : uint8_t {
    kEnhCoins = 1,          // Coin Exchange: any coins pay, the change, the pooled and the shared money in the
                            // highest coins (the five-coin games)
    kEnhOil   = 2,          // Flasks of Oil: a thrown flask burns (fire damage - ordinary, not magic)
    kEnhAll   = 3,
};
void set_enh(uint8_t bits);
bool enh(uint8_t e);
// Every Game Fix, every AD&D 1E Rule and the engine's Enhancements on, or
// all off (as the originals - the tests' way)
void set_all(bool on);
// No fix switched off (the tests' look at set_all's state)
bool all_fixes();
#ifdef CYD_TEST_HOOKS
// The toggle checks (sweep's TOGGLEMAP / FIXOFF / ADNDOFF / ENHOFF): which
// switches were asked since the masks were last cleared, and switches held off
// whatever set_* says
namespace test {
extern uint64_t seen_fix, off_fix;
extern uint16_t seen_adnd, off_adnd;
extern uint8_t seen_enh, off_enh;
}
#endif
// A weapon both melee and thrown, thrown (Thrown Weapons): the record's
// to-hit holds the melee figure (Strength's), this is the change for a throw
// - Dexterity's missile bonus less Strength's to-hit
int thrown_hit_shift(int dex, int uses_strength, int str_full, int str00);
// The record's lost levels (0xE7, the hit points they took 0xE8). PoolRad's
// NPCs come from monster records with 0xFF there (unused for monsters - its
// MON1CHA / MON3CHA's Dirten, Tom 2026-11-11): the original counts 255
// levels lost (Restoration and training count it down, a drain wraps it);
// with Game Fixes a byte of 0x80 or more is none.
// (`lost` = the byte; a drain clears such a byte first with the fixes)
int lost_levels(uint8_t lost);

} // namespace rules
