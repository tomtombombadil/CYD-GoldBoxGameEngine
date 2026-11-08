#!/usr/bin/env python3
# The DOSBox differ (the side-by-side play test, fast): the ORIGINAL in a
# patched DOSBox (dosbox-auto - GPL, built in scratch by the Project's
# build_dosbox.sh + autodos.inc, never in this repo) and the engine's
# playsim (built with -DCYD_TEST_HOOKS -DCYD_MENU_HIGHLIGHT=0: the menus lit
# as the originals) take the same inputs; after each one both screens are
# compared pixel for pixel (by EGA colour index - DOSBox's 6-bit DAC and the
# engine's palette differ in the low bits).
#
#   autodiff.py base <short> [name]          a base: the game copied twice into
#       /tmp/claude-0/differ/base/<name>/{dos,eng}; the DOS copy's program
#       un-EXEPACKed with a fixed seed (fixseed.py)
#   autodiff.py run <scenario.json>          one scenario -> runs/<name>/report.json
#       (+ diff.png: engine | DOSBox | the differing pixels, at the first step that differs)
#   autodiff.py batch <dir> [report]         every *.json there; a line each + the totals
#
# A scenario: {"name", "game": "poolrad", "base": name, "save": a SAVGAMx.DAT
# to load (null: the base's own), "slot": "B", "tokens": "...", "env": {...}}.
# Tokens (the playsim's moves): F L R B steps / turns, A Area, K Look, "."
# Return / a tap, "/X" the menu word X, "|X" the party menu's line X, "^" "v"
# a list's Up / Down (the original: Home / End), "!" Select (Return),
# "=text;" typed + Return, "," nothing, "&N;" N seconds, "~n:v;" the engine's
# next die of n faces rolls v (the original runs on its fixed seed).
# Env: DOSBOX_AUTO (default /tmp/claude-0/differ/bin/dosbox-auto), SIM
# (default /tmp/claude-0/ag7/bin/simh0).
import json, os, shutil, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'oracles'))
import gb

ROOT = '/tmp/claude-0/differ'
DOSBOX = os.environ.get('DOSBOX_AUTO', ROOT + '/bin/dosbox-auto')
SIM = os.environ.get('SIM', '/tmp/claude-0/ag7/bin/simh0')
GAMES = {'poolrad': dict(num=1, sub='POOLRAD', exe='START.EXE', save='SAVGAM%s.DAT', keys={'F': 'up', 'L': 'left', 'R': 'right',
                                                                                        'B': 'down', 'A': 'a', 'K': 'l'})}
ENG_ENV = {'MANUAL': '1', 'NOCAMPFIX': '1', 'NOFIXES': '1', 'NOPORTRAIT': '1'}
CONF = """[sdl]
fullscreen=false
output=surface
usescancodes=false
waitonerror=false
[dosbox]
machine=svga_s3
memsize=16
[render]
frameskip=0
aspect=false
scaler=none
[cpu]
core=normal
cputype=auto
cycles=fixed 12000
[mixer]
nosound=true
[speaker]
pcspeaker=false
tandy=off
[sblaster]
sbtype=none
[gus]
gus=false
[midi]
mpu401=none
[joystick]
joysticktype=none
[autoexec]
mount c "%s"
c:
cd %s
%s
exit
"""
EGA = [(0, 0, 0), (0, 0, 170), (0, 170, 0), (0, 170, 170), (170, 0, 0), (170, 0, 170), (170, 85, 0), (170, 170, 170),
       (85, 85, 85), (85, 85, 255), (85, 255, 85), (85, 255, 255), (255, 85, 85), (255, 85, 255), (255, 255, 85),
       (255, 255, 255)]
SHOT_TOKENS = set('FLRBAK<>/|.,@%=^v!_')


def tokens(s):
    """the moves one by one: (token text, makes a snapshot)"""
    out, i = [], 0
    while i < len(s):
        c = s[i]
        if c in '/|@%' and i + 1 < len(s):
            out.append((s[i:i + 2], True)); i += 2
        elif c in '=~&':
            j = s.index(';', i)
            out.append((s[i:j + 1], c == '='))
            i = j + 1
        else:
            out.append((c, c in SHOT_TOKENS)); i += 1
    return out


