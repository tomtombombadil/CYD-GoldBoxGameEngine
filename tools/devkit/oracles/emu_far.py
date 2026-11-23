#!/usr/bin/env python3
# emu_far.py - runs a routine of a Turbo Pascal Gold Box program (Darkness,
# Treasures, Death Knights: GAME.EXE / START.EXE + GAME.OVR) in Unicorn WITH
# the code it calls: the unpacked program image at linear 0 (its own
# segments as they are), every overlay the run enters loaded on demand into a
# 64 KB slot (the stubs' "int 3Fh" caught: the overlay's file start and the
# entry's offset - guard.py's reading), so helpers in other overlays run as
# the original's code. Routines given a Python handler (screen, delays,
# Random) are not run: their far return (retf n, n read from the routine's
# own first retf) is done for them. Needs unicorn (/tmp/claude-0/uvenv).
# Nothing of the games is kept here: code and data come from the player's
# files at run time (gb.image / gb.raw).
import struct

import capstone
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INTR
from unicorn.x86_const import (UC_X86_REG_AX, UC_X86_REG_DX, UC_X86_REG_SP, UC_X86_REG_IP, UC_X86_REG_CS,
                               UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS, UC_X86_REG_BP)

SLOT0, SLOTS = 0x30000, 6          # overlay slots 0x30000-0x8FFFF
REC = 0x9000                       # records at 0x9000:0 (0x400 apart)
SS = 0xA000                        # the stack
RET = 0xB000                       # the sentinel far return
IMG_PY = 0x31                      # an image routine replaced by a handler: "int 31h"
NEAR_PY = 0x32                     # an overlay's near routine replaced by a handler: "int 32h"


