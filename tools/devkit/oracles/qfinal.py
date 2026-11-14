#!/usr/bin/env python3
# qfinal.py <DQK.EXE>: the Dark Queen's ending (PROGRAM 1 -> overlay 4's routine at file 0x3878B) run in
# Unicorn with its helpers stubbed, printing what it does in order - the source of the engine's
# kQueenFinal table (profile.cpp). Lines:
#   load E        FINAL.TLB's entry E ("final1" + the id: the picture's library) - the parts below are its
#   frame A B     part A drawn, then part B when B > 0 (DQK.EXE 0x3869B: part 1 = the background)
#   wait N        N x 36 hundredths of a second (0x38683: the frame shown, then 0x383CC's wait - a key ends
#                 the wait early)
#   show / palette / free / sound S / other SEG:OFF
# --table: the engine's table (the frames' first part is always 1, the background).
# Run with the unicorn venv python (emu16.py). Nothing of the game is kept here.
import struct, sys
sys.path.insert(0, __import__('os').path.dirname(__file__))
from emu16 import Routine, CS
from unicorn.x86_const import UC_X86_REG_SP, UC_X86_REG_SS

exe = open(sys.argv[1], 'rb').read()
DS_FILE = 0x63A20
BASE = 0x30000                       # (the overlay's code: its near calls at these offsets)
r = Routine(exe, BASE, exe, DS_FILE, pascal=False)
out = []


def stub(lin, fn):
    r.mu.mem_write(lin, b'\xcd\x30\xcb')
    r.handlers[lin] = fn


def a(n):
    return r.args(n)


def frame(_):
    first, second = a(2)
    out.append('frame %d %d' % (first, struct.unpack('<h', struct.pack('<H', second))[0]))
    return 0


def wait(_):
    out.append('wait %d' % a(1)[0])
    return 0


def load(_):
    w = a(7)                         # name far (2 words), id byte, 0, 0, far pointer
    out.append('load %d' % (w[2] & 0xFF))
    return 0


stub(CS * 16 + 0x869B, frame)
stub(CS * 16 + 0x8683, wait)
stub(0x9A4 * 16 + 0x1DC, load)
stub(0x9A4 * 16 + 0x630, lambda _: out.append('show') or 0)
stub(0x9A4 * 16 + 0xB8C, lambda _: out.append('palette') or 0)
stub(0x9A4 * 16 + 0x9, lambda _: out.append('free') or 0)
stub(0x1023 * 16 + 0x6, lambda _: out.append('sound %X' % a(1)[0]) or 0)
for seg, off in ((0x1FC4, 0x200), (0x9A4, 0x5EE), (0x10B2, 0x2D3), (0xD7, 0x141), (0x9A4, 0x1D1), (0x12B7, 0x14BC),
                 (0x12B7, 0x14B4), (0x12B7, 0x2122), (0x12B7, 0xC21)):
    stub(seg * 16 + off, (lambda s, o: (lambda _: out.append('other %04X:%04X %s' % (s, o, a(1))) or 0))(seg, off))
r.call(0x3878B, [])
if '--table' in sys.argv:
    # the engine's kQueenFinal (profile.cpp): {'L', entry} {'F', part} {'W', waits}
    items = []
    for l in out:
        w = l.split()
        if w[0] == 'load': items.append("{'L', %s}" % w[1])
        elif w[0] == 'frame':
            assert w[1] == '1'
            items.append("{'F', %s}" % w[2])
        elif w[0] == 'wait': items.append("{'W', %s}" % w[1])
    line = '    '
    for it in items:
        if len(line) + len(it) + 2 > 116:
            print(line.rstrip())
            line = '    '
        line += it + ', '
    print(line.rstrip())
else:
    print('\n'.join(out))
