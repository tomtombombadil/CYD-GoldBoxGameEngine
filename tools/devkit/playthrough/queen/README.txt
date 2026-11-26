The Dark Queen of Krynn play-through kit (2026-11-25): the Death Knights kit (../knights) moved over -
play folder /tmp/claude-0/qplay (card/The Dark Queen of Krynn = the GOG copy, card/KNIGHTS = the Death
Knights winners' save, slot C, for "Load from where? Death Knights" - POOLDIR=/GOLDBOX/KNIGHTS/SAVE),
playsim GAME=7, saves SAVE/SAVGAMx.QSV (+ VAULTx.DAT). No game bytes here.

Changed from the Knights kit:
- p.sh: GAME 7, the folder name. bk.sh: the QSV and VAULT files.
- adv.sh: also saves at the Queen's exploring menu ("Area Cast View Encamp Search Look Inv").
- events.sh / evsq.py: the Queen's square dispatch ("op 2F: 1:002D 0:007F 1:00BF" then "op 25: 1:00BF 0:N");
  maps up to 17 x 17 (any width).
- walkans.sh: MAPLOCKED / MAPSECRET only with WLOCKED=1 / WSECRET=1 - with them on a walk went through
  Caergoth's warehouse door 3 (7,13) and the engine couldn't leave it. MAPAVOID="7,13" keeps it out.
- place3.sh: the slot from SLOT (default B).

Lessons (Caergoth):
- The dragons' fight (two blue, one green) wipes a Knights-level party on most seeds: SEED=19 won with no
  deaths (17 lost one). Auto fights that lose stop at the party menu (screen 1).
- Daenor's ship 8,14: Explain, then Yes to join - NOBASE="BUY\|DRINK" so the JOIN question is answered Yes.
- The temple list has no Resurrection: dead elves can't be raised there.
- To check a save in the original: tools/devkit/quickmagic/dosrun.py with OVERLAY=<qsv>=SAVE/SAVGAMD.QSV and a
  script Enter, P, L, T, D (load D from the party menu); the party list must show.
