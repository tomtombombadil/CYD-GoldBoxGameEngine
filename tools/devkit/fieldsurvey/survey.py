#!/usr/bin/env python3
# survey.py <game> <sim binary> <out dir> [area:block ...]
# Every script block's fight field in the engine (2026-11-15, Tom: "spot
# check each game and each map"): the block loaded as a script would (AREA /
# BLOCK), its first LOAD MONSTER run (RUNAT, FIGHT=1), the field at the first
# party member's menu snapshotted; one line a block: indoors / outdoors, the
# square. Blocks: the game's ECL files (ECL<n>.DAX: area n) unless given.
# The snapshots show the game's pictures: scratch only.
import glob, os, re, struct, subprocess, sys
game, sim, out = sys.argv[1:4]
R = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
G = {'curse': 'curse', 'poolrad': 'poolrad', 'secret': 'secret', 'gateway': 'gateway', 'champions': 'champions',
     'knights': 'knights', 'darkness': 'darkness', 'treasures': 'treasures', 'queen': 'queen'}[game]
os.makedirs(out, exist_ok=True)
pairs = []
if len(sys.argv) > 4:
    pairs = [tuple(int(x) for x in p.split(':')) for p in sys.argv[4:]]
else:
    for p in sorted(glob.glob('/tmp/claude-0/games/%s/**/ECL*.DAX' % G, recursive=True)):
        m = re.search(r'ECL(\d+)\.DAX$', p, re.I)
        d = open(p, 'rb').read()
        n = struct.unpack('<H', d[:2])[0] // 9
        for i in range(n): pairs.append((int(m.group(1)), d[2 + 9 * i]))


def run(env, moves=''):
    e = dict(os.environ); e.update(env); e.setdefault('SAVE', 'A')
    r = subprocess.run(['bash', R + '/tools/devkit/run.sh', game, sim, moves], env=e, capture_output=True, text=True, errors='replace', timeout=300)
    return r.stdout + r.stderr


log = open(os.path.join(out, 'survey.txt'), 'w')
for area, block in pairs:
    tag = 'sv%s%d_%d' % (game[:3], area, block)
    o = run({'AREA': str(area), 'BLOCK': str(block), 'FINDMON': '1', 'TAG': tag, 'MS': '3000'})
    adds = re.findall(r'LOAD MONSTER at ([0-9A-F]{4})', o)
    if not adds:
        print('%d:%d no LOAD MONSTER' % (area, block), file=log, flush=True)
        continue
    W = '/tmp/claude-0/ag7/run/%s-%s' % (game, tag)
    os.makedirs(W + '/out', exist_ok=True)
    for f in glob.glob(W + '/out/*'): os.remove(f)
    o = run({'AREA': str(area), 'BLOCK': str(block), 'RUNAT': adds[0], 'FIGHT': '1', 'FIGHTOPS': '#', 'KEEP': '1',
             'TAG': tag, 'MS': '3000'})
    fl = re.findall(r'a fight: .*', o)
    pos = re.findall(r'now (\d+,\d+ \w)', o)
    shots = sorted(glob.glob(W + '/out/*fops0.ppm'))
    if shots:
        subprocess.run(['convert', shots[0], os.path.join(out, '%02d_%03d.png' % (area, block))])
    print('%d:%d at %s %s %s' % (area, block, adds[0], fl[0] if fl else 'no fight', 'shot' if shots else 'no shot'),
          file=log, flush=True)
    subprocess.run(['rm', '-rf', W])
