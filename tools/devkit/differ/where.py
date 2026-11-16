#!/usr/bin/env python3
# where.py <run name>... - where an autodiff run's steps differ: for each
# differing step the pixels above the menu row (their box) and on it, and a
# picture engine | DOSBox in runs/<name>/s_<step>.png (scratch: game screens).
# The menu row's differences alone are mostly the engine's lit word / boxed
# list line (CLAUDE.md, menu highlight - kept on purpose).
import json, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import autodiff as A

for name in sys.argv[1:]:
    w = os.path.join(A.ROOT, 'runs', name)
    r = json.load(open(w + '/report.json'))
    print('==', name, 'identical' if r['identical'] else '')
    for s in r['steps']:
        if not s.get('diff'):
            continue
        if 'engine' not in s:
            print('  %s missing' % s['step']); continue
        _, _, a = A.read_ppm(w + '/eng/out/' + s['engine'])
        _, _, d = A.read_ppm(w + '/dshots/' + s['step'] + '.ppm')
        at = [k for k, (x, y) in enumerate(zip(a, d)) if x != y]
        up = [k for k in at if k < 192 * 320]
        box = (min(k % 320 for k in up), max(k % 320 for k in up), min(k // 320 for k in up), max(k // 320 for k in up)) if up else ''
        print('  %s %-22s above %5d %s  menu row %d' % (s['step'], s['engine'], len(up), box, len(at) - len(up)))
        if up:
            subprocess.run(['convert', w + '/eng/out/' + s['engine'], w + '/dshots/' + s['step'] + '.ppm', '+append',
                            '-scale', '200%', w + '/s_%s.png' % s['step']])
