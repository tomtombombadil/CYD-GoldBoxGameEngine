#!/usr/bin/env python3
# (scratch work dir /tmp/claude-0/ag13/dos/<name>; needs the scratch-built dosbox-auto - the Project's
# devkit_dosbox_build.md / autodos patch. ROOT=1: the game folder is C:\ itself (GOG: Curse, PoolRad);
# OVERLAY=a=b,...: scratch files copied over the copy; AUTODOS_MEM=1 / AUTODOS_FRAMES=1 as autodos)
# dosrun.py <name> <game dir to copy> <subdir to cd> <command line> <script file>
# Runs the original in dosbox-auto on a fresh copy; shots in /tmp/claude-0/ag13/dos/<name>/shots (+.png)
import os, shutil, subprocess, sys, glob
name, src, sub, cmd, scr = sys.argv[1:6]
W = '/tmp/claude-0/ag13/dos/' + name
if not os.environ.get('KEEP'):
    shutil.rmtree(W, ignore_errors=True)
    shutil.copytree(src, W + ('/c' if os.environ.get('ROOT') else '/c/GAME'))
    for f in os.environ.get('OVERLAY', '').split(','):
        if f:
            a, b = f.split('=')
            shutil.copy(a, W + ('/c/' if os.environ.get('ROOT') else '/c/GAME/') + b)
shutil.rmtree(W + '/shots', ignore_errors=True); os.makedirs(W + '/shots/frames')
base = os.path.basename(src.rstrip('/'))
conf = open('/tmp/claude-0/differ/dos.conf.in').read().replace('@DIR@', W + '/c').replace('@RUN@', ('cd \\' if os.environ.get('ROOT') else 'cd \\GAME') + ('\\' + sub if sub != '.' else '') + '\n' + cmd)
open(W + '/dos.conf', 'w').write(conf)
shutil.copy(scr, W + '/dos.script')
env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', AUTODOS_SCRIPT=W + '/dos.script', AUTODOS_OUT=W + '/shots')
r = subprocess.run(['/tmp/claude-0/differ/bin/dosbox-auto', '-conf', W + '/dos.conf'], env=env, capture_output=True, text=True, timeout=3000)
open(W + '/dosbox.log', 'w').write(r.stdout + r.stderr)
for p in sorted(glob.glob(W + '/shots/*.ppm')):
    subprocess.run(['convert', p, '-scale', '200%', p[:-4] + '.png'])
print(W, len(glob.glob(W + '/shots/*.ppm')), 'shots')
