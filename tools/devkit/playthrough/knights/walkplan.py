#!/usr/bin/env python3
# walkplan.py <slot> <probe.json> <goal x,y,l> [last-dir]: walk the probe's graph to the goal for real; an
# encounter on the way (a menu with Attack) is fought (fight.sh, its env from ours) and the walk re-planned from
# where the fight left the party; stops at the goal (or the event the last step starts) and says what it shows
import os, re, subprocess, sys, time
P = '/tmp/claude-0/knplay'; S, J, G = sys.argv[1], sys.argv[2], sys.argv[3]; LD = sys.argv[4] if len(sys.argv) > 4 else None
D = {'N': 0, 'E': 2, 'S': 4, 'W': 6}
def run(tok, *env):
    subprocess.run([f'{P}/bin/p.sh', tok, f'SAVE={S}', 'ALLTEXT=1', 'PEEKEND=4C6B', *env], capture_output=True, text=True, env=dict(os.environ, TAILN='1'))
    return open(f'{P}/run.log', errors='replace').read()
def state(o):
    n = re.findall(r'now (\d+),(\d+) (\w)', o); l = re.findall(r'PEEKEND \w+ = (\w+)', o)
    sc = re.findall(r'\? screen (\d+) menu \[([^\]]*)\]', o)
    return int(n[-1][0]), int(n[-1][1]), int(l[-1], 16), D[n[-1][2]], sc[-1] if sc else ('?', '')
for it in range(12):
    x, y, l, f, _ = state(run('LR'))
    args = [sys.executable, f'{P}/bin/plan.py', J, f'{x},{y},{l},{f}', G] + ([LD] if LD else [])
    T = subprocess.run(args, capture_output=True, text=True).stdout.strip()
    if T == 'NO WAY' or not T: print('no way from', x, y, l); sys.exit(1)
    toks = re.findall(r'[LRBF]', T)
    o = run(T); x2, y2, l2, f2, sc = state(o)
    if sc[1].startswith('|Move') and not LD and (x2, y2, l2) == tuple(map(int, G.split(','))):
        run(T, f'SAVENOW={S}'); print('at the goal', x2, y2, l2); break
    pos = re.findall(r'now (\d+,\d+ \w)', o)
    k = len(pos) - 1
    while k > 0 and pos[k - 1] == pos[-1]: k -= 1
    # (pos[k] is the place the stopping step reached: tokens 0..k were taken)
    k += 1
    pre = ''.join(toks[:max(k, 1)])
    if LD and len(toks) <= k + 1:
        print('at the event:', sc[1][:60]); print('\n'.join(re.findall(r'TEXT[^\n]*', o)[-4:])); open(f'{P}/tok.txt', 'w').write(T); break
    if 'Attack' in sc[1] or 'ATTACK' in sc[1]:
        w = sc[1].split()[0].lstrip('|')
        r = subprocess.run([f'{P}/bin/fight.sh', S, pre + '/' + w[0] + '.', f'wp{int(time.time())}'], capture_output=True, text=True)
        print('  fight on the way:', r.stdout.strip().split('\n')[-2][:100] if r.stdout.strip() else r.stderr[-200:])
        continue
    if 'Yes No' in sc[1] and 'LIFT' in o:
        run(pre + '/N', f'SAVENOW={S}'); print('  (the wheel lift: No)'); continue
    print('stopped:', sc, re.findall(r'TEXT[^\n]*', o)[-3:]); open(f'{P}/tok.txt', 'w').write(pre); break
