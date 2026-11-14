Music renderer checks (2026-11-14; facts: the Project's claude/music_facts.md). Nothing of the games here:
the drivers and songs are read from the player's files in scratch.

mmlog.py   the player's MicroMagic driver (MUSIC.DAX block 0) run in Unicorn -> OPL2 write log (the oracle)
mm_log.cpp the engine's music::render_micromagic (x86.cpp) -> the same log form
           g++ -std=gnu++2b -O2 -Isrc -Itools/host tools/devkit/music/mm_log.cpp src/engine/music.cpp
               src/engine/x86.cpp src/engine/dax.cpp -o /tmp/claude-0/mus/mm_log
mm_wav.cpp the whole renderer (driver -> ymfm OPL2 -> 8-bit 11,025 Hz WAV, trailing silence cut); add
           src/engine/opl.cpp src/engine/opl_pcm.cpp to the line above. WAVs stay in scratch.
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
