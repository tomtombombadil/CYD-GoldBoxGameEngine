#!/usr/bin/env python3
# mkfight.py <out script> [poke...]: the Dark Queen's save A to the dragons
# at Caergoth (walk.txt), then Magic On (DS 0x33CF:B1CB, this boot's DS) and
# Quick for everyone, then every new screen (run with AUTODOS_FRAMES=1).
# Extra args: more "SEG:OFF VV" pokes before Quick (e.g. memorized lists).
import os, sys
here = os.path.dirname(os.path.abspath(__file__))
pre = open(os.path.join(here, 'walk.txt')).read().split()
L = ['wait 4000', 'key enter', 'wait 3000', 'key p', 'wait 4000', 'key l', 'wait 3000', 'key t', 'wait 3000', 'key a', 'wait 6000']
for k in pre: L += ['key ' + k, 'wait 300', 'quiet 800 15000', 'wait 300']
L += ['shot f00'] + ['poke ' + p for p in sys.argv[2:]] + ['poke 33CF:B1CB 1']
for i in range(8): L += ['key q', 'wait 400', 'quiet 800 15000', 'shot q%02d' % i]
for i in range(60): L += ['wait 2000', 'shot r%03d' % i]
L.append('quit')
open(sys.argv[1], 'w').write('\n'.join(L) + '\n')
