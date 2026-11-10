#!/usr/bin/env python3
# shots_viewscan.py <short> [ENV=value ...] - where a screenshot's 3D view
# shows something: the playsim (shots_saves.sh's sim) TELE0'd to every square
# and facing of the save's area, a turn left (the moves argument "L"), each
# end snapshot kept as crops/<short>/<x>_<y>_<dir>.png and a contact sheet
# vsheet_<short>.png of 30 of them with their places (pick one by eye: the
# scores only sort). The pictures are game screens: scratch only.
import glob, os, random, subprocess, sys
from PIL import Image, ImageDraw
g = sys.argv[1]
extra = dict(a.split('=', 1) for a in sys.argv[2:])
D = os.environ.get("WORK", "/tmp/claude-0/shots")
here = os.path.dirname(os.path.abspath(__file__))
res = []
for y in range(16):
    for x in range(16):
        for d in (0, 2, 4, 6):
            env = dict(os.environ, SAVE='A', WORK=D + '/sim_scan')
            env.update(extra)
            env['TELE0'] = '%d,%d,%d' % (x, y, d)
            subprocess.run(['bash', '-c', 'source <(sed -n "/^sim()/,/^}/p" %s/shots_saves.sh); SIM=${SIM:-/tmp/claude-0/ag7/bin/simshots}; GF=${GF:-/tmp/claude-0/shots/gf}; W=$WORK; sim %s L' % (here, g)],
                           env=env, capture_output=True, timeout=60)
            fs = glob.glob(D + '/sim_scan/%s/out/*_end.ppm' % g)
            if not fs:
                continue
            im = Image.open(fs[0]).convert('RGB')
            px = im.load()
            e = sum(1 for yy in range(4, 120) for xx in range(4, 130) if px[xx, yy] != px[xx + 1, yy])
            os.makedirs(D + '/crops/' + g, exist_ok=True)
            im.save(D + '/crops/%s/%d_%d_%d.png' % (g, x, y, d))
            res.append((e, x, y, d))
res.sort(reverse=True)
pick = res[:15] + random.Random(1).sample(res[15:], min(15, max(0, len(res) - 15)))
w, h = 136, 128
sh = Image.new('RGB', (w * 6, (h + 12) * 5))
dr = ImageDraw.Draw(sh)
for i, r in enumerate(pick):
    X, Y = (i % 6) * w, (i // 6) * (h + 12)
    sh.paste(Image.open(D + '/crops/%s/%d_%d_%d.png' % (g, r[1], r[2], r[3])).crop((0, 0, w, h)), (X, Y + 12))
    dr.text((X + 2, Y), '%d,%d,%d' % r[1:], fill=(255, 255, 0))
sh.save(D + '/vsheet_%s.png' % g)
print('sheet: ' + D + '/vsheet_%s.png' % g)
