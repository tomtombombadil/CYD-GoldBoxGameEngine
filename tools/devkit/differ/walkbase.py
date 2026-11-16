#!/usr/bin/env python3
# walkbase.py <base> <slot> <walk tokens> <new base> <new slot> - a new differ
# base: <base> copied, the playsim loads <slot>, walks (each move's d20 told
# 1 - no random encounter, as mkwalk.py CALM=1) and saves as <new slot>; both
# sides get the save. The original's dice start again from its fixed seed at
# every load, so a far place is reached from a base near it (a long walk in
# DOSBox meets the encounters its seed gives - PoolRad's stirges at the 30th
# step from the harbour's base).
import os, shutil, subprocess, sys

ROOT = '/tmp/claude-0/differ'
SIM = os.environ.get('SIM', '/tmp/claude-0/ag7/bin/simh0')
SUB = 'POOLRAD'
base, slot, walk, new, nslot = sys.argv[1:6]
walk = ''.join('~20:1;%s$' % c if c in 'FB<>LR' else c for c in walk)
b, n = os.path.join(ROOT, 'base', base), os.path.join(ROOT, 'base', new)
shutil.rmtree(n, ignore_errors=True)
shutil.copytree(b, n)
w = n + '/eng'
os.makedirs(w + '/out', exist_ok=True); os.makedirs(w + '/_CYD', exist_ok=True)
env = dict(os.environ, GAME='1', MANUAL='1', NOCAMPFIX='1', NOFIXES='1', NOPORTRAIT='1', TPSEED='0', TPSKIP='2', SAVE=slot, SAVENOW=nslot)
o = subprocess.run([SIM, w + '/' + SUB, walk + '?'], env=env, cwd=w, capture_output=True, text=True, timeout=600).stdout
print('\n'.join(l for l in o.splitlines() if 'saved now' in l or l.startswith('  ? screen') or l.startswith('   now'))[-600:])
shutil.rmtree(w + '/out'); shutil.rmtree(w + '/_CYD')
for f in os.listdir(w + '/' + SUB):
    if f.upper().startswith(('SAVGAM' + nslot, 'CHRDAT' + nslot)):
        shutil.copy(w + '/' + SUB + '/' + f, n + '/dos/' + SUB + '/' + f)
print('base %s: slot %s' % (n, nslot))
