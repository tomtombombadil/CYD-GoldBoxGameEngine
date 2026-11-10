#!/usr/bin/env python3
# aicast.py [game ...]  (darkness treasures knights queen; none: all four)
# Rules round 20 - the computer's spell use test in the four games whose
# computer takes its spells by priority (Pools of Darkness, Treasures of the
# Savage Frontier, Death Knights of Krynn, the Dark Queen of Krynn): each
# game's per-target routine (Darkness GAME.OVR 0x2249A, Treasures 0x2A74B,
# Death Knights 0x22CE4, the Dark Queen DQK.EXE 0x2902C) RUN in Unicorn (emu16.py) for every
# spell of its table on target states drawn from these dimensions: on the
# side asked or not, hurt, a person or a monster by race, flags (record
# 0x1A4 / Death Knights 0x7A / the Queen's 0xC7), flags 2, size, Hit Dice,
# magic resistance, effects on it (those the routine asks about, found by
# a first run), effects on the caster, an enemy of it next to it (the
# count routine's answer). Needs unicorn (emu16.py says how).
# Output /tmp/claude-0/bt/oracle_aicast_<game>.txt, a line each:
#   use <spell> <side> <hurt> <person> <flags> <flags2> <size> <hd> <mr> <near> <t-effects|-> <c-effects|-> <result>
#   row <spell> <level> <aim> <effect> <castable> <priority> <side byte>
# (effects comma-separated hex; result the routine's signed byte: -10 the
# wrong side, -40 the Queen's, 0 no use, 1 a use).
import os, random, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gb
import emu16

ALL = ['darkness', 'treasures', 'knights', 'queen']

# Per game: the routine, its code segment's file start, DS:0 in the image,
# the record offsets it reads, the race codes (a person / a monster), the
# helpers' far addresses (has effect, the count of a fighter's foes near it,
# Treasures' stack check), the current fighter's far pointer (DS), the spell
# table and count.
GAMES = {
    'darkness': dict(file='GAME.OVR', prog=('darkness', 'GAME.EXE'), ds=0x9E70, fn=0x2249A, seg=0x1E642,
                     side=0x1EF, hpmax=0xB2, hp=0x1FC, race=0xAD, races=(5, 6), flags=0x1A4, flags2=0x1A5,
                     size=0x131, hd=0x138, mr=0x1EB, has=(0xEC, 0xA7), near=(0xEC, 0xC0), check=None,
                     cur=0x9CEE, spells=0x6519, stride=16, n=0x7F, pascal=True),
    'treasures': dict(file='GAME.OVR', prog=('treasures', 'GAME.EXE'), ds=0xC3E0, fn=0x2A74B, seg=0x26642,
                      side=0x1EF, hpmax=0xB2, hp=0x1FC, race=0xAD, races=(5, 6), flags=0x1A4, flags2=0x1A5,
                      size=0x131, hd=0x138, mr=0x1EB, has=(0x105, 0xA7), near=(0x105, 0xC0), check=(0xA77, 0x4DF),
                      cur=0x9308, spells=0x5789, stride=16, n=0x7F, pascal=True),
    'knights': dict(file='GAME.OVR', prog=('knights', 'START.EXE'), ds=0xBCE0, fn=0x22CE4, seg=0x1ED0F,
                    side=0xC9, hpmax=0x28, hp=0xD6, race=0x20, races=(6, 7), flags=0x7A, flags2=0x7B,
                    size=0x29, hd=0x30, mr=0xC1, has=(0xCF, 0xAC), near=(0xCF, 0xC5), check=None,
                    cur=0xA2C2, spells=0x6A7F, stride=16, n=0x72, pascal=True),
    # (the Dark Queen: Borland C - the arguments pushed last first, the caller
    # pops; its code w0's segment 0x21E4 at file 0x24D60, DS:0 at 0x63A20; the
    # side and race words; 138 spells)
    'queen': dict(file='DQK.EXE', prog=None, ds=0x63A20, fn=0x2902C, seg=0x24D60,
                  side=0x66, hpmax=0x89, hp=0x198, race=0x58, races=(6, 7), flags=0xC7, flags2=0xC8,
                  size=0x8A, hd=0x91, mr=0xCC, has=(0x77E, 0x1734), near=(0x20CB, 0x980), check=None,
                  cur=None, spells=0x27E9, stride=22, n=138, pascal=False, words=('side', 'race')),
}


def effects_of(st, which):
    return st['teff'] if which == 0 else st['ceff']


