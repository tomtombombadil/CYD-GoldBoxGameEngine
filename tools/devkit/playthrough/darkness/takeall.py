#!/usr/bin/env python3
# takeall.py <slot> <tokens to a treasure screen> [seed]: every item taken, member after member - Darkness's
# treasure screen: "%m/T" (Take: the items list), then "/T" takes the lit line; a refused take (full / heavy)
# goes back ("/E") to the next member. Prints the tokens (the treasure screen still up) and how many are left.
import re, subprocess, sys, os
P = '/tmp/claude-0/dplay'; S = sys.argv[1]; T = sys.argv[2]
env = dict(os.environ, TAILN='0')
if len(sys.argv) > 3 and sys.argv[3]: env['SEED'] = sys.argv[3]
def run(t):
    subprocess.run([P + '/bin/p.sh', t, f'SAVE={S}'], env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return open(P + '/run.log', errors='replace').read()
left = None
for m in range(8):
    U = T + '%%%d/T' % m
    for k in range(40):
        log = run(U + '/T')
        got = re.findall(r'\[play\] take (ok|refused), left (\d+)', log)
        if not got: break
        res, n = got[-1]; left = int(n)
        if res == 'refused': break
        U += '/T'; T = U
        if left == 0: break
    if left == 0: break
    if T.endswith('/T') and not T.endswith('/E'): T += '/E'
print(T if left == 0 else T)
print('left', left, file=sys.stderr)