def dos_keys(g, t):
    c = t[0]
    if c in g['keys']: return [g['keys'][c]]
    if c in '.!': return ['enter']
    if c in '/|': return [t[1].lower()]
    if c == '^': return ['home']
    if c == 'v': return ['end']
    if c == '=': return [k.lower() if k.isalnum() else {' ': 'space', '-': 'minus'}.get(k, k) for k in t[1:-1]] + ['enter']
    if c == ',': return []
    raise ValueError('token %s: no original keys' % t)


def read_ppm(p):
    d = open(p, 'rb').read()
    parts, k = [], 0
    while len(parts) < 4:
        while d[k:k + 1].isspace(): k += 1
        j = k
        while not d[j:j + 1].isspace(): j += 1
        parts.append(d[k:j]); k = j
    w, h = int(parts[1]), int(parts[2])
    px = d[k + 1:k + 1 + w * h * 3]
    cache, idx = {}, bytearray(w * h)
    for i in range(w * h):
        rgb = px[3 * i:3 * i + 3]
        v = cache.get(rgb)
        if v is None:
            v = min(range(16), key=lambda e: sum((a - b) ** 2 for a, b in zip(rgb, EGA[e])))
            cache[rgb] = v
        idx[i] = v
    return w, h, idx


def base(short, name=None):
    g = GAMES[short]
    name = name or short
    b = os.path.join(ROOT, 'base', name)
    shutil.rmtree(b, ignore_errors=True)
    src = gb.folder(short)
    for side in ('dos', 'eng'):
        os.makedirs(b + '/' + side)
        shutil.copytree(src, b + '/' + side + '/' + g['sub'])
    subprocess.check_call([sys.executable, os.path.join(HERE, 'fixseed.py'), short, b + '/dos/' + g['sub']])
    print('base %s: %s' % (name, b))


