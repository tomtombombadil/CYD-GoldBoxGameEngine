#!/usr/bin/env python3
# Shared helpers for the behave oracle generators (tools/devkit/oracles):
# where each game's files are in scratch, its program unpacked (EXEPACK) with
# tools/devkit/tools/unpack into /tmp/claude-0/prog/, and readers of its data
# segment at the addresses the profile names (src/engine/profile.cpp).
# Nothing of the games is kept here - only file names, addresses and how the
# tables are laid out. Outputs go to /tmp/claude-0/bt (ORACLE_DIR).
import os, struct, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
GAMES = os.environ.get('GAMES', '/tmp/claude-0/games')
PROG = '/tmp/claude-0/prog'
OUT = os.environ.get('ORACLE_DIR', '/tmp/claude-0/bt')
UNPACK = os.path.join(PROG, 'unpack')


def folder(short):
    """The game's folder (PoolRad: the DAX files one down)."""
    g = os.path.join(GAMES, short)
    names = [n for n in sorted(os.listdir(g)) if n != '_CYD']
    f = os.path.join(g, names[0])
    if short == 'poolrad':
        f = os.path.join(f, 'POOLRAD')
    return f


def find(short, name):
    """A file of the game's folder, any case."""
    f = folder(short)
    for n in os.listdir(f):
        if n.upper() == name.upper():
            return os.path.join(f, n)
    raise FileNotFoundError(f + '/' + name)


def _unpacker():
    if not os.path.exists(UNPACK):
        os.makedirs(PROG, exist_ok=True)
        subprocess.check_call(['g++', '-std=c++17', '-O1', '-I' + os.path.join(REPO, 'src'),
                               os.path.join(REPO, 'tools/devkit/tools/unpack.cpp'),
                               os.path.join(REPO, 'src/engine/exepack.cpp'), os.path.join(REPO, 'src/engine/dax.cpp'),
                               '-o', UNPACK])
    return UNPACK


def image(short, exe='START.EXE'):
    """The unpacked program image (made once into /tmp/claude-0/prog)."""
    os.makedirs(PROG, exist_ok=True)
    out = os.path.join(PROG, '%s_%s.bin' % (short, exe.split('.')[0].lower()))
    if not os.path.exists(out):
        subprocess.check_call([_unpacker(), find(short, exe), out], stdout=subprocess.DEVNULL)
    return open(out, 'rb').read()


def raw(short, name):
    return open(find(short, name), 'rb').read()


class DS:
    """Reads a program's data segment at its image offset `base`."""
    def __init__(self, img, base):
        self.b, self.base = img, base

    def u8(self, a): return self.b[self.base + a]
    def s8(self, a): return struct.unpack_from('<b', self.b, self.base + a)[0]
    def u16(self, a): return struct.unpack_from('<H', self.b, self.base + a)[0]
    def s16(self, a): return struct.unpack_from('<h', self.b, self.base + a)[0]
    def i32(self, a): return struct.unpack_from('<i', self.b, self.base + a)[0]
    def bytes(self, a, n): return self.b[self.base + a:self.base + a + n]
    def sbytes(self, a, n): return [struct.unpack_from('<b', self.b, self.base + a + i)[0] for i in range(n)]


class Writer:
    def __init__(self, name):
        os.makedirs(OUT, exist_ok=True)
        self.path = os.path.join(OUT, 'oracle_%s.txt' % name)
        self.lines = []

    def c(self, text): self.lines.append('# ' + text)

    def row(self, key, *vals):
        flat = []
        for v in vals:
            flat.extend(v if isinstance(v, (list, tuple, bytes)) else [v])
        self.lines.append(' '.join([key] + [str(int(x)) for x in flat]))

    def done(self):
        with open(self.path, 'w') as f:
            f.write('\n'.join(self.lines) + '\n')
        print('wrote %s (%d lines)' % (self.path, len(self.lines)), file=sys.stderr)


# ---- AD&D rules (Players Handbook / DMG tables, the games' own sign
# convention: positive = better). Strength as one number: 3-17, 18 = 18,
# 19-23 = 18/01-50, /51-75, /76-90, /91-99, /00, then 19-25 = 24-30.
STR_HIT = {}
STR_DMG = {}
STR_ENC = {}
for g in range(31):
    STR_HIT[g] = (-3 if 1 <= g <= 3 else -2 if g in (4, 5) else -1 if g in (6, 7) else 1 if 17 <= g <= 19 else
                  2 if 20 <= g <= 22 else 3 if 23 <= g <= 25 else 4 if g in (26, 27) else g - 23 if g >= 28 else 0)
    STR_DMG[g] = (-2 if g in (1, 2) else -1 if 3 <= g <= 5 else 1 if g == 16 else g - 16 if 17 <= g <= 19 else
                  g - 17 if 20 <= g <= 29 else 14 if g == 30 else 0)
    STR_ENC[g] = {1: -350, 2: -350, 3: -350, 4: -250, 5: -250, 6: -150, 7: -150, 12: 100, 13: 100, 14: 200, 15: 200,
                  16: 350, 17: 500, 18: 750, 19: 1000, 20: 1250, 21: 1500, 22: 2000, 23: 3000, 24: 4000, 25: 5000,
                  26: 6000, 27: 7500, 28: 9000, 29: 12000, 30: 15000}.get(g, 0)
# (19's allowance: the DMG has 4,500; the games' encumbrance routine 4,000 -
# coab's listing of it, the original's code)
DEX_AC = {d: (-4 if d <= 3 else d - 7 if d <= 6 else d - 14 if 15 <= d <= 18 else 4 if d in (19, 20) else
              5 if 21 <= d <= 23 else 6 if d >= 24 else 0) for d in range(26)}
DEX_MISSILE = {d: (-4 if d <= 2 else d - 6 if d <= 5 else d - 15 if 16 <= d <= 18 else 3 if d in (19, 20) else
                   4 if 21 <= d <= 23 else 5 if d >= 24 else 0) for d in range(26)}
