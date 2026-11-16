#!/usr/bin/env python3
# The DOSBox differ's scenarios along a walk (Tom, 2026-11-15: "start a play
# thru near and exit to the wilderness, then go out and move to those
# places"): from a base's save (both games load the same file) the given
# moves, then every way through what follows, worked out from the engine
# alone as mkscen.py does - text '.', one-word menus '.', a menu of several
# words one scenario per word (up to BRANCH menus deep; later ones their
# first word), lists 'v!', typed '=FRIEND;' / numbers '=1;', the treasure /
# shop screens '/E'; a scenario ends at a fight, back exploring, or after
# MAXTOK tokens past the walk.
# FIGHT=1: fights played out (each fighter Quick, the pages answered).
# STEPWISE=1: the walk crosses other events (texts answered on the way).
# Usage: mkwalk.py <out dir> <base> <slot> <walk tokens> [name]
# Env: SIM (default /tmp/claude-0/ag7/bin/simh0), BEGIN=1 (the original
# BEGINs after its load - a wilderness save), BRANCH (2), MAXTOK (14),
# MAXSC (60), CALM=1 (no random encounter on the walk: each move's d20 told
# 1 - "~20:1;" before it, "$" after; the original's own fixed seed meets
# none on a walk that matches - a DOSBox encounter shows as a difference).
# The overland walks: the playsim's OVPATH=x,y prints the shortest one.
import json, os, re, shutil, subprocess, sys

ROOT = '/tmp/claude-0/differ'
SIM = os.environ.get('SIM', '/tmp/claude-0/ag7/bin/simh0')
SUB = 'POOLRAD'
ENV = {'GAME': '1', 'MANUAL': '1', 'NOCAMPFIX': '1', 'NOFIXES': '1', 'NOPORTRAIT': '1', 'TPSEED': '0', 'TPSKIP': '2'}
FIGHT = os.environ.get('FIGHT') == '1'
BRANCH, MAXTOK, MAXSC = int(os.environ.get('BRANCH', '2')), int(os.environ.get('MAXTOK', '14')), int(os.environ.get('MAXSC', '60'))
out, base, slot, walk = sys.argv[1:5]
if os.environ.get('CALM') == '1':
    walk = ''.join('~20:1;%s$' % c if c in 'FB<>LR' else c for c in walk)
name = sys.argv[5] if len(sys.argv) > 5 else os.path.basename(out.rstrip('/'))
W = os.path.join(ROOT, 'mk', 'walk_' + name)
shutil.rmtree(W, ignore_errors=True)
shutil.copytree(os.path.join(ROOT, 'base', base, 'eng'), W)
os.makedirs(W + '/out'); os.makedirs(W + '/_CYD', exist_ok=True)
os.makedirs(out, exist_ok=True)
EXPLORE = ['Cast', 'View', 'Encamp', 'Search', 'Look']


def state(moves):
    for f in os.listdir(W + '/out'):
        os.remove(W + '/out/' + f)
    e = dict(os.environ, SAVE=slot, **ENV)
    o = subprocess.run([SIM, W + '/' + SUB, moves + '?'], env=e, cwd=W, capture_output=True, text=True, timeout=300).stdout
    q = [l for l in o.splitlines() if l.startswith('  ? screen')]
    if not q:
        return None
    mm = re.match(r'  \? screen (\d+) menu \[(.*)\|(.*)\] wait (\d+)', q[-1])
    words = mm.group(3).split()
    if int(mm.group(4)) == 2:
        ms = [l for l in o.splitlines() if l.startswith('  MENU prompt=')]
        if ms:
            words = re.findall(r'\[([^\]]*)\]', ms[-1])
    return int(mm.group(1)), words, int(mm.group(4))


if os.environ.get('STEPWISE') == '1':
    # (a walk through other events: each move's text answered '.', a
    # one-word menu its word; a menu of several ends the walk there)
    toks, i = '', 0
    while i < len(walk):
        j = walk.index('$', i) + 1 if walk.startswith('~', i) else i + 1
        toks += walk[i:j]; i = j
        for _ in range(400 if FIGHT else 6):
            st = state(toks)
            if st is None or st[2] == 0:
                break
            if st[2] == 1 or (st[2] == 2 and len(st[1]) == 1):
                toks += '.'
            elif FIGHT and st[0] == 24:
                toks += '/Q' if 'Quick' in st[1] else '.'
            elif FIGHT and st[2] == 2 and 'Combat' in st[1]:
                toks += '/C'
            else:
                i = len(walk)
                break
    walk = toks
    print('walk', walk, file=sys.stderr)
todo, done = [(walk, 0)], []
while todo and len(done) < MAXSC:
    toks, menus = todo.pop()
    for _ in range(MAXTOK):
        st = state(toks)
        if st is None:
            break
        scr, words, wait = st
        if scr == 24 and FIGHT:
            # (a fight played out: every fighter Quick - the engine's dice the
            # original's own, TPSEED - its pages answered)
            toks += '/Q' if 'Quick' in words else '.'
            continue
        if scr in (1, 24) or (scr == 0 and wait == 0 and words[-5:] == EXPLORE and toks != walk):
            break
        if wait == 1:
            toks += '.'
        elif wait == 2 and len(words) > 1:
            if menus < BRANCH:
                todo += [(toks + '/' + w[0].upper(), menus + 1) for w in words[1:]]
            toks += '/' + words[0][0].upper()
            menus += 1
        elif wait == 2:
            toks += '.'
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
    sc = {'name': '%s_%d' % (name, i), 'game': 'poolrad', 'base': base, 'save': None, 'slot': slot,
          'begin': os.environ.get('BEGIN') == '1', 'tokens': toks}
    json.dump(sc, open(os.path.join(out, sc['name'] + '.json'), 'w'))
    print(sc['name'], toks, flush=True)
print('%d scenarios in %s' % (len(done), out), file=sys.stderr)