def run(path, keep=False):
    sc = json.load(open(path))
    g = GAMES[sc.get('game', 'poolrad')]
    name = sc['name']
    w = os.path.join(ROOT, 'runs', name)
    shutil.rmtree(w, ignore_errors=True)
    b = os.path.join(ROOT, 'base', sc.get('base', sc.get('game', 'poolrad')))
    for side in ('dos', 'eng'):
        shutil.copytree(b + '/' + side, w + '/' + side)
    slot = sc.get('slot', 'A')
    if sc.get('save'):
        for side in ('dos', 'eng'):
            shutil.copy(sc['save'], w + '/' + side + '/' + g['sub'] + '/' + g['save'] % slot)
    toks = tokens(sc['tokens'])
    # the original
    script = ['wait 2000', 'quiet 1500 30000', 'key enter', 'quiet 1000 20000', 'key enter', 'quiet 1000 20000',
              'key l', 'quiet 1000 20000', 'key ' + slot.lower(), 'quiet 1500 30000', 'shot start']
    n = 0
    for t, shot in toks:
        if t[0] == '&':
            script.append('wait %d' % (int(t[1:-1]) * 1000))
        elif t[0] in '~$?':
            continue
        else:
            for k in dos_keys(g, t):
                script += ['key ' + k, 'wait 150']
        if shot:
            script += ['quiet 700 12000', 'shot t%03d' % n]
            n += 1
    script.append('quit')
    os.makedirs(w + '/dshots')
    open(w + '/dos.script', 'w').write('\n'.join(script) + '\n')
    open(w + '/dos.conf', 'w').write(CONF % (w + '/dos', g['sub'], g['exe']))
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy', AUTODOS_SCRIPT=w + '/dos.script',
               AUTODOS_OUT=w + '/dshots')
    if sc.get('mem'): env['AUTODOS_MEM'] = '1'
    subprocess.run([DOSBOX, '-conf', w + '/dos.conf'], env=env, capture_output=True, timeout=1800)
    # the engine
    os.makedirs(w + '/eng/out'); os.makedirs(w + '/eng/_CYD', exist_ok=True)
    env = dict(os.environ, GAME=str(g['num']), SAVE=slot, **ENG_ENV)
    env.update({k: str(v) for k, v in sc.get('env', {}).items()})
    r = subprocess.run([SIM, w + '/eng/' + g['sub'], sc['tokens']], env=env, cwd=w + '/eng', capture_output=True, text=True,
                       timeout=600)
    open(w + '/eng.log', 'w').write(r.stdout + r.stderr)
    es = sorted(f for f in os.listdir(w + '/eng/out') if f.endswith('.ppm'))
    st = [i for i, f in enumerate(es) if f.endswith('_start.ppm')]
    es = es[st[0]:] if st else es
    es = [f for f in es if not f.endswith('_end.ppm')]
    # compared step by step
    steps, first = [], None
    names = ['start'] + ['t%03d' % i for i in range(n)]
    for i, nm in enumerate(names):
        dp = w + '/dshots/' + nm + '.ppm'
        if i >= len(es) or not os.path.exists(dp):
            steps.append({'step': nm, 'diff': -1})
            if first is None: first = i
            continue
        _, _, a = read_ppm(w + '/eng/out/' + es[i])
        _, _, d = read_ppm(dp)
        at = [k for k, (x, y) in enumerate(zip(a, d)) if x != y]
        # (the menu line's lit word: the engine lights a word on every menu -
        # the cursor keys' place, kept on purpose (CLAUDE.md, menu highlight) -
        # where Pool's menus light none; forgiven when the menu row's
        # differences span one word at most and nothing else differs)
        row = [k for k in at if k >= 192 * 320]
        xs = [k % 320 for k in row]
        forgiven = len(row) if row and len(row) == len(at) and max(xs) - min(xs) < 120 else 0
        diff = len(at) - forgiven
        steps.append({'step': nm, 'engine': es[i], 'diff': diff, 'menu_word': forgiven})
        if diff and first is None:
            first = i
            mask = bytes(255 if x != y else 0 for x, y in zip(a, d))
            open(w + '/mask.pgm', 'wb').write(b'P5 320 200 255\n' + mask)
            subprocess.run(['convert', w + '/eng/out/' + es[i], dp, w + '/mask.pgm', '+append', '-scale', '200%', w + '/diff.png'])
    rep = {'name': name, 'tokens': sc['tokens'], 'identical': first is None, 'first_diff': first, 'steps': steps}
    json.dump(rep, open(w + '/report.json', 'w'), indent=1)
    if not keep:
        for side in ('dos', 'eng'):
            shutil.rmtree(w + '/' + side + '/' + g['sub'], ignore_errors=True)
    return rep


def batch(d, out=None):
    lines, same = [], 0
    files = sorted(f for f in os.listdir(d) if f.endswith('.json'))
    for f in files:
        try:
            r = run(os.path.join(d, f))
        except Exception as e:                      # (a scenario that won't run is counted as differing)
            r = {'name': f, 'identical': False, 'first_diff': 'error %s' % e, 'steps': []}
        same += r['identical']
        s = r['steps'][r['first_diff']] if isinstance(r['first_diff'], int) and r['first_diff'] < len(r['steps']) else {}
        lines.append('%-40s %s %s' % (r['name'], 'identical' if r['identical'] else 'DIFFERS at %s' % r['first_diff'],
                                     '' if r['identical'] else s.get('diff', '')))
        print(lines[-1], flush=True)
    lines.append('== %d of %d identical' % (same, len(files)))
    print(lines[-1])
    if out:
        open(out, 'w').write('\n'.join(lines) + '\n')


if __name__ == '__main__':
    if sys.argv[1] == 'base': base(*sys.argv[2:])
    elif sys.argv[1] == 'run': print(json.dumps(run(sys.argv[2], keep=True), indent=1))
    elif sys.argv[1] == 'batch': batch(*sys.argv[2:])
