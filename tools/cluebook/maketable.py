"""src/engine/cluebook_tables.cpp from the checker's results - positions only, nothing of the books' text or pictures:
each edition of a cluebook (its game, page count and page size), each map found in it (its GEO file and block, the
page, the straightened grid's two polynomials as fractions of the page picture, the box the area's name is printed
in, in points from the page's foot). The device reads the name's words and cuts the picture from the player's own PDF.

    maketable.py <out.cpp> <game id>:<geo name>:<pdf>[:min score] ...
    e.g. maketable.py src/engine/cluebook_tables.cpp ChampionsOfKrynn:Champions_of_Krynn:/tmp/cb/steam/Champions/Cluebook.pdf

Needs warp_<geo name>.json and read_<geo name>.json in the current folder, poppler's pdfinfo / pdftotext."""
import sys, json, re, subprocess
import numpy as np
from warp import ev

DPI = 200

# The names whose text layer the books' OCR misread (Tom, 2026-11-26: the journals' misreadings are put right by a
# table - the same here): the name as the book prints it. "" - none shown (the line read isn't the name).
NAME_FIX = {
    'POOLRAD': {'GEO3.DAX:0': 'Civilized Area, New Phlan', 'GEO2.DAX:9': 'Stojanow Gate', 'GEO3.DAX:14': 'Kovel Mansion',
                'GEO4.DAX:2': 'Cadorna Textile House', 'GEO4.DAX:21': 'Sokal Keep', 'GEO6.DAX:1': "Buccaneers' Base",
                'GEO6.DAX:28': 'Outpost of Zhentil Keep', 'GEO8.DAX:29': "Kuto's Well",
                'GEO8.DAX:32': "Kuto's Well Catacombs", 'GEO1.DAX:31': 'Wealthy Area'},
    'CURSE': {'GEO2.DAX:4': 'The Fire Knife Hideout', 'GEO3.DAX:16': '', 'GEO4.DAX:32': 'Zhentil Keep',
              'GEO6.DAX:64': 'The Burial Glen', 'GEO6.DAX:66': 'The Ruins of Myth Drannor',
              'GEO6.DAX:67': 'The Grand Ruined Temple'},
    'SECRET': {'GEO1.DAX:16': 'New Verdigris', 'GEO1.DAX:33': 'Well of Knowledge', 'GEO2.DAX:68': 'Drider Base',
               'GEO3.DAX:81': 'Frost Giant Village', 'GEO3.DAX:96': 'Castle Entrance',
               'GEO3.DAX:98': "Dreadlord's Sanctum"},
    'Champions_of_Krynn': {'GEO2.DAX:48': 'Gargath', 'GEO2.DAX:67': 'Tomb of Sir Dargaard', 'GEO3.DAX:99': 'Ogre Base'},
    'Death_Knights_of_Krynn': {'GEO2.DAX:49': 'Tunnels Beneath Tower', 'GEO2.DAX:65': 'Kuo-Toa Slave Ship',
                               'GEO2.DAX:66': 'Gnome Village (Quazle)', 'GEO2.DAX:67': '',
                               'GEO3.DAX:97': 'Dargaard Keep Second Floor', 'GEO3.DAX:98': ''},
    'Gateway_to_the_Savage_Frontier': {'GEO2.DAX:3': 'Nesme', 'GEO4.DAX:11': 'Gundarlun', 'GEO5.DAX:16': 'Llorkh',
                                       'GEO3.DAX:7': 'Port Llast & Gallant Prince',
                                       'GEO3.DAX:9': 'Host Tower of the Arcane'},
    'Treasures_of_the_Savage_Frontier': {'GEO1.DAX:19': 'Secomber', 'GEO1.DAX:21': "Smuggler's Dock (Near Waterdeep)",
                                         'GEO1.DAX:26': '', 'GEO1.DAX:27': 'Orlumbor', 'GEO1.DAX:28': 'Neverwinter',
                                         'GEO1.DAX:29': 'Port Llast', 'GEO1.DAX:31': 'Luskan',
                                         'GEO1.DAX:33': 'Fireshear', 'GEO1.DAX:34': '',
                                         'GEO1.DAX:35': 'Ice Peak: Stage 1 - Aurilssbaarg',
                                         'GEO1.DAX:36': "Ice Peak: Stage 2 - Bjorn's Hold"},
    'Pools_of_Darkness': {'GEO1.DAX:83': 'Ruined City'},
}


def page_info(pdf):
    """The page count and each page's size in points (they differ within a book: a cover, then spreads)."""
    t = subprocess.run(['pdfinfo', pdf], capture_output=True, text=True).stdout
    n = int(re.search(r'Pages:\s+(\d+)', t)[1])
    t = subprocess.run(['pdfinfo', '-f', '1', '-l', str(n), pdf], capture_output=True, text=True).stdout
    sizes = {int(a): (float(b), float(c)) for a, b, c in re.findall(r'Page\s+(\d+) size:\s+([\d.]+) x ([\d.]+)', t)}
    return n, sizes


