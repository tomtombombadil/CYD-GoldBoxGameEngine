// Engine comforts beyond the original games - each one a switch, so a port
// to another platform (bigger screens, PortMaster ...) can drop it with a
// build flag, e.g. -DCYD_BIG_ICON_PREVIEW=0 in its build_flags.
#pragma once

// The icon editor's big preview (Tom, 2026-10-10, v0.59.0): the combat
// icons are 24 x 24 pixels, too small to judge on these 2.8" - 4.0"
// screens, so the editor (Alter -> Icon, Create New Character) also shows
// the NEW icon as large as fits - on 480x320 always, in the Companion
// strip; on 320x240 when Settings' Large Icon is on, in
// the editor's empty right side. A tap on it flips ready / action.
// Where: viewer.cpp (big_icon_* / draw_big_icon, the Settings key),
// play.cpp (play::icon_preview), settings.h (large_icons).
// Turn it off for screens where the game shows at a readable size.
#ifndef CYD_BIG_ICON_PREVIEW
#define CYD_BIG_ICON_PREVIEW 1
#endif

// The demo stops on a tap (Tom, 2026-10-10, v0.64.0): the title's Demo
// runs the game's own demo; in the original a key doesn't stop it (it runs
// until the demo party wins or falls). Here any tap or key ends it and the
// title comes back. 0: as the original (input is ignored while it runs).
// Where: play.cpp (demo_input / stop_demo).
#ifndef CYD_DEMO_TAP_STOPS
#define CYD_DEMO_TAP_STOPS 1
#endif

// The icon gallery (Tom, 2026-10-11, v0.71.0): the icon editor (Alter ->
// Icon, Create New Character) opens on pages of ready-made icons to tap -
// whole icons, then every head, every body / weapon and colour schemes,
// each shown on the character - instead of stepping through values one at
// a time with Next / Prev. Edit goes on to the original editor (its Exit
// comes back here); Done asks the original's "Is this icon ok?" (No: back
// here). The pictures are the player's own CHEAD / CBODY parts; the schemes
// and combinations are the engine's (engine/icon_looks.*).
// v0.74.0 (Tom): with the player's Gold Box Companion on the card
// (/GOLDBOX/GBC, its GOG release's DAX-Icons-EGA), its icon pairs follow as
// more pages; a pick is kept by the character's name (_CYD/<folder>/
// ICONS.TXT) and drawn in fights instead of the head / body icon. Never
// shipped: the Companion is its author's work (engine/gbc_icons.*).
// Where: play.cpp (gal_*), viewer.cpp (no 320x240 big preview over it).
// 0: the original editor only.
#ifndef CYD_ICON_GALLERY
#define CYD_ICON_GALLERY 1
#endif

// More characters to add (Tom, v0.56.0 / v0.58.0): Add Character also lists
// the members of saved games ("NAME from saved game A") and, for Pool of
// Radiance, the player's Pool of Radiance folder on the card; the original
// lists only its save folder's files. Where: play.cpp (find_guys).
#ifndef CYD_MORE_CHARACTERS
#define CYD_MORE_CHARACTERS 1
#endif

// Journal entries shown (Tom, 2026-10-10): when the game says "read entry
// N", the next tap opens the entry from the player's own journal (the
// original only prints the number). Where: viewer.cpp (play_journal).
#ifndef CYD_JOURNAL_POPUP
#define CYD_JOURNAL_POPUP 1
#endif

// The Clear font (Tom, 2026-10-11, v0.73.0): the engine's own upright, bold
// letters with real lower case (tools/fonts/clear8.txt, drawn for the
// engine) in place of the game's narrow slanted capitals - much easier to
// read on 2.8" - 4.0" screens. The game's symbols and frame pieces stay.
// Off unless the player picks it (Tom); switchable mid-game (Game menu ->
// Options), the text on screen changing with it.
// v0.74.0 (Tom): the switch is the Game Font key - Original / Clear /
// Darkness, the last the player's own Pools of Darkness font (8X8D0.DAX in
// their Pools of Darkness folder; offered only when it's on the card).
// Where: engine/font.* (set_letters), play.cpp (font_wanted), viewer.cpp,
// settings.h (game_font).
#ifndef CYD_CLEAR_FONT
#define CYD_CLEAR_FONT 1
#endif

// v0.84.0 (Tom): Clear Caps, a fourth choice - the Clear font's capitals,
// digits and punctuation without its lower case (all capitals, as the
// games print).

