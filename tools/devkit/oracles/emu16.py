#!/usr/bin/env python3
# emu16.py - runs ONE routine of an original's 16-bit code in Unicorn (rules
# round 20: the computer's spell judges). Needs the unicorn module: a venv on
# Claude's side (python3 -m venv /tmp/claude-0/uvenv; /tmp/claude-0/uvenv/bin/
# python -m pip install unicorn) - the oracles that use it run under that
# python. Nothing of the games is kept here: the code and data come from the
# player's files at run time.
#
# Memory: the routine's code segment (a file stretch) at 0x1000:0, the data
# segment (64 KB of the program image from its DS:0) at 0x3000:0, records at
# 0x5000:0 / 0x6000:0 (a far pointer each), the stack 0x7000:FFF0. Far calls
# out of the routine are not run: each helper's address (seg:off, as the
# routine calls it) gets "int 0x30 / retf n" and the interrupt hook gives it a
# Python handler - it reads the pushed arguments (args(n): the words above
# the far return) and returns AX; retf n pops them (Pascal: argbytes), C's
# plain retf leaves them for the caller.
import struct

try:
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INTR, UcError
    from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_SP, UC_X86_REG_IP, UC_X86_REG_CS, UC_X86_REG_DS,
                                   UC_X86_REG_ES, UC_X86_REG_SS, UC_X86_REG_BP)
except ImportError:
    raise SystemExit('emu16: the unicorn module is missing - run this oracle with a venv python that has it '
                     '(/tmp/claude-0/uvenv/bin/python; python -m pip install unicorn)')

CS, DS, R1, R2, SS = 0x1000, 0x3000, 0x5000, 0x6000, 0x7000
RET = 0x0800                                    # the sentinel far return (0x0800:0000)


class Routine:
    def __init__(self, code, code_base, data, data_base, pascal=True):
        """code: the file's bytes; code_base: the file offset of the code
        segment's offset 0; data: the program image; data_base: DS:0 in it."""
        self.mu = Uc(UC_ARCH_X86, UC_MODE_16)
        self.mu.mem_map(0, 0x100000)
        seg = code[code_base:code_base + 0x10000]
        self.mu.mem_write(CS * 16, seg)
        self.mu.mem_write(DS * 16, data[data_base:data_base + 0x10000].ljust(0x10000, b'\0'))
        self.mu.mem_write(RET * 16, b'\xf4')
        self.base = code_base
        self.pascal = pascal
        self.handlers = {}
        self.calls = []
        self.mu.hook_add(UC_HOOK_INTR, self._intr)

    def helper(self, seg, off, fn, argbytes=0):
        """fn(routine) -> AX, for the helper the routine calls at seg:off."""
        lin = seg * 16 + off
        for lo, hi in ((CS * 16, CS * 16 + 0x10000), (DS * 16, DS * 16 + 0x10000), (R1 * 16, SS * 16 + 0x10000),
                       (RET * 16, RET * 16 + 16)):
            if lo <= lin < hi:
                raise RuntimeError('emu16: helper %04X:%04X lands on the routine\'s memory' % (seg, off))
        stub = b'\xcd\x30' + (b'\xca' + struct.pack('<H', argbytes) if self.pascal and argbytes else b'\xcb')
        self.mu.mem_write(lin, stub)
        self.handlers[lin] = fn

    def record(self, which, data):
        self.mu.mem_write((R1 if which == 0 else R2) * 16, bytes(data).ljust(0x400, b'\0'))

    def poke_ds(self, a, data):
        self.mu.mem_write(DS * 16 + a, bytes(data))

    def u8_ds(self, a):
        return self.mu.mem_read(DS * 16 + a, 1)[0]

    def args(self, n):
        """The n words above the far return, the last pushed first (inside a helper)."""
        sp = self.mu.reg_read(UC_X86_REG_SP)
        return list(struct.unpack('<%dH' % n, self.mu.mem_read(SS * 16 + sp + 4, 2 * n)))

    def _intr(self, mu, intno, _):
        # (int 0x30 is not delivered: the helper's own far return is on top)
        lin = mu.reg_read(UC_X86_REG_CS) * 16 + mu.reg_read(UC_X86_REG_IP) - 2
        h = self.handlers.get(lin) if intno == 0x30 else None
        if h is None:
            raise RuntimeError('emu16: interrupt %X at %X' % (intno, lin))
        self.calls.append(lin)
        mu.reg_write(UC_X86_REG_AX, h(self) & 0xFFFF)

    def call(self, foff, args):
        """args: words in the order pushed (the first pushed first); a
        far return to the sentinel. AX comes back."""
        mu = self.mu
        sp = 0xFFF0
        for w in args:
            sp -= 2
            mu.mem_write(SS * 16 + sp, struct.pack('<H', w & 0xFFFF))
        sp -= 4
        mu.mem_write(SS * 16 + sp, struct.pack('<HH', 0, RET))
        for r, v in ((UC_X86_REG_SS, SS), (UC_X86_REG_SP, sp), (UC_X86_REG_DS, DS), (UC_X86_REG_ES, DS),
                     (UC_X86_REG_BP, 0), (UC_X86_REG_CS, CS), (UC_X86_REG_IP, foff - self.base)):
            mu.reg_write(r, v)
        self.calls = []
        mu.emu_start(CS * 16 + foff - self.base, RET * 16, count=200000)
        return mu.reg_read(UC_X86_REG_AX)
