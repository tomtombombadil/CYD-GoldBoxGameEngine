#!/usr/bin/env python3
# yell.py [game ...]   (champions knights queen; none: all three)
# The Krynn games' Yell (rules round 19), read from each original's code -
# for sweep's behave rules4 ONLY=yell. The Yell's routine is found by its
# shape: the taunted effect pushed (mov al,E / push ax - Champions / Death
# Knights 0x5B in GAME.OVR, the Dark Queen 0x8C in DQK.EXE) shortly after
# the tests on each fighter of the other side:
#   okay (Champions +0x188 / Death Knights +0xC7 byte 0, the Dark Queen
#   +0x64 word 0), Intelligence in use over 3 (cmp byte [..+0x13 / 0x7B],0x3
#   / jna), the side byte / word different, a far call whose result skips
#   the fighter (or al,al / jnz) - the held list's routine - then the save
#   against spells (4, 0 pushed).
# The held routine is compared with the guard routine's list test (the
# oracle_guard_<game>.txt "routine <hex>" for the DAX games; the Dark Queen's
# 0x2EDFC): the same far call there means the same list (its "eff" bytes).
# Output oracle_yell_<game>.txt:
#   yell <hex> taunted <hex> int 3 held <seg:off> guard_same <0|1> save 4 order held-save
import os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gb
import x86

ALL = ['champions', 'knights', 'queen']


def scan(path, eff, di):
    b = open(path, 'rb').read()
    pat = bytes([0xB0, eff, 0x50])
    for m in re.finditer(re.escape(pat), b):
        st = m.start() - 0x90
        c = x86.Code(path, st, 0xA0, base=0)
        ls = [(a, t) for a, _, t in c.ls]
        intl = [a for a, t in ls if re.match(r'cmp byte \[es:%s\+0x(13|7b)\],0x3$' % di, t)]
        if not intl:
            continue
        held = None
        save = None
        for k, (a, t) in enumerate(ls):
            if a < intl[0]:
                continue
            mm = re.match(r'call 0x(\w+):0x(\w+)$', t)
            if mm and held is None:
                j = k + 1
                while j < len(ls) and ls[j][1] == 'pop cx':      # (C: the caller pops)
                    j += 1
                if j + 1 < len(ls) and ls[j][1] == 'or al,al' and ls[j + 1][1].startswith('jnz'):
                    held = (mm.group(1), mm.group(2), a)
                    continue
            if held and save is None and re.match(r'mov (al|ax),0x4$', t):
                save = a
        if held and save:
            return st, intl[0], held, save
    return None


out_dir = gb.OUT
for g in sys.argv[1:] or ALL:
    if g == 'queen':
        path = gb.find('queen', 'DQK.EXE')
        r = scan(path, 0x8C, 'bx')
        guard_at = 0x2EDFC
    else:
        path = gb.find(g, 'GAME.OVR')
        r = scan(path, 0x5B, 'di')
        gl = open(os.path.join(out_dir, 'oracle_guard_%s.txt' % g)).read()
        guard_at = int(re.search(r'routine 0x(\w+)', gl).group(1), 16)
    if not r:
        raise SystemExit('%s: no Yell found' % g)
    st, intl, (seg, off, hat), save = r
    gc = x86.Code(path, guard_at, 0x60, base=0)
    same = any(t == 'call 0x%s:0x%s' % (seg, off) for _, _, t in gc.ls)
    line = 'yell %x taunted %x int 3 held %s:%s guard_same %d save 4 order %s' % (
        intl, 0x8C if g == 'queen' else 0x5B, seg, off, 1 if same else 0, 'held-save' if hat < save else 'save-held')
    open(os.path.join(out_dir, 'oracle_yell_%s.txt' % g), 'w').write('# oracle_yell_%s\n%s\n' % (g, line))
    print(g, line)
