#!/usr/bin/env python3
# vace_cmp.py <dosbox frames dir> <dosbox log> <engine frames dir> <engine log>: for each
# engine frame, the first DOSBox capture (after the last matched) with the very same
# pixels, and the two times - the movie's check (claude/vace_facts.md). Prints a
# summary: frames matched, the largest time difference. Scratch outputs only.
import os, re, sys
def frames(d, log):
    t = {int(m.group(1)): int(m.group(2)) for m in re.finditer(r'frame (\d+) at (\d+) ms', open(log).read())}
    out = []
    for k in sorted(t):
        fn = '%s/%05d.ppm' % (d, k)
        if os.path.exists(fn):
            b = open(fn, 'rb').read()
            out.append((t[k], b[b.index(b'255\n') + 4:]))
    return out
dos = frames(sys.argv[1], sys.argv[2]); eng = frames(sys.argv[3], sys.argv[4])
if len(sys.argv) > 5 and sys.argv[5] == 'every':     # (engine at every change: each DOSBox capture found there)
    worst = 0; miss = 0
    for td, img in dos:
        near = [te for te, e in eng if abs(te - td) <= 1500 and e == img]   # (the same picture within 1.5 s)
        if not near: miss += 1; print('DOSBox %6d ms: no engine state the same' % td); continue
        d = min(near, key=lambda te: abs(te - td)) - td; worst = max(worst, abs(d))
        if len(sys.argv) > 6: print('DOSBox %6d ms = engine %+d' % (td, d))
    print('DOSBox captures %d: %d found among the engine\'s %d states, largest time difference %d ms' % (len(dos), len(dos) - miss, len(eng), worst))
    sys.exit(0)
j = 0; matched = 0; worst = 0; miss = 0
for te, img in eng:
    k = next((i for i in range(j, len(dos)) if dos[i][1] == img), None)
    if k is None:
        miss += 1
        print('engine %6d ms: no DOSBox capture the same' % te)
        continue
    matched += 1; d = te - dos[k][0]; worst = max(worst, abs(d)); j = k
    print('engine %6d ms = DOSBox %6d ms (%+d)' % (te, dos[k][0], d))
print('matched %d of %d engine frames (DOSBox %d), largest time difference %d ms' % (matched, len(eng), len(dos), worst))
