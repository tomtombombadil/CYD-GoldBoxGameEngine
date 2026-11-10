#!/usr/bin/env python3
# guard.py [game ...]   (all nine games; none: all nine - the Dark Queen's from DQK.EXE, queen())
# The computer's guard in each DAX game (rules round 18), read from its
# GAME.OVR's code - for sweep's behave rules4 ONLY=guard0:
#   - the AI turn's loop (turnfind.py's call of the finder with range 0xFF):
#     where "found" fails AND where the moves test (combat +3, signed, jng)
#     fails - both must reach the same near call, the guard routine;
#   - the guard routine: a screen helper, then two tests on the fighter - A
#     (a loop over a DS byte list, each asked of the has-effect routine) and
#     B (the readied weapon's type: its range byte > 1, and - B2 - its flags
#     & 0x14 = 0x14) - and the moves byte; any of A, B or moves 0 -> the
#     turn just ends (the routine that zeroes moves / guard), else the guard
#     (the same plus the guard byte set).
# Output oracle_guard_<game>.txt, one line:
#   guard loop <hex> routine <hex> moves0 <0|1> eff <hex ...> missile <range|range+ammo> end <hex> set <hex>
import os, re, struct, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import x86

ALL = ['poolrad', 'curse', 'secret', 'darkness', 'champions', 'knights', 'gateway', 'treasures', 'queen']
# (the program and its data segment's place in the unpacked image - as the
# other oracles: pool.py, classes.py, secret.py, gateway.py, champions.py,
# deathknights.py, darkness.py)
PROG = {'poolrad': ('START.EXE', 0xC7C0), 'curse': ('START.EXE', 0xABE0), 'secret': ('START.EXE', 0xDE20),
        'gateway': ('GAME.EXE', 0x4C70), 'champions': ('START.EXE', 0xBC60), 'knights': ('START.EXE', 0xBCE0),
        'darkness': ('GAME.EXE', 0x9E70), 'treasures': ('GAME.EXE', 0xC3E0)}