def words(pdf, page):
    t = subprocess.run(['pdftotext', '-f', str(page), '-l', str(page), '-bbox', pdf, '-'], capture_output=True,
                       text=True).stdout
    out = []
    for m in re.finditer(r'xMin="([\d.]+)" yMin="([\d.]+)" xMax="([\d.]+)" yMax="([\d.]+)">([^<]*)</word>', t):
        out.append((float(m[1]), float(m[2]), float(m[3]), float(m[4]), m[5]))
    return out


def name_box(pdf, page, cx, cy):
    """The line just above the map's column numbers, starting near its left edge: (x0, x1, y0, y1) in points from
    the page's top - or None. The column numbers' row is the first line above the grid."""
    pts = 72.0 / DPI
    gx0, gy0 = float(ev(cx, 0, 0)) * pts, float(ev(cy, 0, 0)) * pts
    gx1 = float(ev(cx, 16, 0)) * pts
    sq = (gx1 - gx0) / 16
    cand = [w for w in words(pdf, page) if gy0 - 5 * sq < w[3] < gy0 - 0.2 * sq and gx0 - 1.5 * sq < w[0] < gx1]
    if not cand: return None
    rows = {}
    for w in cand: rows.setdefault(round((w[1] + w[3]) / 2 / (sq * 0.4)), []).append(w)
    lines = sorted(rows.values(), key=lambda r: -max(w[3] for w in r))
    for ln in lines:
        text = ' '.join(w[4] for w in sorted(ln))
        if re.fullmatch(r'[\d ]+', text): continue          # (the column numbers)
        if not re.search(r'[A-Za-z]{3}', text): continue
        return (min(w[0] for w in ln) - 2, max(w[2] for w in ln) + 2, min(w[1] for w in ln) - 2,
                max(w[3] for w in ln) + 2, text)
    return None


def main():
    out = sys.argv[1]
    books = []
    for spec in sys.argv[2:]:
        parts = spec.split(':')
        game, g, pdf = parts[0], parts[1], parts[2]
        lo = float(parts[3]) if len(parts) > 3 else 0.6
        n, sizes = page_info(pdf)
        rd = json.load(open(f'read_{g}.json'))
        wp = json.load(open(f'warp_{g}.json'))
        maps = []
        for name, w in sorted(wp.items(), key=lambda kv: (kv[1]['page'], kv[0])):
            if rd[name]['score'] < lo: continue
            f, b = name.split(':')
            num = int(re.search(r'(\d+)', f)[1]) if re.search(r'\d', f) else 0
            cx, cy = np.array(w['cx']), np.array(w['cy'])
            # (fractions of the page picture: the render was DPI dots an inch over the page's points)
            pw, ph = sizes[w['page']]
            fx = cx / (pw * DPI / 72.0)
            fy = cy / (ph * DPI / 72.0)
            nb = name_box(pdf, w['page'], cx, cy)
            if nb: nb = (nb[0], nb[1], ph - nb[3], ph - nb[2], nb[4])      # (y from the page's foot, as PDFs count)
            fix = NAME_FIX.get(g, {}).get(name)
            maps.append((num, int(b), w['page'], fx, fy, nb, name, fix))
        p0 = maps[0][2] if maps else 1
        books.append((game, n, sizes[p0][0] / sizes[p0][1], maps, pdf))
    L = ['// Generated by tools/cluebook/maketable.py from the checker\'s results on the cluebooks - positions only:',
         '// the pages, the maps\' grids (fractions of the page picture) and the boxes their names are printed in.',
         '// Nothing of the books\' text or pictures; those are read from the player\'s own PDF. Do not edit.',
         '#include "cluebook.h"', '', 'namespace cluebook {', 'namespace {']
    for bi, (game, n, asp, maps, pdf) in enumerate(books):
        L.append(f'const Map kMaps{bi}[] = {{')
        for num, b, page, fx, fy, nb, name, fix in maps:
            box = f'{{{nb[0]:.1f}f, {nb[1]:.1f}f, {nb[2]:.1f}f, {nb[3]:.1f}f}}' if nb else '{0, 0, 0, 0}'
            L.append(f'    {{{num}, {b}, {page}, {{' + ', '.join(f'{v:.6g}f' for v in fx) + '}, {' +
                     ', '.join(f'{v:.6g}f' for v in fy) + f'}}, {box}, ' +
                     ('nullptr' if fix is None else '"' + fix.replace('"', '\\"') + '"') + f'}},   // {name}')
        L.append('};')
    L.append('}  // namespace')
    L.append('')
    L.append('const Book kBooks[] = {')
    for bi, (game, n, asp, maps, pdf) in enumerate(books):
        L.append(f'    {{games::Game::{game}, {n}, {asp:.4f}f, kMaps{bi}, {len(maps)}}},')
    L.append('};')
    L.append(f'const int kBookCount = {len(books)};')
    L.append('')
    L.append('}  // namespace cluebook')
    open(out, 'w').write('\n'.join(L) + '\n')
    for game, n, asp, maps, pdf in books:
        print(game, n, len(maps), 'maps,', sum(1 for m in maps if m[5]), 'names:',
              '; '.join(m[5][4] for m in maps if m[5])[:300])


if __name__ == '__main__':
    main()
