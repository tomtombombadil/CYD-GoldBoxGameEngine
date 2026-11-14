Music renderer checks (2026-11-14; facts: the Project's claude/music_facts.md). Nothing of the games here:
the drivers and songs are read from the player's files in scratch.

mmlog.py   the player's MicroMagic driver (MUSIC.DAX block 0) run in Unicorn -> OPL2 write log (the oracle)
mm_log.cpp the engine's music::render_micromagic (x86.cpp) -> the same log form
           g++ -std=gnu++2b -O2 -Isrc -Itools/host tools/devkit/music/mm_log.cpp src/engine/music.cpp
               src/engine/x86.cpp src/engine/dax.cpp -o /tmp/claude-0/mus/mm_log
mm_wav.cpp the whole renderer (driver -> ymfm OPL2 -> 8-bit 11,025 Hz WAV, trailing silence cut); add
           src/engine/opl.cpp src/engine/opl_pcm.cpp to the line above. WAVs stay in scratch.
ail_log.py the Dark Queen's Miles AIL 2 driver (ADLIB.ADV + INSTR.AD + ADDQn.XMI) run in Unicorn through
           the AIL API -> "tick reg val" lines (the oracle for music::render_ail)
ail_wav.cpp music::render_ail end to end (-> WAV; "log" = ail_log.py's lines)
ealog.py   Champions' Electronic Arts driver (MIDI.DAX block 3) + SOUND.DAX block 1 in Unicorn, ticked as
           START.EXE's sound unit does (PIT 0x5555) -> "T n" / "reg val" (the oracle for music::render_ea)
ea_wav.cpp music::render_ea end to end (-> WAV; "log" = ealog.py's lines)
           (both: g++ -std=gnu++2b -O2 -Isrc -Itools/host <tool> src/engine/music.cpp src/engine/x86.cpp
            src/engine/opl.cpp src/engine/opl_pcm.cpp src/engine/dax.cpp)
dro.py / cmpdro.py   DOSBox's raw OPL capture (.dro) against a log: notes, channels, pitch, timing and
           every register the capture saw written, at each note-on.

DOSBox (the original): dosbox with [sblaster] oplmode=opl2, a mapperfile binding hand_caprawopl to a key
(`hand_caprawopl "key 290"` = F9; ctrl+alt+F7 doesn't reach it through Xvfb) and [dosbox] captures=<dir>;
xdotool windowfocus + key F9 to start / stop. Darkness: autoexec "snd" then "game" (skips VACE's intro)
or "control" (the intro, which plays P1.SMS / P3.SMS = MUSIC.DAX blocks 1 / 3 with the same driver).

Results (2026-11-14):
- mm_log vs mmlog.py: every write and its tick identical for all 17 songs (Darkness 1-8, Death Knights
  1-7, Treasures 1, Gateway 1).
- mmlog.py vs DOSBox: Darkness song 1 in the intro - 986 notes identical (channel, pitch), timing within
  1 ms over 93.6 s, every register the capture saw written identical at every note-on; in the game's
  title 271 notes identical (then the credits restart it), timing within 4 ms.
- ail_wav vs ail_log.py: every write and tick identical for the Dark Queen's 8 songs. ail_log.py vs
  DOSBox ("dqk SB220 SI7" = AdLib music, the title): 1828 notes identical, every register; timing within
  142 ms over 62 s (an ail_wav log turned into "T n" lines at HZ 120 for cmpdro.py).
- ea_wav vs ealog.py: identical (Champions' title, 144 s). ealog.py vs DOSBox (KRYNN.CFG's AdLib, START):
  626 notes identical, timing within 2 ms, then the title's demo stops the music.