def queen():
    """The Dark Queen (DQK.EXE, file offsets; not packed, DS:0 at file
    0x63A20 - queen.py): its AI loop (the finder pushed 0, 0, 1, 0xFF then
    the record - C order), the moves word test (cmp word [es:bx+3],0 / jng)
    and the guard routine (the same three tests: a DS list's effects, the
    readied weapon's range byte over 1, the moves word); and its finder:
    as many tries as the list holds ([bp-0xd] = the count), a roll before
    the tries, the taunted effect asked with the try count 0x13 and the list
    builder's swap of the kender to the list's front.
    Output oracle_guard_queen.txt: the guard line and a qfinder line."""
    import gb
    exe = gb.find('queen', 'DQK.EXE')
    x = open(exe, 'rb').read()
    here = os.path.dirname(os.path.abspath(__file__))

    def where(seg, off):
        out = subprocess.run([sys.executable, '-I', os.path.join(here, '..', 'tools', 'dqk_addr.py'), exe,
                              '%x:%x' % (seg, off)], capture_output=True, text=True, check=True).stdout
        m = re.search(r'from inside overlay w0: file 0x([0-9A-F]+)', out) or re.search(r'-> file 0x([0-9A-F]+)', out)
        return int(m.group(1), 16)

    m = re.search(rb'\xB0\x00\x50\xB0\x00\x50\xB0\x01\x50\xB0\xFF\x50\xFF\x76\x08\xFF\x76\x06\x9A(..)(..)\x83\xC4\x0C\x0A\xC0\x74(.)', x, re.S)
    loop = m.start()
    finder = where(struct.unpack('<H', m.group(2))[0], struct.unpack('<H', m.group(1))[0])
    nf = m.end() + struct.unpack('<b', m.group(3))[0]
    # the guard: push [bp+8] / push [bp+6] / push cs / call near
    def near(a):
        if x[a:a + 7] != bytes.fromhex('FF7608FF76060E') or x[a + 7] != 0xE8:
            return None
        return a + 10 + struct.unpack_from('<h', x, a + 8)[0]
    g1 = near(nf)
    mv = x.find(bytes.fromhex('26837F03007E'), m.end(), m.end() + 0x30)
    g2 = near(mv + 7 + struct.unpack_from('<b', x, mv + 6)[0]) if mv >= 0 else None
    moves0 = int(g1 is not None and g1 == g2)
    gr = g1
    body = x[gr:gr + 0x64]
    calls = [mm for mm in re.finditer(rb'\x9A(..)(..)', body, re.S)]
    A = where(struct.unpack('<H', calls[1].group(2))[0], struct.unpack('<H', calls[1].group(1))[0])
    Bt = where(struct.unpack('<H', calls[2].group(2))[0], struct.unpack('<H', calls[2].group(1))[0])
    mtest = re.search(rb'\x26\x83\x7F\x03\x00\x75', body) is not None
    # A: push word [bx+T] ... cmp byte [bp-5],N / jc
    a = x[A:A + 0x50]
    t = struct.unpack('<H', re.search(rb'\xFF\xB7(..)', a, re.S).group(1))[0]
    n = re.search(rb'\x80\x7E\xFB(.)\x72', a, re.S).group(1)[0]
    eff = [x[0x63A20 + t + i] for i in range(n)]
    bb = x[Bt:Bt + 0x40]
    rng = re.search(rb'\x26\x80\x7F.\x01\x76', bb, re.S) is not None
    line = 'guard loop 0x%X routine 0x%X moves0 %d movetest %d eff %s missile %s' % (
        loop, gr, moves0, int(mtest), ' '.join('0x%02X' % e for e in eff), 'range' if rng else 'unknown')
    # the finder
    f = x[finder:finder + 0x260]
    tries_list = re.search(rb'\x8A\x46\xF2\x88\x46\xF3', f) is not None            # mov al,[bp-0xe] / mov [bp-0xd],al
    first = re.search(rb'\x80\x7E\xF2\x00\x76.\xFF\x76\xF2\xB0\x01\x50\x9A', f, re.S) is not None
    tn = re.search(rb'\xB0(.)\x50\xFF\x76\x08\xFF\x76\x06\x9A....\x83\xC4\x0A\x0A\xC0\x74.\xA0..\xB4\x00.{12,40}\x80\x7E\xF3(.)', f, re.S)
    lb = re.search(rb'\x9A(..)(..)\x83\xC4\x08\x88\x46\xF2', f, re.S)
    lst = where(struct.unpack('<H', lb.group(2))[0], struct.unpack('<H', lb.group(1))[0])
    swap = re.search(rb'\xB0' + bytes([tn.group(1)[0]]) + rb'\x50', x[lst:lst + 0x140]) is not None if tn else False
    # the turn's first: the same call, only when the target is empty
    # (or ax,[es:bx+0xd] / jnz over it), before the loop
    ea = re.search(rb'\x26\x0B\x47\x0D\x75.\xB0\x00\x50\xB0\x00\x50\xB0\x01\x50\xB0\xFF\x50\xFF\x76\x08\xFF\x76\x06\x9A' +
                   re.escape(m.group(1) + m.group(2)), x[loop - 0x300:loop], re.S)
    early = '0x%X' % (loop - 0x300 + ea.start() + 6) if ea else 'none'
    q = 'qfinder 0x%X tries %s firstroll %d taunt 0x%02X at 0x%02X swap %d early %s' % (
        finder, 'list' if tries_list else 'unknown', int(first), tn.group(1)[0] if tn else 0, tn.group(2)[0] if tn else 0,
        int(swap), early)
    open(os.path.join(gb.OUT, 'oracle_guard_queen.txt'), 'w').write('# oracle_guard_queen\n%s\n%s\n' % (line, q))
    print('queen', line)
    print('queen', q)


if len(sys.argv) < 2 or sys.argv[1] != '--one':
    for g in sys.argv[1:] or ALL:
        subprocess.check_call([sys.executable, '-I', os.path.abspath(__file__), '--one', g])
    sys.exit(0)
import gb
game = sys.argv[2]
if game == 'queen':
    queen()
    sys.exit(0)
exe, B = PROG[game]
im = gb.image(game, exe)
path = gb.find(game, 'GAME.OVR')
d = open(path, 'rb').read()


IMGPATH = os.path.join(gb.PROG, '%s_%s.bin' % (game, exe.split('.')[0].lower()))


def resolve(p, where=False):
    """a far call at GAME.OVR p -> (file, offset): an overlay stub's entry in
    GAME.OVR, else a resident segment in the unpacked image (PoolRad)"""
    off, seg = struct.unpack_from('<HH', d, p + 1)
    s = seg * 16
    if im[s:s + 2] != b'\xcd\x3f':
        r = (IMGPATH, s + off) if im[s + off:s + off + 3] == b'\x55\x89\xe5' else None
    else:
        r = (path, struct.unpack_from('<I', im, s + 4)[0] + struct.unpack_from('<H', im, s + off + 2)[0])
    return r if where or r is None else r[1]


