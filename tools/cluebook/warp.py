"""A found map's grid as the scan really drew it - the page curled near the binding bends the straight lines (Tom,
2026-11-27). For each of the engine's walls the drawn line is looked for across it (three points a side) and two
smooth maps fitted: X(u, v) from the walls that run up and down, Y(u, v) from those that run across, (u, v) the grid's
own coordinates (0 - 16, a square a unit). Polynomials of degree 3 in u and v (10 terms each).

    from warp import measure, fit, ev
    pts = measure(S, block, X, Y, C)        # S: the page's ink (200 dpi), X / Y / C: refine()'s registration
    cx, cy, info = fit(pts)                 # the coefficients; info: points used, rms, the bend at its most
    ev(cx, u, v), ev(cy, u, v)              # a grid point on the page

Run alone: warp.py <game> <pdf> - every map of read_<game>.json, its rms and bend (pixels at 200 dpi)."""
import numpy as np

TERMS = [(i, j) for i in range(4) for j in range(4) if i + j <= 3]


def basis(u, v):
    u = np.asarray(u, float) / 16.0
    v = np.asarray(v, float) / 16.0
    return np.stack([u ** i * v ** j for i, j in TERMS], -1)


def ev(c, u, v):
    return basis(u, v) @ c


def walls(bk):
    out = set()
    for y in range(16):
        for x in range(16):
            q = bk['sq'][y * 16 + x]
            if q[0]: out.add((x, y, True))
            if q[6]: out.add((x, y + 1, True))
            if q[9]: out.add((x, y, False))
            if q[3]: out.add((x + 1, y, False))
    return out


def find_line(S, px, py, horizontal, reach):
    """The drawn line's place across a side near (px, py): the ink's weighted middle within reach; None if none."""
    H, W = S.shape
    px, py = int(round(px)), int(round(py))
    if horizontal:
        if not (0 <= px < W): return None
        lo, hi = max(0, py - reach), min(H, py + reach + 1)
        col = S[lo:hi, max(0, px - 2):px + 3].sum(1).astype(float)
    else:
        if not (0 <= py < H): return None
        lo, hi = max(0, px - reach), min(W, px + reach + 1)
        col = S[max(0, py - 2):py + 3, lo:hi].sum(0).astype(float)
    if col.size == 0 or col.max() < 3: return None
    k = int(np.argmax(col))
    a, b = max(0, k - 3), min(col.size, k + 4)
    w = col[a:b]
    return lo + float((np.arange(a, b) * w).sum() / w.sum())


def measure(S, bk, X, Y, C, cx=None, cy=None, reach=None):
    """(u, v, value, horizontal): a wall line's place on the page across it, three points a side."""
    reach = reach or int(C * 0.3)
    pts = []
    for x, y, h in walls(bk):
        for t in (0.25, 0.5, 0.75):
            u, v = (x + t, y) if h else (x, y + t)
            if cx is not None:
                px, py = float(ev(cx, u, v)), float(ev(cy, u, v))
            else:
                px, py = X + u * C, Y + v * C
            m = find_line(S, px, py, h, reach)
            if m is not None: pts.append((u, v, m, h))
    return pts


def fit(pts, X=None, Y=None, C=None):
    pts = list(pts)
    out = []
    info = {}
    for h, name in ((False, 'x'), (True, 'y')):
        P = np.array([(u, v, m) for u, v, m, hh in pts if hh == h])
        if len(P) < 12: return None, None, {'points': len(P)}
        keep = np.ones(len(P), bool)
        for _ in range(3):
            A = basis(P[keep, 0], P[keep, 1])
            c, *_ = np.linalg.lstsq(A, P[keep, 2], rcond=None)
            r = np.abs(basis(P[:, 0], P[:, 1]) @ c - P[:, 2])
            keep = r < max(2.5, 3 * np.median(r[keep]))
        res = basis(P[keep, 0], P[keep, 1]) @ c - P[keep, 2]
        info[name + '_points'] = int(keep.sum())
        info[name + '_rms'] = float(np.sqrt((res ** 2).mean()))
        out.append(c)
    cx, cy = out
    # the bend: how far the fitted grid strays from the straight one fitted to the same corners
    g = np.array([(u, v) for u in range(17) for v in range(17)], float)
    gx, gy = ev(cx, g[:, 0], g[:, 1]), ev(cy, g[:, 0], g[:, 1])
    A = np.c_[np.ones(len(g)), g]
    ax, *_ = np.linalg.lstsq(A, gx, rcond=None)
    ay, *_ = np.linalg.lstsq(A, gy, rcond=None)
    info['bend'] = float(max(np.abs(A @ ax - gx).max(), np.abs(A @ ay - gy).max()))
    return cx, cy, info


def solve(S, bk, X, Y, C):
    cx, cy, info = fit(measure(S, bk, X, Y, C))
    if cx is None: return None, None, info
    cx, cy, info = fit(measure(S, bk, X, Y, C, cx, cy, reach=int(C * 0.15)))
    return cx, cy, info


if __name__ == '__main__':
    import sys, json
    sys.path.insert(0, '.')
    from cluemap import render, ink
    from match import geo_blocks
    g, pdf = sys.argv[1], sys.argv[2]
    blocks = {b['name']: b for b in geo_blocks(f'./geo_{g}.txt')}
    rd = json.load(open(f'./read_{g}.json'))
    pages = {}
    out = {}
    for name, e in sorted(rd.items()):
        p = e['page']
        if p not in pages: pages[p] = ink(render(pdf, p, 200))
        s, X, Y, C = e['reg']
        cx, cy, info = solve(pages[p], blocks[name], X, Y, C)
        print(name, p, {k: round(v, 2) if isinstance(v, float) else v for k, v in info.items()}, flush=True)
        if cx is not None: out[name] = {'page': p, 'cx': list(map(float, cx)), 'cy': list(map(float, cy)), **info}
    json.dump(out, open(f'./warp_{g}.json', 'w'))
