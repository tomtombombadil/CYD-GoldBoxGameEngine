#!/usr/bin/env python3
# drv3.py <slot> <start tokens> [seed]: a fight played by Tom's tactics, a step a turn.
#  - formation: each member's first turn takes the steps in FORM (env "NAME:dirs;NAME:dirs", dirs 0 N .. 7 NW)
#  - fighters: focus fire - the weakest foe next to them (the party's common target first); none next to them:
#    guard (the foes come to the line)
#  - Bran: Prayer first, then cures for a member under 40% (Cure Serious, then Light), else as a fighter
#  - Mira / Selene: Magic Missile on a leader (LEADER env, else the weakest foe in reach), Fireball on the best
#    cluster no member is within 3 squares of (not on fire creatures), Stinking Cloud likewise (2), Hold Person
#    on a person; else guard
#  - the spells lists are tracked as they're cast (the list closes up)
# prints WON / OUT and the party's hit points; the tokens in ftok.txt
import re, subprocess, sys, os
P = '/tmp/claude-0/cplay'; slot = sys.argv[1]; T = sys.argv[2]; seed = sys.argv[3] if len(sys.argv) > 3 else ''
env = dict(os.environ)
if seed: env['SEED'] = seed
SPELLS = {
    'MIRA': ['MM', 'MM', 'Sleep', 'Sleep', 'Knock', 'SC', 'SC', 'SC', 'FB', 'FB', 'FB', 'HP'],
    'SELENE': ['Bless', 'Bless', 'CLW', 'CLW', 'MM', 'MM', 'Sleep', 'Sleep', 'HP', 'HP', 'Silence', 'SC', 'SC', 'Prayer', 'FB', 'X'],
    'BRAN': ['Bless', 'Bless', 'CLW', 'CLW', 'HP', 'HP', 'Silence', 'Prayer', 'Prayer', 'Prayer', 'CSW', 'CSW'],
}
FIGHTERS = {'ALDRIC', 'THORIN', 'KESTREL', 'AKABAR BEL AKAS'}
FIREPROOF = ('EFREETI', 'SALAMANDER', 'FIRE')
PERSONS = ('ELF', 'DROW', 'KNIFE', 'THIEF', 'GUARD', 'MAGE', 'CLERIC', 'FIGHTER', 'LORD')
LEADER = os.environ.get('LEADER', '')
FORM = {}
for part in os.environ.get('FORM', '').split(';'):
    if ':' in part:
        n, ds = part.split(':'); FORM[n] = [int(c) for c in ds]
DIRS = {(0, -1): 0, (1, -1): 1, (1, 0): 2, (1, 1): 3, (0, 1): 4, (-1, 1): 5, (-1, 0): 6, (-1, -1): 7}
maxhp = {}; formed = set(); prayed = set(); lastsig = None; stuck = 0; aimn = 0


