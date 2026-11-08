#!/usr/bin/env python3
# Readers of the originals' CODE for the oracle generators: a stretch of a
# program file disassembled with ndisasm (16-bit), and a tiny interpreter
# for the few instructions the creation / rule routines use to pick their
# constants (cmp / jcc chains on al or a byte, byte inc / dec / add / sub /
# mov to a named place). Only addresses and how the code is shaped live
# here - the values come out of the player's own file at run time.
import re, subprocess

_LINE = re.compile(r'^([0-9A-F]{8})\s+([0-9A-F]+)\s+(.*)$')


class Code:
    """ndisasm of file[foff:foff+n], addresses shown as foff - base."""
    def __init__(self, path, foff, n, base=None):
        base = foff if base is None else base
        self.base = base
        out = subprocess.run(['ndisasm', '-b16', '-o', str(foff - base), '-e', str(foff), path],
                             capture_output=True, text=True, check=True).stdout
        self.ls = []
        for ln in out.splitlines():
            m = _LINE.match(ln)
            if not m:
                continue
            a = int(m.group(1), 16)
            if a >= foff - base + n:
                break
            self.ls.append((a, bytes.fromhex(m.group(2)), m.group(3).strip()))
        self.at = {a: i for i, (a, _, _) in enumerate(self.ls)}

    def find(self, text, start=0, stop=None):
        """Addresses of lines whose text matches the regex, in [start, stop)."""
        r = re.compile(text)
        return [a for a, _, t in self.ls if a >= start and (stop is None or a < stop) and r.search(t)]

    def after(self, a, k=1):
        i = self.at[a] + k
        return self.ls[i] if i < len(self.ls) else (None, b'', '')

    def text(self, a):
        return self.ls[self.at[a]][2]

    def imm(self, a):
        """The last number in the line at a (its immediate)."""
        return int(re.findall(r'0x[0-9a-fA-F]+|\b\d+\b', self.text(a))[-1], 0)


def num(s):
    return int(s, 0)


def run(code, start, stop, regs, mem, limit=5000):
    """Runs code from `start` until `stop` (or a ret / far call / unknown
    jump): al from regs['al'], bytes in mem by their operand text (e.g.
    '[bp-0x15]'). cmp sets the flags; jz/jnz/jc/jnc/jna/ja/jl/jg/jng/jnl
    and jmp short / near follow them; inc / dec / add / sub / mov byte to
    a place change mem; 'mov al,<place>' loads al. Other lines are passed
    over. Returns mem."""
    a = start
    flags = (0, 0)
    for _ in range(limit):
        if a is None or a == stop or a not in code.at:
            return mem
        _, _, t = code.ls[code.at[a]]
        nxt = code.after(a)[0]
        m = re.match(r'(\w+)\s+(.*)$', t)
        op, args = (m.group(1), m.group(2)) if m else (t, '')
        args = re.sub(r'^(byte|word|short|near)\s+', '', args)
        parts = [p.strip() for p in args.split(',')]
        if op == 'mov' and parts[0] == 'al' and parts[1].startswith('['):
            regs['al'] = mem.get(parts[1], 0) & 0xFF
        elif op == 'mov' and parts[0] == 'al':
            regs['al'] = num(parts[1]) & 0xFF
        elif op == 'cmp':
            x = regs['al'] if parts[0] == 'al' else mem.get(re.sub(r'^byte\s+', '', parts[0]), 0)
            flags = (x & 0xFF, num(parts[1]) & 0xFF)
        elif op.startswith('j'):
            x, y = flags
            sx, sy = (x - 256 if x > 127 else x), (y - 256 if y > 127 else y)
            take = {'jmp': True, 'jz': x == y, 'jnz': x != y, 'jc': x < y, 'jnc': x >= y, 'jna': x <= y,
                    'ja': x > y, 'jl': sx < sy, 'jnl': sx >= sy, 'jg': sx > sy, 'jng': sx <= sy}.get(op)
            if take is None:
                raise ValueError('jump %s at %X' % (t, a))
            if take:
                nxt = num(args.split()[-1])
        elif op in ('inc', 'dec') and parts[0].startswith('['):
            mem[parts[0]] = mem.get(parts[0], 0) + (1 if op == 'inc' else -1)
        elif op in ('add', 'sub') and parts[0].startswith('['):
            v = num(parts[1])
            v = v - 256 if v > 127 else v
            mem[parts[0]] = mem.get(parts[0], 0) + (v if op == 'add' else -v)
        elif op == 'mov' and parts[0].startswith('[') and re.match(r'0x[0-9a-fA-F]+$|\d+$', parts[1]):
            mem[parts[0]] = num(parts[1])
        elif op in ('ret', 'retf') or (op == 'call' and ':' in args):
            return mem
        a = nxt
    raise RuntimeError('ran too long from %X' % start)


def cases(code, start, stop, reg='al'):
    """A switch on al written as a cmp / jz / jnz chain in [start, stop):
    [(values, body start, body end)] - the body runs from after a jnz up
    to the jnz's target (jz cases gather onto the next jnz)."""
    out, pend = [], []
    for a, _, t in code.ls:
        if a < start or a >= stop:
            continue
        m = re.match(r'cmp %s,(0x[0-9a-fA-F]+|\d+)$' % reg, t)
        if not m:
            continue
        nxt = code.after(a)
        j = nxt[2].split()
        if j[0] == 'jz':
            pend.append(num(m.group(1)))
        elif j[0] == 'jnz':
            out.append((pend + [num(m.group(1))], code.after(a, 2)[0], num(j[-1])))
            pend = []
    return out
