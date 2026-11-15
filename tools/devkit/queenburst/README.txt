The Dark Queen's area burst (2026-11-15, Tom: "What does the DOSBox version do? Test it and replicate it").
Facts: the Project's claude/queen_explode_facts.md. Engine: play_fight.inc explode_begin / explode_step (Fx::Explode,
a Fly's boom_*), checked by fx_check.inc F4 (rings 1..3 x area + 1, in order, whole) in fightfuzz FXCHECK=1.

DOSBox (dosbox-auto; scratch only, never the shots in the repo):
  python3 tools/devkit/queenburst/mkfight.py /tmp/x.scr
  AUTODOS_FRAMES=1 ROOT=1 python3 tools/devkit/quickmagic/dosrun.py qfx "<Queen GOG folder>" . dqk /tmp/x.scr
  - save A: Begin, the Palace (the story), Leave (to Caergoth), the walk in walk.txt (F up / L left / R right, Return
    after the waits) - "DRAGONS DESCEND AROUND YOU" at 6,9; Magic On + Quick: the party's mages cast (a Fireball in
    most runs - the original's dice are seeded from the clock, so runs differ). mkscr.py <out> keys... makes a key
    script with a shot after each key (W<ms> wait, S<name> shot). dosrun.py: NOPNG=1 no PNG copies, CONFIN=<conf.in>
    another DOSBox config (e.g. cycles=fixed 3000, GOG's "auto" for this real-mode game).
Playsim (the same fight): MS=20000 CHOICES=061 SAVE=A FIGHT=1 FORCESPELL=4:2F,... FXSHOTS=n with the walk's tokens
  (FFFFFFRLFFFFFFFFFLFLFFLFLFFFFFFFFRFFRLFRLFFFLFLRFFRFFFFFRFFLFFFFRFFFFF) and a _CYD/QUICK.TXT ticking Spells for
  ALARIC THE BOLD; "[fight] burst:" lines name each burst.
