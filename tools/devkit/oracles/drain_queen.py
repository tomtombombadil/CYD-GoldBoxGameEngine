#!/usr/bin/env python3
# drain_queen.py [restore] [in] [out]: the Dark Queen's level drain - or with
# "restore" its Restoration (overlay w10, file 0x5838C: the target the far
# pointer DS 0xC5E3, its own Constitution figure resident 0x77E:0x20CE run,
# "able to train" 0x77E:0x1FE5 not; the cases from part `restore`) - as the ORIGINAL does
# it - DQK.EXE's own drain routine (overlay w9, 0x2B3A:0x17DD = file 0x4F66D;
# its effects 0x65 / 0x66 call it with one and two levels) RUN in Unicorn on
# every case sweep's `behave levels` part `drain` wrote for the Dark Queen
# (DRAINMAKE; the same lines as drain_emu.py's), with the code it calls: its
# experience routine and its hit point routine (overlay w11, through the
# resident stubs 0x1D99:0x0F6D / 0x143B - their loader calls skipped, w11
# loaded at its region segment), the class's experience row (resident
# 0x77E:0x1F39), Constitution's figure, the most hit points, the C runtime's
# long arithmetic. Not run: the words, the delay, the local redraw, the class
# values (0x1D99:0x13AF - the check leaves out what they set) and the fall
# (0x48F:0xD4: the status it is given noted); the dice (0x48F:0x1968, n dice
# of s sides) all at their highest or lowest by the case's dice, as the
# engine's die hook. Writes "res id status hex" / "fail id why" lines, read by
# DRAINCHK (behave_levels.inc). Needs unicorn (/tmp/claude-0/uvenv/bin/python).
# Memory: the file from 0x2400 at linear 0 (resident code at seg x 16 + off,
# DS = file 0x63A20 = segment 0x6162), w11's code at its region 0x2B3A, w9's
# (the drain) at 0x8000 so it doesn't overlap w11, records at 0x9000:0, the
# stack 0xA000, the sentinel return 0xB000:0. Nothing of the game is kept here.
import os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gb
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INTR
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_DX, UC_X86_REG_SP, UC_X86_REG_IP, UC_X86_REG_CS,
                               UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS, UC_X86_REG_BP)

RES = 0x2400                        # resident code: file 0x2400 + seg x 16 + off
DS_FILE = 0x63A20
OVT_END = 0x20A83                   # the overlay table's 18-byte rows end here
W9_CS = 0x8000
REC, SS, RET = 0x9000, 0xA000, 0xB000
DRAIN_OFF = 0x17DD                  # w9 0x2B3A:0x17DD
REDRAW_OFF = 0x158D                 # w9's local redraw (file 0x4F41D), "push cs / call"
STUB_E, STUB_HP, STUB_VALUES = (0x1D99, 0x0F6D), (0x1D99, 0x143B), (0x1D99, 0x13AF)
WORDS, DELAY, DICE, FALL = (0x77E, 0x156E), (0xA62, 0x949), (0x48F, 0x1968), (0x48F, 0xD4)
RESTORE_FILE, RESTORE_TARGET, TRAIN = 0x5838C, 0xC5E3, (0x77E, 0x1FE5)


def overlay_rows(d):
    rows, p = {}, OVT_END - 18
    while p > 0:
        r = struct.unpack_from('<9H', d, p)
        if r[3] != 0x0DCB:
            break
        rows[r[0]] = r
        p -= 18
    return rows


def overlay(d, rows, w):
    r = rows[w + 1]                 # (w >= 1: the row labelled w + 1)
    return r[2], (r[4] * 16 + r[7] * 4 + 15) & ~15


