#!/usr/bin/env python3
# probe.py <slot> <x,y> <layer> <out.json> [VAR]: a map's real moves found by the engine itself - from each place
# reached, a step each way (TELE there with the map's own state word VAR (default 4C6B - Secret's mines: which of a
# level's two halves) POKEd, then F); the state after the step read back (PEEKEND). Places = (x, y, state).
# A fight / question on the step is noted, not crossed.
import json, os, subprocess, sys, re
P = '/tmp/claude-0/knplay'; S = sys.argv[1]; sx, sy = map(int, sys.argv[2].split(',')); L0 = int(sys.argv[3]); out = sys.argv[4]
V = sys.argv[5] if len(sys.argv) > 5 else '4C6B'
LOG = f'{P}/run_probe.log'
def step(x, y, l, d):
    env = dict(os.environ, TAILN='1', RUNLOG=LOG)
    subprocess.run([f'{P}/bin/p.sh', 'F', f'SAVE={S}', f'TELE={x},{y},{d}', f'POKE={V}={l}', f'PEEKEND={V}', 'ALLTEXT=1'],
                   capture_output=True, text=True, env=env, timeout=300)
    o = open(LOG, errors='replace').read()
    now = re.findall(r'now (\d+),(\d+) (\w)', o); scr = re.findall(r'\? screen (\d+) menu \[([^\]]*)\]', o)
    txt = re.findall(r'TEXT[^:]*: (.*)', o); pe = re.findall(r'PEEKEND \w+ = (\w+)', o)
    if not now: return None
    sc = scr[-1] if scr else ('?', '')
    return [int(now[-1][0]), int(now[-1][1]), int(pe[-1], 16) if pe else l, sc[0], sc[1][:40], txt[-1][:70] if txt else '']
seen = {(sx, sy, L0)}; front = [(sx, sy, L0)]; edges = {}
while front:
    nf = []
    for (x, y, l) in front:
        for d in (0, 2, 4, 6):
            r = step(x, y, l, d)
            if not r: continue
            edges[f'{x},{y},{l},{d}'] = r
            nx, ny, nl, sc, menu, t = r
            if (nx, ny, nl) not in seen:   # (a fight / question square too: the party stands there after it)
                seen.add((nx, ny, nl)); nf.append((nx, ny, nl))
    front = nf
    print(len(seen), 'places', flush=True)
    json.dump(edges, open(out, 'w'))
