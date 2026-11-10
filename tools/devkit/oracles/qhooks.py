#!/usr/bin/env python3
# The Dark Queen's to-hit hooks (round 16), read from the player's DQK.EXE:
#   oracle_qhooks.txt - for the attacker's effects (hook event 0xA), the
#   target's (0x10) and the target's AC ones (0xB): each effect's handler
#   (DS table + 4 x effect, filled by the overlay's setter) read and told
#   as a line: what it does to the roll / the AC and on what condition.
# Found by shape: the hook switch (dec ax / cmp ax,0x17 / jmp [cs:bx+t]),
# each case's "mov al,EFF / push ... / call near" list, the dispatcher's
# "call far [bx+TABLE]", the setter's "mov [T+2],cs|seg / mov word [T],off"
# pairs; the overlay segment's file base the one that makes the handlers
# function starts (push bp / mov bp,sp). Nothing of the game is kept here.
# usage: qhooks.py <DQK.EXE> [out]
import re, struct, subprocess, sys
from collections import Counter

exe = sys.argv[1]
out_path = sys.argv[2] if len(sys.argv) > 2 else '/tmp/claude-0/bt/oracle_qhooks.txt'
d = open(exe, 'rb').read()
LINE = re.compile(r'^([0-9A-F]{8})\s+([0-9A-F]+)\s+(.*)$')


def dis(foff, n):
    o = subprocess.run(['ndisasm', '-b16', '-o', str(foff), '-e', str(foff), exe], capture_output=True, text=True,
                       check=True).stdout
    ls = []
    for ln in o.splitlines():
        m = LINE.match(ln)
        if not m:
            continue
        a = int(m.group(1), 16)
        if a >= foff + n:
            break
        ls.append((a, m.group(3).strip()))
    return ls


# 1. the hook switch: push bp / mov bp,sp / mov al,[bp+6] / mov ah,0 / dec ax / cmp ax,0x17
sw = d.find(bytes.fromhex('558BEC8A4606B400483D1700'))
assert sw > 0, 'hook switch not found'
j = d.find(b'\x2E\xFF\xA7', sw)
toff = struct.unpack_from('<H', d, j + 3)[0]
tfile = j + 5
cbase = tfile - toff
end = None


def case_effects(ev):
    f = cbase + struct.unpack_from('<H', d, tfile + 2 * (ev - 1))[0]
    effs = []
    last = None
    for a, t in dis(f, 0x400):
        m = re.match(r'mov al,0x([0-9a-f]+)$', t)
        if m:
            last = int(m.group(1), 16)
        if t.startswith('call 0x') and last is not None and 'push' not in t:
            effs.append(last)
            last = None
        if t.startswith('jmp ') and 'short' not in t:
            break
        if re.match(r'ret', t):
            break
    return effs