def run_game(game, out):
    g = GAMES[game]
    path = gb.find(game, g['file'])
    code = open(path, 'rb').read()
    img = gb.image(*g['prog']) if g['prog'] else code
    r = emu16.Routine(code, g['seg'], img, g['ds'], pascal=g['pascal'])
    asked = set()

    def ptr_which(seg):
        return 0 if seg == emu16.R1 else 1

    def h_has(e):
        # (record far, effect, var far): al - it has it. Pascal: the var on
        # top; C: the record on top
        w = e.args(5)
        eff, seg = (w[2] & 0xFF, w[4]) if g['pascal'] else (w[2] & 0xFF, w[1])
        asked.add(eff)
        return 1 if eff in effects_of(e.state, ptr_which(seg)) else 0

    def h_near(e):
        # (record far, range, own): how many
        return e.state['near']

    r.helper(*g['has'], h_has, 10)
    r.helper(*g['near'], h_near, 8)
    if g['check']:
        r.helper(*g['check'], lambda e: 0, 0)
    if g['cur']:
        r.poke_ds(g['cur'], struct.pack('<HH', 0, emu16.R2))

    def record(st, which):
        rec = bytearray(0x400)
        if which == 0:
            rec[g['side']] = 1 if st['side'] else 0
            rec[g['hpmax']], rec[g['hp']] = (20, 10) if st['hurt'] else (20, 20)
            rec[g['race']] = g['races'][0] if st['person'] else g['races'][1]
            # (the Queen's side and race are words: their high bytes stay 0)
            rec[g['flags']] = st['flags']
            rec[g['flags2']] = st['flags2']
            rec[g['size']] = st['size']
            rec[g['hd']] = st['hd']
            rec[g['mr']] = st['mr']
        return rec

    def run(spell, st):
        r.state = st
        r.record(0, record(st, 0))
        r.record(1, record(st, 1))
        # (side asked 1, the spell, the target far) - Pascal: pushed target
        # first; C (target far, spell, side): side pushed first
        ax = r.call(g['fn'], [emu16.R1, 0, spell, 1] if g['pascal'] else [1, spell, emu16.R1, 0])
        return struct.unpack('<b', bytes([ax & 0xFF]))[0]

    base = dict(side=1, hurt=0, person=1, flags=0, flags2=0, size=1, hd=4, mr=0, near=0, teff=set(), ceff=set())
    rnd = random.Random(20)
    lines = []
    for s in range(1, g['n'] + 1):
        asked.clear()
        run(s, base)
        for e in list(asked):
            run(s, dict(base, teff={e}))
            run(s, dict(base, ceff={e}))
        es = sorted(asked)
        states = [dict(base)]
        for _ in range(160):
            st = dict(side=rnd.random() < 0.85, hurt=rnd.random() < 0.5, person=rnd.random() < 0.5,
                      flags=rnd.choice([0, 2, 0x10, 0x12, 0x20]), flags2=rnd.choice([0, 2, 8, 0x10, 0x1A]),
                      size=rnd.choice([0, 1, 2]), hd=rnd.choice([2, 4, 5, 6, 8]), mr=rnd.choice([0, 0x32, 0x33]),
                      near=rnd.choice([0, 0, 1]),
                      teff={e for e in es if rnd.random() < 0.3}, ceff={e for e in es if rnd.random() < 0.3})
            states.append(st)
        for st in states:
            res = run(s, st)
            fx = lambda v: ','.join('%x' % e for e in sorted(v)) or '-'
            lines.append('use %d %d %d %d %d %d %d %d %d %d %s %s %d' % (
                s, st['side'], st['hurt'], st['person'], st['flags'], st['flags2'], st['size'], st['hd'], st['mr'],
                st['near'], fx(st['teff']), fx(st['ceff']), res))
    # the spell rows the routines read (Pascal games: a lead byte, then
    # Curse's 16; the Queen's 22): level, aim, effect, castable in fights,
    # priority, the side byte (0: its own)
    for s in range(1, g['n'] + 1):
        if g['pascal']:
            row = lambda k: img[g['ds'] + g['spells'] + s * 16 + 1 + k]
            v = (row(1), row(6), row(10), row(11), row(13), row(14))
        else:
            at = g['ds'] + g['spells'] + s * 22
            w16 = lambda k: img[at + k] | img[at + k + 1] << 8
            v = (img[at + 2], img[at + 7], img[at + 0xE], w16(0xF), img[at + 0x12], w16(0x13))
        lines.append('row %d %d %d %d %d %d %d' % ((s,) + tuple(v)))
    with open(out, 'w') as f:
        f.write('# aicast oracle (%s) - tools/devkit/oracles/aicast.py\n' % game)
        f.write('\n'.join(lines) + '\n')
    print('%s: %d lines -> %s' % (game, len(lines), out))


if __name__ == '__main__':
    os.makedirs(gb.OUT, exist_ok=True)
    for game in sys.argv[1:] or [g for g in ALL if g in GAMES]:
        run_game(game, os.path.join(gb.OUT, 'oracle_aicast_%s.txt' % game))
