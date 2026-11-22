VACE movies (Pools of Darkness's ending; the Project's claude/vace_facts.md), 2026-11-22.

vace_run.cpp   the engine's player (src/engine/vace.*) on a simulated clock, the player's FINAL.ACF / FINAL.BMA:
               g++ -std=c++17 -O2 -Isrc tools/devkit/vace/vace_run.cpp src/engine/vace.cpp src/engine/dax.cpp \
                   -o /tmp/claude-0/ag7/bin/vace_run
               vace_run <ACF> <BMA> <out dir> [ega] [nosound] [every]  -> NNNNN.ppm + "frame N at T ms"
vace_cmp.py    <dosbox frames> <dosbox log> <engine frames> <engine log> [every [v]]: the same pictures and their
               times (every: each DOSBox capture looked for among the engine's states within 1.5 s)

The original's recording (scratch; DOSBox autodos - claude/devkit_dosbox_build.md + _autodos_patch.md):
  a copy of the game folder (dk/) plus VM13.COM (b8 13 00 cd 10 c3: mode 13h; VMD.COM b8 0d 00 ... for EGA -
  CONTROL.EXE sets the mode before VACE, which won't run without it), dos.conf.in with nosound=false,
  sbtype=sb16, oplmode=opl2, @RUN@ = "snd / vm13 / vace 4 FINAL 1 1" (EGA: vmd, vace 2), a script
  "wait 400000 / quit"; AUTODOS_FRAMES=1 AUTODOS_DACHASH=1 (fades) [AUTODOS_PALLOG=1].
Results 2026-11-22: VGA 181 of 185 captures within 70 ms; EGA frames 47 of 47 (the original 280 ms late from
its first chunk's conversion), the dissolve a prefix of the register's order.

The playsim: GAME=4 SAVE=<a save in block 65> AREA=1 BLOCK=75 SETVAR=254=255,1D0=255 RUNAT=9666 (CANVAS8=1 VGA=1
for 256 colours; CACHE=... MAKEMUSIC=1 for the song; MOVIEKEY=ms a tap) - shots "movie" a second, then the script's
"AS IF FROM FAR AWAY ...".
