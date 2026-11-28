"""The cluebook's numbered markers on a found map: each square's middle (inside its walls) on the straightened map
picture, its ink read as digits (tesseract, digits only). The numbers and their squares are positions only - the
words they stand for are read from the player's PDF on the device.

    markers.py <game> <pdf> [map ...]   -> markers_<game>.json {map: [[x, y, number], ...]}

Needs warp_<game>.json (warp.py) and tesseract."""
import sys, json, subprocess, tempfile, os
import numpy as np
from PIL import Image
from warp import ev

S = 48                      # a square's size in the straightened picture


def straighten(page_rgb, w, margin=0.5):
    """The map picture with its grid straight: S pixels a square, margin squares round it."""
    N = int((16 + 2 * margin) * S)
    oy, ox = np.mgrid[0:N, 0:N]
    u, v = (ox / S - margin).ravel(), (oy / S - margin).ravel()
    X = ev(np.array(w['cx']), u, v).reshape(N, N)
    Y = ev(np.array(w['cy']), u, v).reshape(N, N)
    H, W, _ = page_rgb.shape
    x0 = np.floor(X).astype(int).clip(0, W - 2)
    y0 = np.floor(Y).astype(int).clip(0, H - 2)
    fx = (X - x0)[..., None]
    fy = (Y - y0)[..., None]
    im = page_rgb
    return (im[y0, x0] * (1 - fx) * (1 - fy) + im[y0, x0 + 1] * fx * (1 - fy) +
            im[y0 + 1, x0] * (1 - fx) * fy + im[y0 + 1, x0 + 1] * fx * fy)


def ink_mask(rgb):
    """The printed ink: darker than the paper by a margin (the books are red or black ink on cream)."""
    g = rgb.mean(-1)
    paper = np.percentile(g, 90)
    return g < paper - 70


def read_digits(patch, pieces=1):
    """A grey patch of the picture read as a number (tesseract's raw line mode first - it reads lone thick digits
    the word modes miss), else None."""
    lo, hi = np.percentile(patch, 5), np.percentile(patch, 95)
    a = ((patch - lo) / max(hi - lo, 1) * 255).clip(0, 255).astype(np.uint8)
    a = np.pad(a, 10, constant_values=255)
    with tempfile.TemporaryDirectory() as td:
        p = os.path.join(td, 'c.png')
        Image.fromarray(a).resize((a.shape[1] * 4, a.shape[0] * 4), Image.LANCZOS).save(p)
        got = []
        for psm in ('13', '7', '8'):
            r = subprocess.run(['tesseract', p, '-', '--psm', psm, '-c', 'tessedit_char_whitelist=0123456789'],
                               capture_output=True, text=True)
            t = r.stdout.strip()
            if t.isdigit() and len(t) <= 3: got.append(t)
    # (as many digits as the pieces of ink, when a reading has them)
    for t in got:
        if len(t) == pieces: return int(t)
    return int(got[0]) if got and pieces == 1 else None


def pieces(st):
    """Digit-sized pieces of ink: (y0, y1, x0, x1, label) and the labelled ink."""
    from scipy import ndimage
    m = ink_mask(st)
    lab, n = ndimage.label(m)
    out = []
    for i, sl in enumerate(ndimage.find_objects(lab)):
        h = sl[0].stop - sl[0].start
        w = sl[1].stop - sl[1].start
        if not (0.2 * S <= h <= 0.55 * S and 0.02 * S <= w <= 0.45 * S): continue
        piece = lab[sl] == i + 1
        if piece.mean() > 0.75 and w > 0.2 * S: continue        # (a door's box)
        out.append((sl[0].start, sl[0].stop, sl[1].start, sl[1].stop, i + 1))
    return out, lab


def glyph(lab, p, size=(20, 14)):
    """A piece as a fixed-size picture for matching (its own ink only)."""
    y0, y1, x0, x1, i = p
    g = (lab[y0:y1, x0:x1] == i).astype(np.uint8) * 255
    return np.asarray(Image.fromarray(g).resize((size[1], size[0]), Image.BILINEAR)).astype(float) / 255


def group(ps):
    """Pieces side by side on one line: a number's digits."""
    groups = []
    for c in sorted(ps, key=lambda c: c[2]):
        for gq in groups:
            t = gq[-1]
            if abs((c[0] + c[1]) / 2 - (t[0] + t[1]) / 2) < 0.15 * S and 0 <= c[2] - t[3] < 0.15 * S:
                gq.append(c)
                break
        else:
            groups.append([c])
    return groups


def find_all(sts):
    """Every map of a book at once (sts: name -> straightened picture): the book's own digits learnt from the pieces
    tesseract reads alone with one voice, then every piece matched against them - the book's numbers are all in one
    hand, so a piece looks most like its own digit's mean."""
    data = {}
    votes = {}
    for name, st in sts.items():
        ps, lab = pieces(st)
        grey = st.min(-1)
        data[name] = (ps, lab, grey)
        for p in ps:
            y0, y1, x0, x1, i = p
            d = read_digits(grey[max(0, y0 - 2):y1 + 2, max(0, x0 - 2):x1 + 2], 1)
            if d is not None and 0 <= d <= 9: votes.setdefault(d, []).append(glyph(lab, p))
    tmpl = {d: np.mean(v, 0) for d, v in votes.items() if len(v) >= 3}
    res = {}
    for name, (ps, lab, grey) in data.items():
        out = []
        for gq in group(ps):
            digits = []
            for p in gq:
                g = glyph(lab, p)
                best, bd = None, 1e9
                for d, t in tmpl.items():
                    e = ((g - t) ** 2).mean()
                    if e < bd: best, bd = d, e
                if best is None or bd > 0.08: digits = None; break
                digits.append(str(best))
            if not digits: continue
            num = int(''.join(digits))
            if num == 0: continue
            y0 = min(c[0] for c in gq); y1 = max(c[1] for c in gq)
            x0 = min(c[2] for c in gq); x1 = max(c[3] for c in gq)
            cx, cy = (x0 + x1) / 2 / S - 0.5, (y0 + y1) / 2 / S - 0.5
            if 0 <= cx < 16 and 0 <= cy < 16: out.append([int(cx), int(cy), num])
        res[name] = sorted(out, key=lambda r: (r[1], r[0]))
    return res, sorted(tmpl)


if __name__ == '__main__':
    g, pdf = sys.argv[1], sys.argv[2]
    only = set(sys.argv[3:])
    wp = json.load(open(f'./warp_{g}.json'))
    pages = {}
    res = {}
    for name, w in sorted(wp.items()):
        if only and name not in only: continue
        p = w['page']
        if p not in pages:
            subprocess.run(['pdftoppm', '-f', str(p), '-l', str(p), '-r', '200', '-png', pdf, f'/tmp/mk_{p}'])
            f = [q for q in os.listdir('/tmp') if q.startswith(f'mk_{p}-') and q.endswith('.png')][0]
            pages[p] = np.asarray(Image.open('/tmp/' + f).convert('RGB')).astype(float)
        res[name] = straighten(pages[p], w)
    found, digits = find_all(res)
    print('digits learnt:', digits)
    for name, r in found.items(): print(name, len(r), r, flush=True)
    json.dump(found, open(f'./markers_{g}.json', 'w'))