def run(t):
    subprocess.run([P + '/bin/p.sh', t + '?[', f'SAVE={slot}', 'PAGELOG=1'], env=dict(env, TAILN='0'),
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return open(P + '/run.log', errors='replace').read()


def cheb(a, b): return max(abs(a['x'] - b['x']), abs(a['y'] - b['y']))


def cast(who, name, target=None):
    lst = SPELLS[who]; i = lst.index(name); lst.pop(i)
    a = '/C' + 'v' * i + '/C'
    if target is not None: a += f'#@{target};/T~/E~'
    return a + ','


for step in range(int(os.environ.get('STEPS', '260'))):
    log = run(T)
    if '[fight] won' in log: print('WON', re.findall(r'\[fight\] won.*', log)[-1][:90]); break
    st = re.findall(r'  \? screen (\d+) menu \[([^\]]*)\]', log)
    if not st: print('no state'); break
    scr, menu = st[-1]
    if scr != '24': print('OUT', scr, menu[:60]); break
    figs = re.findall(r'  FIG (\d+) (\S) (.+?) at (\d+),(\d+) team (\d) hp (\d+)', log)
    n = len(set(f[0] for f in figs)); figs = figs[-n:]
    F = {int(f[0]): dict(name=f[2], x=int(f[3]), y=int(f[4]), team=int(f[5]), hp=int(f[6])) for f in figs}
    for k, v in F.items(): maxhp[k] = max(maxhp.get(k, 0), v['hp'])
    who = (re.findall(r'  \? turn (.+)', log) or [''])[-1].strip()
    sig = (who, menu, tuple(sorted((k, v['hp'], v['x'], v['y']) for k, v in F.items())))
    if sig == lastsig and 'Aim:' not in menu:
        T += '/E,/Q,' if 'Aim:' in menu else '{,/Q,'; lastsig = None; continue
    lastsig = sig
    if 'Continue Battle' in menu or 'Abort Spell' in menu: T += '/N,'; continue
    if 'Attack Ally' in menu: T += '/N{,'; continue
    if 'Aim:' in menu:
        aimn += 1
        if 'Target' in menu and aimn < 14: T += '/T,'
        elif aimn < 12: T += '/N'
        else: T += '/E,'; aimn = 0
        continue
    aimn = 0
    if 'Delay' in menu or 'Guard' in menu: T += '/G,' if 'Guard' in menu else '/E/Q,'; continue
    me = [k for k, v in F.items() if v['name'] == who and v['team'] == 0]
    if not me: T += '/Q,'; continue
    m = F[me[0]]
    foes = [k for k, v in F.items() if v['team'] == 1 and v['hp'] > 0]
    party = [k for k, v in F.items() if v['team'] == 0 and v['hp'] > 0]
    near = [k for k in foes if cheb(F[k], m) <= 1]
    a = None
    # formation steps first
    if who in FORM and who not in formed:
        formed.add(who)
        if FORM[who] and 'Move' in menu:
            T += '/M' + ''.join('}%d' % d for d in FORM[who]) + '{,'; continue
    can_cast = 'Cast' in menu and who in SPELLS
    sp = SPELLS.get(who, [])
    if can_cast and who == 'BRAN' or (can_cast and who == 'SELENE'):
        if 'Prayer' in sp and who not in prayed and any(cheb(F[k], m) <= 6 for k in foes):
            prayed.add(who); a = cast(who, 'Prayer')
        else:
            hurt = sorted([k for k in party if F[k]['hp'] < 0.4 * maxhp[k]], key=lambda k: F[k]['hp'] / maxhp[k])
            for c in ('CSW', 'CLW'):
                if hurt and c in sp: a = cast(who, c, hurt[0]); break
    if a is None and can_cast and who in ('MIRA', 'SELENE'):
        lead = [k for k in foes if LEADER and LEADER in F[k]['name'] and cheb(F[k], m) <= int(os.environ.get('RANGE','12'))]
        def best(radius, safe, fireok):
            bestk, bestn = None, 0
            for k in foes:
                if not fireok and any(x in F[k]['name'] for x in FIREPROOF): continue
                if cheb(F[k], m) > int(os.environ.get('RANGE','12')) or any(cheb(F[k], F[p]) <= safe for p in party): continue
                cnt = sum(1 for j in foes if cheb(F[j], F[k]) <= radius and not (not fireok and any(x in F[j]['name'] for x in FIREPROOF)))
                if cnt > bestn: bestk, bestn = k, cnt
            return bestk, bestn
        if lead and 'MM' in sp: a = cast(who, 'MM', lead[0])
        if a is None and 'FB' in sp:
            k, c = best(2, 3, False)
            if k is not None and c >= 2: a = cast(who, 'FB', k)
        if a is None and 'SC' in sp:
            k, c = best(1, 2, True)
            if k is not None and c >= 2: a = cast(who, 'SC', k)
        if a is None and 'MM' in sp:
            reach = [k for k in foes if cheb(F[k], m) <= 7]
            if reach: a = cast(who, 'MM', min(reach, key=lambda k: F[k]['hp']))
        if a is None and 'HP' in sp:
            ppl = [k for k in foes if any(x in F[k]['name'] for x in PERSONS) and cheb(F[k], m) <= 7]
            if ppl: a = cast(who, 'HP', min(ppl, key=lambda k: cheb(F[k], m)))
    if a is None and near:
        # focus: the foe next to the most party members, then the weakest
        t = min(near, key=lambda k: (-sum(1 for p in party if cheb(F[p], F[k]) <= 1), F[k]['hp']))
        dx = max(-1, min(1, F[t]['x'] - m['x'])); dy = max(-1, min(1, F[t]['y'] - m['y']))
        a = '/M}%d,' % DIRS[(dx, dy)]
    if a is None:
        a = '/D,'          # (Done: Guard comes up next)
    T += a
open(P + '/ftok.txt', 'w').write(T)
print('party', [(F[k]['name'], F[k]['hp']) for k in F if F[k]['team'] == 0])
