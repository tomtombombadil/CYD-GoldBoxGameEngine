# The cluebook checker (a developer's tool - nothing of it is built into the engine)

Tom, 2026-11-27: "the Cluebooks contain maps! Accurate maps with legends and markers ... I would like to see our
maps improved". This reads the player's own cluebook PDFs (never in this repo) and compares their maps with the
engine's own reading of the game's GEO files - to put the engine's wall / door rules right.

- `geotxt.cpp` - the engine's maps as text (each square's sides: wall type, door code, passage) and each map's
  plain wall types. Build: `g++ -std=c++17 -O1 -Isrc -o geotxt tools/cluebook/geotxt.cpp src/engine/*.cpp`;
  run: `geotxt <the game's GEO*.DAX> > geo_<game>.txt`.
- `scalefind.py <cluebook.pdf> <geo.txt>` - the book's square size (pixels at 100 dpi): each of the game's maps
  drawn at sizes 15 - 30 and slid over the busiest pages.
- `findmaps.py <cluebook.pdf> <geo.txt> <sizes at 150 dpi> "" <out.json>` - every map's best place in the book:
  its walls correlated with the page's straight lines (FFT); score = the share of its walls on a drawn line less
  the share of its open sides on one. `summary.py <game>` lists them.
- `crops.py`, `kind2.py`, `exemplar.py`, `rulecheck.py` - reading each side (wall, door, secret door ...) at a
  found map; not reliable yet (see docs/TODO.md).

Needs Python 3 with numpy, scipy, Pillow, and poppler's pdftoppm / pdftotext / pdfinfo.

Found so far (the Steam cluebooks, 2026-11-27; a square's size in pixels at 100 dpi in brackets), score >= 0.5 -
most at 0.85 - 0.98, the next best map far lower: Curse 10 of 16 maps (20), Pool of Radiance 19 of 29 (20), Secret
10 of 17 (25 - its compressed 130 dpi copy reads well), Champions 13 of 15 (20), Death Knights 14 of 19 (25.5),
Gateway 20 of 30 (23.5), Treasures (24.5) still to finish, Pools of Darkness 2 of 32 (20 - each map drawn as four
8 x 8 quarters apart: quarters to be tried). The Dark Queen's maps (HLIB, GEO.GLB) not yet. The rest: wilderness
maps, maps drawn in pieces, parts of maps.

Reading the sides (2026-11-27): `readall.py <game> <pdf> 0.6` registers each found map closely (`refine.py`: the
engine's walls on the drawn lines, at 200 dpi) and reads each side (`sides.py`: the line found at the side's ends,
then the middle - a box breaks the line and has long straight edges, a hook or arch leaves it whole); `compare.py`
counts the engine's codes against them. The game's own pictures: `mapload.cpp` (each area script's map and wall
sets - ecl::find_map_load), `looks.cpp` (each map's 15 wall types' pictures, hashed: alike = the same look),
`wallpic.cpp` (a wall block's five pieces drawn head-on, PPM). The last two #include engine/view3d.cpp (its piece()
is file-local) - a developer's tool's licence, never the engine's way.