// The icon portrait (Tom, 2026-10-11, v0.84.0): the games have no
// character portraits, but View Character leaves room for one (Pool of
// Radiance draws its head / body portrait there, top right). The engine
// shows the character's combat icon three times its size in that space,
// inside a frame of the game's own tiles (the 3D view's frame; Pool's
// box). Where the game has a portrait (Pool: every character, NPCs too)
// the portrait comes first and a tap on it swaps to the icon and back;
// otherwise a tap flips the icon's ready / action pose. 0: as the
// originals (Pool's portrait only; nothing in Curse's space).
// Where: play.cpp (view_box), engine/layout.* (picture_frame).
#ifndef CYD_ICON_PORTRAIT
#define CYD_ICON_PORTRAIT 1
#endif

// The originals' item mistakes fixed (Tom, 2026-10-12, v0.90.0): Pool of
// Radiance's "Long Sword +2 Flame Tongue" has no flame tongue in its record
// (0x3D 0x16 - its Use casts Find Traps - and no effect code), so it is a
// plain +2 sword whose blows aren't fire: a troll it fells gets back up.
// Fixed it is the AD&D Flame Tongue, as Curse's: a Long Sword +1 Flame
// Tongue, +2 against regenerating creatures, +3 against cold-using / avian
// ones, +4 against undead, its blows fire. The files keep the original's
// bytes (the fix is made as characters are read, items taken, the switch
// turned). 0: as the original. Where: engine/items.* (fix_item),
// rules.cpp (add_item, fix_items), play.cpp (character_read, set_options,
// write_character_files), profile.cpp (Pool's items.flame).
// The Settings key is "Game Fixes" (Tom, 2026-10-23; "Item Fixes" until
// v0.139.0): every fix of an original game's mistake - items and rules -
// with the originals' behaviour when off.
#ifndef CYD_ITEM_FIXES
#define CYD_ITEM_FIXES 1
#endif

// The menu highlight rule (Tom, 2026-10-24: "My preferences about the
// highlight should be on ALL games"): one rule for every game's menu line -
// (1) the highlight is a white box round the whole word (Treasures draws
// white letters, its words' first letters white anyway, so one-letter
// choices couldn't be told apart; its lists too); (2) a menu starts on its
// first word (Darkness / Treasures carried the last menu's place into the
// next - past a short menu's end that was its last word); questions keep
// the word their game starts them on (No; Darkness's / Gateway's Yes);
// (3) after a pick the highlight stays there: the same menu or question
// shown again (a reroll, a menu back after the one it opened) starts on
// the word picked last. Built in (no Settings key: it changes no play).
// 0: as the originals. Where: play.cpp (hl_apply, hl_remember, hl_plain,
// hl_inherit).
#ifndef CYD_MENU_HIGHLIGHT
#define CYD_MENU_HIGHLIGHT 1
#endif

// Spells chosen to memorize stay chosen when the camp breaks before they
// are learnt (Tom, 2026-10-31: a rest interrupted by the watch or monsters
// left them to be chosen again) - the next camp's Memorize asks "Memorize
// these spells?" with them, its Rest learns them (its time worked out
// again). The originals forget them as the camp breaks. Built in (no
// Settings key: it changes no play). 0: as the originals. Where: play.cpp
// leave_camp.
#ifndef CYD_KEEP_MEMORIZING
#define CYD_KEEP_MEMORIZING 1
#endif

// The rolls shown in fights (Tom, 2026-10-31: "In combat there is room
// below the display of hits and misses to show the rolls and modifiers that
// were just rolled ... It will also help in troubleshooting combat issues"):
// under each message, at the foot of the fight's panel (its rows 19-21 -
// fewer when the message's own lines reach down; never over them), in light
// grey, the dice that message tells of as they came up, the modifiers'
// total and the result: "To Hit: 17+2=19" and "Needed: 14" (a weapon's
// blow: the modifiers counted from THAC0 20 - 20 - THAC0 plus the fight's
// own, the total needed 20 - the target's AC as it stood), "Damage: 4+3=7"
// when it hit (the dice, then all else to the points done); a miss shows
// the last d20 missed; a spell: its target's save ("Save: 12+1=13",
// "Needed: 14"; "immune") and its damage as rolled, before the target's
// defences and save (the message says what was taken), a touch's d20.
// Several blows in an attack: each hit's page its own. The engine writes
// the rolls down (combat::g_rolls) only while a fight runs with the switch
// on - nothing reads them back, so play is the same either way. On by
// default (ours, for the small screens and testing). 0: as the originals.
// Where: engine/combat.* (log_roll: attack, saving_throw, harm, cast's
// touches), play_fight.inc (RollShow, take_roll, page_roll, show_rolls),
// viewer.cpp (the Show Rolls key).
#ifndef CYD_COMBAT_ROLLS
#define CYD_COMBAT_ROLLS 1
#endif