def near_call_at(a):
    """push word [bp+8] / push word [bp+6] / push cs / call near X at a -> X"""
    if d[a:a + 7] != bytes.fromhex('FF7608FF76060E') or d[a + 7] != 0xE8:
        return None
    return a + 10 + struct.unpack_from('<h', d, a + 8)[0]


def jtarget(a):
    """a short / near conditional jump's target"""
    if 0x70 <= d[a] <= 0x7F:
        return a + 2 + struct.unpack_from('<b', d, a + 1)[0]
    raise ValueError


loop = None
for m in re.finditer(re.escape(bytes.fromhex('B0FF50B00150B00050')), d):
    p = m.end()
    if d[p:p + 3] == bytes.fromhex('B00050'):
        p += 3
    if d[p] != 0x9A or resolve(p) is None or d[p + 5:p + 8] != bytes.fromhex('08C074'):
        continue
    loop = (m.start(), p)
    break
if loop is None:
    raise SystemExit('%s: no AI loop' % game)
start, call = loop
nf = jtarget(call + 7)                       # finder found nothing
g1 = near_call_at(nf)
# the moves test: "cmp byte [es:di+3],0 / jng" before (Darkness-type) or after the call
mv = None
for k in range(start - 0x30, call + 0x30):
    if d[k:k + 6] == bytes.fromhex('26807D03007E'):
        mv = k + 5
        break
g2 = near_call_at(jtarget(mv)) if mv is not None else None
moves0 = int(g1 is not None and g1 == g2)
gr = g1
code = x86.Code(path, gr, 0x80, base=0)
far = [a for a, b_, t in code.ls if t.startswith('call 0x') and ':' in t]
ret = next(a for a, _, t in code.ls if t.startswith('retf'))
far = [a for a in far if a < ret and not code.text(code.after(a, -1)[0]).startswith('mov ax,')]   # (not the stack check)
# helper (no arguments), A, B, end, set - in that order
if len(far) != 5:
    raise SystemExit('%s: guard routine at 0x%X has %d far calls' % (game, gr, len(far)))
A, Bt, end, sett = (resolve(a, True) for a in far[1:])
# its moves test (cmp byte [es:di+3],0 / jnz -> set)
mtest = re.search(rb'\x26\x80\x7D\x03\x00\x75', d[gr:ret]) is not None

# A: the list - mov al,[di+T] ... cmp byte [bp-2],N
ca = x86.Code(A[0], A[1], 0x60, base=0)
ta = next(x86.num(re.search(r'\[di\+(0x[0-9a-f]+)\]', t).group(1)) for a, _, t in ca.ls if re.match(r'mov al,\[di\+0x', t))
n = next(ca.imm(a) for a, _, t in ca.ls if re.match(r'cmp byte \[bp-0x2\],', t))
first = next(ca.imm(a) for a, _, t in ca.ls if re.match(r'mov byte \[bp-0x2\],', t) and ca.imm(a) <= 1)
eff = [im[B + ta + i] for i in range(first, n + 1)]

# B: the readied weapon's type table - cmp byte [di+R],0x1 / jna (range), and
# an "and al,0x14 / cmp al,0x14" (ammunition) in it or in a near call it makes
cb = x86.Code(Bt[0], Bt[1], 0x60, base=0)
rng = any(re.match(r'cmp byte \[di[+-]0x[0-9a-f]+\],0x1$', t) for _, _, t in cb.ls)
bret = next(a for a, _, t in cb.ls if t.startswith('retf'))
ammo = any(t == 'and al,0x14' for a, _, t in cb.ls if a < bret)
if not rng:
    # (B calls the range test first: call near X / or al,al / jz)
    for a, _, t in cb.ls:
        mm = re.match(r'call (0x[0-9a-f]+)$', t)
        if mm and a < bret:
            sub = x86.Code(Bt[0], x86.num(mm.group(1)), 0x40, base=0)
            rng = any(re.match(r'cmp byte \[di[+-]0x[0-9a-f]+\],0x1$', t2) for _, _, t2 in sub.ls)
missile = ('range+ammo' if ammo else 'range') if rng else 'unknown'
line = 'guard loop 0x%X routine 0x%X moves0 %d movetest %d eff %s missile %s end %s set %s' % (
    start, gr, moves0, int(mtest), ' '.join('0x%02X' % e for e in eff), missile,
    ('img:' if end[0] != path else '') + '0x%X' % end[1], ('img:' if sett[0] != path else '') + '0x%X' % sett[1])
open(os.path.join(gb.OUT, 'oracle_guard_%s.txt' % game), 'w').write('# oracle_guard_%s\n%s\n' % (game, line))
print(game, line)