# 2. the dispatcher's table: call far [bx+T] after "mov al,[bp+6] / mov ah,0 /
# mov bx,ax / shl bx,1 / shl bx,1" - the T the setter's pairs fill most
def setter_pairs(T):
    pairs = {}
    for m in re.finditer(rb'(?=\x8C\x0E(..)\xC7\x06(..)(..))', d, re.S):
        hi = struct.unpack('<H', m.group(1))[0]
        lo = struct.unpack('<H', m.group(2))[0]
        if hi == lo + 2 and 0 <= lo - T < 4 * 0x100 and (lo - T) % 4 == 0:
            pairs[(lo - T) // 4] = ('cs', struct.unpack('<H', m.group(3))[0])
    for m in re.finditer(rb'(?=\xC7\x06(..)(..)\xC7\x06(..)(..))', d, re.S):
        hi = struct.unpack('<H', m.group(1))[0]
        lo = struct.unpack('<H', m.group(3))[0]
        if hi == lo + 2 and 0 <= lo - T < 4 * 0x100 and (lo - T) % 4 == 0:
            pairs[(lo - T) // 4] = (struct.unpack('<H', m.group(2))[0], struct.unpack('<H', m.group(4))[0])
    return pairs


cands = [struct.unpack_from('<H', d, m.start() + 13)[0]
         for m in re.finditer(re.escape(bytes.fromhex('8A4606B4008BD8D1E3D1E3FF9F')), d)]
T = max(cands, key=lambda t: len(setter_pairs(t)))
pairs = setter_pairs(T)
start = lambda f: d[f:f + 3] == b'\x55\x8B\xEC'
cs_offs = {o for s, o in pairs.values() if s == 'cs'}
B = max(range(0x10000, len(d) - 0x10000), key=lambda b: sum(start(b + o) for o in cs_offs))
segs = {s for s, o in pairs.values() if s != 'cs'}


def seg_base(s):
    best = max(range(0, 0x1000), key=lambda dd: sum(start(B + dd * 16 + o) for ss, o in pairs.values() if ss == s))
    return B + best * 16


SB = {s: seg_base(s) for s in segs}


def handler(e):
    if e not in pairs:
        return None
    s, o = pairs[e]
    return (B if s == 'cs' else SB[s]) + o


# 4. each handler told
def tell(e):
    h = handler(e)
    if h is None:
        return 'nohandler'
    code = dis(h, 0x200)
    out, subj, cond = [], 'holder', []
    roll = 0
    i = 0
    while i < len(code):
        a, t = code[i]
        if t == 'retf':
            break
        if t == 'les bx,[0xad67]':
            subj = 'attacker'
        elif t == 'les bx,[bp+0x6]':
            subj = 'holder'
        elif t == 'les bx,[es:bx+0x40]':
            subj = subj + '.combat'
        elif t == 'les bx,[es:bx+0xb]' and subj.endswith('.combat'):
            subj = subj[:-7] + '.target'
        m = re.match(r'test byte \[es:bx\+0xc7\],0x([0-9a-f]+)$', t)
        if m:
            cond.append('%s_flags&0x%02X' % (subj, int(m.group(1), 16)))
        if t == 'mov al,0x18' and i + 3 < len(code):
            who = 'attacker' if '0xad69' in code[i + 2][1] else 'holder'
            cond.append('%s_lacks 0x18' % who)
        if re.match(r'cmp word \[es:bx\+0x3\],byte \+0x0', t) and subj.endswith('.combat'):
            cond.append('holder_acted')
        if re.match(r'(cmp word|mov ax,)\[?\[?es:bx\+0x66\]', t):
            cond.append('prayer')
        m = re.match(r'(add|sub) byte \[0xbec5\],0x([0-9a-f]+)$', t)
        if m:
            roll += (1 if m.group(1) == 'add' else -1) * int(m.group(2), 16)
        if t == 'inc byte [0xbec5]':
            roll += 1
        if t == 'dec byte [0xbec5]':
            roll -= 1
        if t == 'mov byte [0xbec5],0xff':
            out.append('miss')
        if 'test byte [es:bx+0x3],0x10' in t:
            out.append('cloak')
        m = re.match(r'(add|sub) byte \[es:bx\+0x18([ef])\],0x([0-9a-f]+)$', t)
        if m:
            out.append('ac%s %+d' % ('front' if m.group(2) == 'e' else 'rear',
                                     (1 if m.group(1) == 'add' else -1) * int(m.group(3), 16)))
        if re.match(r'mov byte \[es:bx\+0x18e\],0x([0-9a-f]+)$', t):
            out.append('floorfront 0x%02X' % int(t.split(',')[-1], 16))
        if re.match(r'mov \[es:bx\+0x18d\],al', t):
            out.append('sets_tohit')
        if t == 'call 0x77e:0xbe2':
            out.append('recalc')
        m = re.match(r'call 0x([0-9a-f]+)$', t)
        if m and 'prayer' in cond:
            # the same side's branch: a near call into the handler's segment
            tgt = (h & ~0xFFFF) + int(m.group(1), 16)
            if tgt > h + 0x8000:
                tgt -= 0x10000
            sub = 0
            for _, tt in dis(tgt, 0x40):
                if tt == 'retf':
                    break
                if tt == 'inc byte [0xbec5]':
                    sub += 1
                if tt == 'dec byte [0xbec5]':
                    sub -= 1
            out.append('same %+d' % sub)
            cond.remove('prayer')
            cond.append('prayer')
        i += 1
    if 'prayer' in cond:
        return 'prayer %s other %+d' % (' '.join(o for o in out if o.startswith('same')), roll)
    parts = []
    if roll:
        parts.append('roll %+d' % roll)
    parts += out
    if not parts:
        return 'none'
    return ' '.join(parts) + (' if ' + ' '.join(cond) if cond else '')


lines = ['# oracle_qhooks - DQK.EXE hook switch file 0x%X (table 0x%X), handlers DS 0x%X + 4 x effect, base 0x%X'
         % (sw, tfile, T, B)]
for ev, tag in ((0x0A, 'A'), (0x10, 'T'), (0x0B, 'B')):
    for e in case_effects(ev):
        h = handler(e)
        lines.append('%s 0x%02X %s   # %s' % (tag, e, tell(e), ('0x%X' % h) if h else '-'))
open(out_path, 'w').write('\n'.join(lines) + '\n')
print('\n'.join(lines))