// The Companion panel's views (Tom, 2026-11-01): on 480x320 a tap on the
// panel beside the game cycles Map -> Party -> (in a fight) Battle. Party:
// each member's combat icon, name, hit points now / most with a bar (green,
// yellow under half, red under a quarter) and status / effects in words.
// Battle: the whole 50 x 25 battlefield (the part the fight uses, as
// large as fits - 3 px a square for an outdoor field, more indoors): walls
// grey, the party and allies green, the enemies red (charmed ones switch),
// asleep / held / paralysed / stunned / fleeing / fallen blinking, the
// game screen's 7 x 7 window outlined, the turn's fighter ringed white,
// what is aimed at gold. A fight starts on Battle; the view the player had
// comes back after it. 320x240 (and the 5.0" board's 2x view): in a fight
// the Map key shows the Battle view over the game screen until a tap. The
// originals have no such panel. Drawn straight to the screen, no memory
// kept. 0: the map alone. Settings key "Companion Views" (settings.h
// comforts_off2). Where: game/companion.* (drawing), play.cpp (panel_*:
// what they show), viewer.cpp (comp_*: views, taps, blinking).
#ifndef CYD_COMPANION_VIEWS
#define CYD_COMPANION_VIEWS 1
#endif

// The camp's Fix in every game (Tom, 2026-11-01: "add the Fix menu option
// to the PoolRad encamp menu (and any other game that lacks it)"): Curse
// added Fix to the camp - the healers rest, memorize their cure spells
// again and heal the party, as long as it takes (spells::fix_plan) - and
// every later game kept it; Pool of Radiance's camp has none ("Save View
// Magic Rest Alter Exit"). With this on, Pool's camp is "Save View Magic
// Rest Alter Fix Exit" and its Fix is Curse's with Pool's own cure spells,
// rest and memorizing (an encounter breaks it off as it does a Rest).
// Every other game's own camp already has Fix. A game "enhancement" -
// whether these get a list of their own beside Game Fixes is Tom's to
// decide (docs/TODO.md); it sits with the comforts meanwhile. 0: as the
// original. Where: play.cpp (camp_words, fix_party), viewer.cpp (the Camp
// Fix key).
#ifndef CYD_CAMP_FIX
#define CYD_CAMP_FIX 1
#endif

// The Cheats tab (Tom, 2026-11-01): the in-game Game menu's Cheats tab -
// Healer (the game's own temple, every cure free), Merchant (the game's
// own shop with the goods of all its shops together, Buy / Sell / Id /
// Appraise as its shops have them, at the goods' normal prices, Id free;
// the shop's / temple's own picture in the view) and Sanctuary (the game's own camp - Rest, Magic, Fix, View, Alter,
// Save - where nothing interrupts a rest: no wandering monsters, no city
// watch, no camp scripts) and Trainer (Tom, 2026-11-06: the game's own
// training hall - its party menu - training every class the game has,
// free, otherwise as its training: one level a visit, the experience
// kept, race limits, spells, hit points, its messages; and Modify
// Character for any character there, any time, within the game's Modify
// bounds; Begin Adventuring goes back). Only while the party stands at the exploring
// menu; leaving one goes back there as it was. Hidden with the Settings
// key Cheats Tab off. 0: no Cheats tab. Where: play.cpp (cheat_*),
// engine/ecl.* (find_shops: the goods the game's scripts sell),
// viewer.cpp (the tab, the key).
#ifndef CYD_CHEATS
#define CYD_CHEATS 1
#endif

// Max Hit Points (Tom, 2026-11-06): a cheat the player opts into - the
// Settings key Max Hit Points (also on the Cheats tab), OFF by default.
// On: every level gained by training (the halls, the Cheats tab's
// Trainer) and a new character's levels (Tom, 2026-11-06: creation too)
// take each hit die at its highest (fighter d10 -> 10, cleric
// d8 -> 8, thief d6 -> 6, magic-user d4 -> 4; two dice at 1st level as
// the game rolls them), the rest of the game's sum unchanged (Constitution,
// the multi-class division, past the last hit die, Pool's 2/3 rule
// moot; with the Game Fixes a multi-class's per-class rule). The save files are written as the
// game would with those rolls. 0: no such switch. Where: engine/create.*
// (Dice::max), play.cpp (train_yes_no, the creation's rolls), viewer.cpp (the keys).
#ifndef CYD_MAX_HP
#define CYD_MAX_HP 1
#endif

// Each comfort compiled in can also be switched off by the player: the
// game's Settings page (Tom, 2026-10-11, v0.72.0; one set for all games,
// settings.h comforts_off), the one Settings screen (Home, a game's page, the
// in-game Game menu's Settings tab - v0.91.0).
