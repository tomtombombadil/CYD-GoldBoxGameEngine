#!/usr/bin/env python3
# takeall.py <slot> <tokens to a treasure screen> [seed]: every item taken (Tom, 2026-11-16: "pick up all the
# items you can"), member after member (a refused take - full or too heavy - moves to the next); prints the
# tokens after the takes (the treasure screen still up) and how many items were left.
import re, subprocess, sys, os
P = '/tmp/claude-0/cplay'; S = sys.argv[1]; T = sys.argv[2]
env = dict(os.environ, TAILN='0')
if len(sys.argv) > 3 and sys.argv[3]: env['SEED'] = sys.argv[3]


def run(t):
    subprocess.run([P + '/bin/p.sh', t, f'SAVE={S}'], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return open(P + '/run.log', errors='replace').read()


left = None
for m in range(7):
    for k in range(17):
        log = run(T + '%%%d/T/I@1/T' % m)
        got = re.findall(r'\[play\] take (ok|refused), left (\d+)', log)
        if not got: break                       # (no Take - nothing there / not a member)
        res, n = got[-1]; left = int(n)
        if res == 'refused': break
        T += '%%%d/T/I@1/T/E' % m if left else '%%%d/T/I@1/T' % m
        if left == 0: break
    if left == 0: break
print(T)
print('left', left, file=sys.stderr)