class Machine:
    def __init__(self, img, ovr, ds_image):
        self.img, self.ovr = img, ovr
        self.mu = Uc(UC_ARCH_X86, UC_MODE_16)
        self.mu.mem_map(0, 0x100000)
        self.mu.mem_write(0, img)
        self.mu.mem_write(RET * 16, b'\xf4')
        assert ds_image % 16 == 0
        self.ds = ds_image // 16
        self.slots = {}             # overlay file start -> segment
        self.py_ovr = {}            # overlay routine (file offset) -> (fn, argbytes)
        self.py_img = {}            # image routine (linear) -> (fn, argbytes)
        self.py_near = {}           # overlay near routine (file offset) -> (fn, argbytes)
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
        self.mu.hook_add(UC_HOOK_INTR, self._intr)
        self.log = []

    # The argument bytes a routine's own retf takes (its first retf)
    def argbytes(self, code, off):
        for i in self.md.disasm(code[off:off + 0x2000], off):
            if i.mnemonic == 'retf':
                return int(i.op_str, 16) if i.op_str else 0
        raise RuntimeError('emu_far: no retf after %X' % off)

    def py_overlay(self, foff, fn, argbytes=None):
        self.py_ovr[foff] = (fn, self.argbytes(self.ovr, foff) if argbytes is None else argbytes)

    def py_image(self, lin, fn, argbytes=None):
        n = self.argbytes(self.img, lin) if argbytes is None else argbytes
        self.py_img[lin] = (fn, n)
        self.mu.mem_write(lin, bytes([0xCD, IMG_PY]))

    def py_near_routine(self, foff, fn, argbytes=None):
        """A routine inside an overlay's segment (reached by a near call, or by
        "push cs; call" - a far one): its handler, then its own return (ret n
        or retf n, the routine's first)."""
        far = False
        if argbytes is None:
            for i in self.md.disasm(self.ovr[foff:foff + 0x2000], foff):
                if i.mnemonic in ('ret', 'retf'):
                    argbytes = int(i.op_str, 16) if i.op_str else 0
                    far = i.mnemonic == 'retf'
                    break
            else:
                raise RuntimeError('emu_far: no return after %X' % foff)
        self.py_near[foff] = (fn, argbytes, far)

    def stub_target(self, seg, off):
        s = seg * 16
        base = struct.unpack_from('<I', self.img, s + 4)[0]
        return base, base + struct.unpack_from('<H', self.img, s + off + 2)[0]

    def load(self, base):
        if base not in self.slots:
            if len(self.slots) >= SLOTS:
                raise RuntimeError('emu_far: out of overlay slots')
            seg = (SLOT0 >> 4) + 0x1000 * len(self.slots)
            self.mu.mem_write(seg * 16, self.ovr[base:base + 0x10000].ljust(0x10000, b'\0'))
            for foff in self.py_near:
                if base <= foff < base + 0x10000:
                    self.mu.mem_write(seg * 16 + foff - base, bytes([0xCD, NEAR_PY]))
            self.slots[base] = seg
        return self.slots[base]

    # The words above the far return (the last pushed first)
    def args(self, n, near=False):
        sp = self.mu.reg_read(UC_X86_REG_SP)
        return list(struct.unpack('<%dH' % n, self.mu.mem_read(SS * 16 + sp + (2 if near else 4), 2 * n)))

    def _return(self, argbytes, ax=0, dx=None):
        mu = self.mu
        sp = mu.reg_read(UC_X86_REG_SP)
        ip, cs = struct.unpack('<HH', mu.mem_read(SS * 16 + sp, 4))
        mu.reg_write(UC_X86_REG_SP, sp + 4 + argbytes)
        mu.reg_write(UC_X86_REG_AX, ax & 0xFFFF)
        if dx is not None:
            mu.reg_write(UC_X86_REG_DX, dx & 0xFFFF)
        mu.reg_write(UC_X86_REG_CS, cs)
        mu.reg_write(UC_X86_REG_IP, ip)

    def _intr(self, mu, intno, _):
        cs, ip = mu.reg_read(UC_X86_REG_CS), mu.reg_read(UC_X86_REG_IP)
        if intno == 0x3F:
            base, foff = self.stub_target(cs, ip - 2)
            if foff in self.py_ovr:
                fn, n = self.py_ovr[foff]
                self.log.append('ovr %X' % foff)
                r = fn(self)
                ax, dx = (r if isinstance(r, tuple) else (r or 0, None))
                self._return(n, ax, dx)
                return
            seg = self.load(base)
            mu.reg_write(UC_X86_REG_CS, seg)
            mu.reg_write(UC_X86_REG_IP, foff - base)
            return
        if intno == IMG_PY:
            lin = cs * 16 + ip - 2
            if lin in self.py_img:
                fn, n = self.py_img[lin]
                self.log.append('img %X' % lin)
                r = fn(self)
                ax, dx = (r if isinstance(r, tuple) else (r or 0, None))
                self._return(n, ax, dx)
                return
        if intno == NEAR_PY:
            base = next(b for b, sg in self.slots.items() if sg == cs)
            foff = base + ip - 2
            if foff in self.py_near:
                fn, n, far = self.py_near[foff]
                self.log.append('near %X' % foff)
                r = fn(self, near=not far)
                ax = r or 0
                if far:
                    self._return(n, ax)
                    return
                sp = mu.reg_read(UC_X86_REG_SP)
                rip = struct.unpack('<H', mu.mem_read(SS * 16 + sp, 2))[0]
                mu.reg_write(UC_X86_REG_SP, sp + 2 + n)
                mu.reg_write(UC_X86_REG_AX, ax & 0xFFFF)
                mu.reg_write(UC_X86_REG_IP, rip)
                return
        raise RuntimeError('emu_far: interrupt %X at %04X:%04X' % (intno, cs, ip))

    def record(self, k, data):
        self.mu.mem_write(REC * 16 + k * 0x400, bytes(data).ljust(0x400, b'\0'))

    def read_record(self, k, n):
        return bytes(self.mu.mem_read(REC * 16 + k * 0x400, n))

    def call_ovr(self, foff, args):
        """An overlay routine (its file offset) with words pushed in order; AX back."""
        base = max(b for b in self.bases() if b <= foff)
        seg = self.load(base)
        mu = self.mu
        sp = 0xFFF0
        for w in args:
            sp -= 2
            mu.mem_write(SS * 16 + sp, struct.pack('<H', w & 0xFFFF))
        sp -= 4
        mu.mem_write(SS * 16 + sp, struct.pack('<HH', 0, RET))
        for r, v in ((UC_X86_REG_SS, SS), (UC_X86_REG_SP, sp), (UC_X86_REG_DS, self.ds), (UC_X86_REG_ES, self.ds),
                     (UC_X86_REG_BP, 0), (UC_X86_REG_CS, seg), (UC_X86_REG_IP, foff - base)):
            mu.reg_write(r, v)
        self.log = []
        mu.emu_start(seg * 16 + foff - base, RET * 16, count=5000000)
        return mu.reg_read(UC_X86_REG_AX)

    def bases(self):
        """Every overlay's file start the image's stubs name."""
        out = set()
        for s in range(0, len(self.img) - 16, 16):
            if self.img[s:s + 2] == b'\xcd\x3f':
                b = struct.unpack_from('<I', self.img, s + 4)[0]
                if b < len(self.ovr):
                    out.add(b)
        return out