def run(fin, fout, mode='drain'):
    d = open(gb.find('queen', 'DQK.EXE'), 'rb').read()
    rows = overlay_rows(d)
    reg9, code9 = overlay(d, rows, 9)
    reg11, code11 = overlay(d, rows, 11)
    reg10, code10 = overlay(d, rows, 10)
    mu = Uc(UC_ARCH_X86, UC_MODE_16)
    mu.mem_map(0, 0x100000)
    image = d[RES:]
    mu.mem_write(0, image[:0x100000])
    mu.mem_write(reg11 * 16, d[code11:code11 + 0x10000])
    # w9's link segment 0x2B3A at W9_CS (the drain's own segment: its near
    # calls and its "push cs" stay inside it)
    w9 = code9 + (0x2B3A - reg9) * 16
    if mode == 'restore':           # (w10 from its region's start at W9_CS: the routine's offset its file place less it)
        mu.mem_write(W9_CS * 16, d[code10:code10 + 0x10000])
        start = RESTORE_FILE - code10
    else:
        mu.mem_write(W9_CS * 16, d[w9:w9 + 0x10000])
        start = DRAIN_OFF
    mu.mem_write(RET * 16, b'\xf4')
    # the stubs into w11: their loader call skipped (3 NOPs), the far jump runs
    for seg, off in (STUB_E, STUB_HP):
        lin = seg * 16 + off
        assert bytes(mu.mem_read(lin, 4))[0] == 0xE8 and bytes(mu.mem_read(lin + 3, 1))[0] == 0xEA
        mu.mem_write(lin, b'\x90\x90\x90')
    handlers = {}
    st = {}
    dice_hi = {'hi': 1}

    def helper(seg, off, fn):
        lin = seg * 16 + off
        mu.mem_write(lin, b'\xcd\x30\xcb')      # (C: a plain retf, the caller pops)
        handlers[lin] = fn

    def args(n):
        sp = mu.reg_read(UC_X86_REG_SP)
        return list(struct.unpack('<%dH' % n, mu.mem_read(SS * 16 + sp + 4, 2 * n)))

    def fall():
        st['status'] = args(3)[2] & 0xFF
        return 0

    def dice():
        n, s = args(2)
        n, s = n & 0xFF, s & 0xFF
        return n * (s if dice_hi['hi'] else 1)

    for a in (WORDS, DELAY, STUB_VALUES):
        helper(a[0], a[1], lambda: 0)
    if mode == 'restore':
        helper(TRAIN[0], TRAIN[1], lambda: 0)
    else:
        helper(W9_CS, REDRAW_OFF, lambda: 0)
    helper(DICE[0], DICE[1], dice)
    helper(FALL[0], FALL[1], fall)

    def intr(mu_, intno, _):
        lin = mu.reg_read(UC_X86_REG_CS) * 16 + mu.reg_read(UC_X86_REG_IP) - 2
        h = handlers.get(lin) if intno == 0x30 else None
        if h is None:
            raise RuntimeError('interrupt %X at %04X:%04X' % (intno, mu.reg_read(UC_X86_REG_CS),
                                                              mu.reg_read(UC_X86_REG_IP) - 2))
        mu.reg_write(UC_X86_REG_AX, h() & 0xFFFF)
    mu.hook_add(UC_HOOK_INTR, intr)
    ds = (DS_FILE - RES) // 16
    out = []
    for line in open(fin):
        p = line.split()
        if not p or p[0] != 'case':
            continue
        cid, n, hi, rec = int(p[1]), int(p[2]), int(p[3]), bytes.fromhex(p[4])
        dice_hi['hi'] = hi
        st.clear()
        mu.mem_write(REC * 16, rec.ljust(0x400, b'\0'))
        sp = 0xFFF0
        mu.mem_write(DS_FILE - RES + RESTORE_TARGET, struct.pack('<HH', 0, REC))
        for w in ((n, REC, 0) if mode == 'drain' else ()):   # (C: the last argument first - n, then the record's far pointer)
            sp -= 2
            mu.mem_write(SS * 16 + sp, struct.pack('<H', w))
        sp -= 4
        mu.mem_write(SS * 16 + sp, struct.pack('<HH', 0, RET))
        for r_, v in ((UC_X86_REG_SS, SS), (UC_X86_REG_SP, sp), (UC_X86_REG_DS, ds), (UC_X86_REG_ES, ds),
                      (UC_X86_REG_BP, 0), (UC_X86_REG_CS, W9_CS), (UC_X86_REG_IP, start)):
            mu.reg_write(r_, v)
        try:
            mu.emu_start(W9_CS * 16 + start, RET * 16, count=5000000)
        except Exception as e:      # (a run the emulator can't finish: said, not taken as a result)
            out.append('fail %d %s' % (cid, str(e).replace(' ', '_')[:80]))
            continue
        after = bytes(mu.mem_read(REC * 16, len(rec)))
        out.append('res %d %d %s' % (cid, st.get('status', 0), after.hex()))
    open(fout, 'w').write('\n'.join(out) + '\n')
    print('queen %s: %d cases, %d failed' % (mode, len(out), sum(1 for l in out if l.startswith('fail'))))


if __name__ == '__main__':
    o = gb.OUT
    a = sys.argv[1:]
    mode = 'restore' if a and a[0] == 'restore' else 'drain'
    if mode == 'restore':
        a = a[1:]
    run(a[0] if a else os.path.join(o, '%s_in_queen.txt' % mode),
        a[1] if len(a) > 1 else os.path.join(o, '%s_out_queen.txt' % mode), mode)
