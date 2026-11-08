#!/usr/bin/env python3
# The DOSBox differ's scenarios for one block (the old rb/mkblock + evsq +
# gentok, rebuilt): from the engine alone -
#   1. the block's map (the playsim's move "M": each square's event byte and
#      open sides) and its event squares: an event byte other than its
#      neighbour's, entered through an open side (a step from the neighbour
#      into it);
#   2. for each, a base save made by the playsim (SAVE=<base slot> loaded,
#      TELE0 to the neighbour facing the square, SAVENOW=<slot>) - the
#      original loads the same file;
#   3. the tokens worked out step by step from the engine's state ("?" after
#      the moves): text '.', a one-word menu '.', the FIRST menu with several
#      words one scenario per word ('/X'), later menus their first word,
#      lists 'v!', typed '=FRIEND;' / numbers '=1;', the treasure / shop
#      screens '/E'; stops at a fight, back at the exploring menu, or after
#      MAXTOK tokens.
# Usage: mkscen.py <out dir> [AREA BLOCK]   (no area / block: the base save's own)
# Env: SIM (default /tmp/claude-0/ag7/bin/simh0), BASE (default poolrad),
# BASESLOT (A), SLOT (B: the scenarios' save), MAXSC (200), MAXTOK (8).
import json, os, re, shutil, subprocess, sys

ROOT = '/tmp/claude-0/differ'
SIM = os.environ.get('SIM', '/tmp/claude-0/ag7/bin/simh0')
BASE = os.environ.get('BASE', 'poolrad')
BSLOT, SLOT = os.environ.get('BASESLOT', 'A'), os.environ.get('SLOT', 'B')
MAXSC, MAXTOK = int(os.environ.get('MAXSC', '200')), int(os.environ.get('MAXTOK', '8'))
SUB = 'POOLRAD'
ENV = {'GAME': '1', 'MANUAL': '1', 'NOCAMPFIX': '1', 'NOFIXES': '1', 'NOPORTRAIT': '1'}
out = sys.argv[1]
area_block = sys.argv[2:4]
W = os.path.join(ROOT, 'mk', os.path.basename(out.rstrip('/')))
shutil.rmtree(W, ignore_errors=True)
shutil.copytree(os.path.join(ROOT, 'base', BASE, 'eng'), W)
os.makedirs(W + '/out'); os.makedirs(W + '/_CYD', exist_ok=True)
shutil.rmtree(out, ignore_errors=True)
os.makedirs(out + '/saves', exist_ok=True)
savefile = W + '/' + SUB + '/SAVGAM%s.DAT'


def sim(moves, **env):
    e = dict(os.environ, **ENV)
    e.update({k: str(v) for k, v in env.items()})
    for f in os.listdir(W + '/out'):
        os.remove(W + '/out/' + f)
    r = subprocess.run([SIM, W + '/' + SUB, moves], env=e, cwd=W, capture_output=True, text=True, timeout=300)
    return r.stdout


start = dict(SAVE=BSLOT)
if area_block:
    start.update(AREA=area_block[0], BLOCK=area_block[1])
# 1. the map
m = sim('M?', **start)
grid = {}
for ln in m.splitlines():
    mm = re.match(r'\s+y\s*(\d+) (.*)$', ln)
    if mm:
        y = int(mm.group(1))
        for x, cell in enumerate(mm.group(2).split()):
            f, sides = cell.split(':')
            grid[x, y] = (int(f, 16), [c == '0' for c in sides])      # (the digits: a wall on that side)
DIRS = [(0, 0, -1), (2, 1, 0), (4, 0, 1), (6, -1, 0)]       # (facing, dx, dy): N E S W
cands = []
for (x, y), (f, sides) in sorted(grid.items(), key=lambda t: (t[0][1], t[0][0])):
    for k, (d, dx, dy) in enumerate(DIRS):
        s = (x + dx, y + dy)
        if s in grid and sides[k] and grid[s][0] != f and grid[s][0] != 0:
            cands.append((x, y, d, s))
print('%d event entries' % len(cands), file=sys.stderr)


def state(moves, env):
    o = sim(moves + '?', **env)
    q = [l for l in o.splitlines() if l.startswith('  ? screen')]
    if not q:
        return None
    mm = re.match(r'  \? screen (\d+) menu \[(.*)\|(.*)\] wait (\d+)', q[-1])
    words = mm.group(3).split()
    if int(mm.group(4)) == 2:                       # (a script's menu: its items as MANUAL printed them)
        ms = [l for l in o.splitlines() if l.startswith('  MENU prompt=')]
        if ms:
            words = re.findall(r'\[([^\]]*)\]', ms[-1])
    return int(mm.group(1)), mm.group(2), words, int(mm.group(4)), o


EXPLORE = ['Area', 'Cast', 'View', 'Encamp', 'Search', 'Look']
n = 0
for (x, y, d, s) in cands:
    if n >= MAXSC:
        break
    # 2. the save at the neighbour, facing the event square
    tag = '%s_%d_%d_%d' % ('_'.join(area_block) or 'base', x, y, d)
    sim('?', TELE0='%d,%d,%d' % (x, y, d), SAVENOW=SLOT, **start)
    sp = os.path.join(out, 'saves', tag + '.dat')
    shutil.copy(savefile % SLOT, sp)
    # 3. the tokens
    env = dict(SAVE=SLOT)
    shutil.copy(sp, savefile % SLOT)
    todo, done = ['F'], []
    while todo:
        toks = todo.pop()
        for _ in range(MAXTOK):
            st = state(toks, env)
            if st is None:
                break
            scr, prompt, words, wait, o = st
            if scr == 24 or (scr == 0 and wait == 0 and words == EXPLORE):
                break
            if wait == 1:
                toks += '.'
            elif wait == 2 and len(words) > 1 and '/' not in toks:
                todo += [toks + '/' + w[0].upper() for w in words[1:]]
                toks += '/' + words[0][0].upper()
            elif wait == 2:
                toks += '.' if len(words) <= 1 else '/' + words[0][0].upper()
            elif wait == 3:
                toks += 'v!'
            elif wait == 4:
                toks += '=1;'
            elif wait == 5:
                toks += '=FRIEND;'
            elif scr in (5, 16, 25) or words[-1:] == ['Exit']:
                toks += '/E'
            else:
                break
        done.append(toks)
    for i, toks in enumerate(done):
        sc = {'name': '%s_%d' % (tag, i), 'game': 'poolrad', 'base': BASE, 'save': sp, 'slot': SLOT, 'tokens': toks}
        json.dump(sc, open(os.path.join(out, sc['name'] + '.json'), 'w'))
        n += 1
        print(sc['name'], toks, flush=True)
print('%d scenarios in %s' % (n, out), file=sys.stderr)
