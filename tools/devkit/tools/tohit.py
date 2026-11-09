#!/usr/bin/env python3
# tohit.py <GAME.OVR | DQK.EXE> [more ...]: every game's to-hit routines found by
# their shape (a d20 kept in a DS byte, "cmp [d20],20 / jnz / mov [d20],100")
# and each enclosing routine disassembled (ndisasm, file offsets) - round 11.
# Two in each program: the plain roll against a value, and attacker against
# target (the side's bonus: game block bytes, the difficulty's +2..-2).
import re, subprocess, sys

PAT = re.compile(rb'(\x80\x3e..\x14\x75\x05\xc6\x06..\x64)', re.S)

def start_of(b, at):
    # back to the routine's prologue (push bp; mov bp,sp - both encodings)
    for i in range(at, max(0, at - 0x200), -1):
        if b[i] == 0x55 and b[i + 1:i + 3] in (b'\x8b\xec', b'\x89\xe5'):
            return i
    return at - 0x40

def end_of(b, at):
    # the first "pop bp; retf [n]" after it
    for i in range(at, min(len(b), at + 0x300)):
        if b[i] == 0x5d and b[i + 1] in (0xcb, 0xca):
            return i + (4 if b[i + 1] == 0xca else 2)
    return at + 0x100

for path in sys.argv[1:]:
    b = open(path, 'rb').read()
    for m in PAT.finditer(b):
        s = start_of(b, m.start())
        e = end_of(b, m.start())
        print('=== %s: d20 at 0x%X, routine 0x%X-0x%X' % (path, m.start(), s, e))
        out = subprocess.run(['ndisasm', '-b16', '-e', str(s), '-o', str(s), '-'], input=b[:e],
                             capture_output=True).stdout.decode()
        print(out)
