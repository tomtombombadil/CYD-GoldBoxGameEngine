#!/usr/bin/env python3
# appraise.py <game> <program file> [out]: a game's Appraise values by its
# d100 roll, read from the original's code (round 10) - for sweep's behave
# economy G2. Output /tmp/claude-0/bt/oracle_appraise_<game>.txt:
#   gem <r> <value>             r = 1..100
#   jewel <r> <base> <span>     the value base + random(span)
# game:
#   champions / knights  GAME.OVR: the gem chain (cmp al,lo / jc / cmp al,hi /
#       ja / mov word [bp-0xA8],v - Champions 0x2D4F1, Death Knights 0x295B6)
#       and the jewel chain (cmp al,lo / jc / cmp al,hi / ja / mov ax,span /
#       push / call random ... / add ax,base - Death Knights 0x2980A), the
#       last case "cmp al,0x64 / jnz" a single roll
#   queen  DQK.EXE: two case tables of 100 offsets (file 8's code at
#       0x491F0): gems jmp [cs:bx+0xB9C] at 0x49D87, each case
#       mov word [bp-0xE],v; jewels jmp [cs:bx+0xE51] at 0x4A03C, each case
#       mov ax,span / push / call random / pop / add ax,base
import re, struct, sys

def chains(b):
    gems, jewels = {}, {}
    # gems: 3C lo 72 xx 3C hi 77 xx C7 86 58 FF v  |  3C 64 75 xx C7 86 58 FF v
    g = re.search(rb'\x3c\x01\x72.\x3c(.)\x77.\xc7\x86\x58\xff', b, re.S)
    if not g:
        raise SystemExit('no gem chain')
    p = g.start()
    while True:
        m = re.compile(rb'\x3c(.)\x72.\x3c(.)\x77.\xc7\x86\x58\xff(..)\xeb.', re.S).match(b, p)
        if m:
            lo, hi, v = m.group(1)[0], m.group(2)[0], struct.unpack('<H', m.group(3))[0]
            for r in range(lo, hi + 1): gems[r] = v
            p = m.end()
            continue
        m = re.compile(rb'\x3c(.)\x75.\xc7\x86\x58\xff(..)', re.S).match(b, p)
        if m:
            gems[m.group(1)[0]] = struct.unpack('<H', m.group(2))[0]
        break
    j = re.search(rb'\x3c\x01\x72.\x3c(.)\x77.\xb8', b[p:], re.S)
    if not j:
        raise SystemExit('no jewel chain')
    p += j.start()
    case = re.compile(rb'\x3c(.)\x72.\x3c(.)\x77.\xb8(..)\x50\x9a....(?:\x31\xd2)?(?:\x9a....)*\x05(..)', re.S)
    while True:
        m = case.match(b, p)
        if not m:
            break
        lo, hi = m.group(1)[0], m.group(2)[0]
        span, base = struct.unpack('<H', m.group(3))[0], struct.unpack('<H', m.group(4))[0]
        for r in range(lo, hi + 1): jewels[r] = (base, span)
        nxt = b.find(b'\x3c', m.end())
        if nxt < 0 or nxt - m.end() > 12:
            break
        p = nxt
    return gems, jewels

def queen(exe):
    cs = 0x491F0
    def targets(jmp_at, table):
        assert exe[jmp_at:jmp_at + 3] == b'\x2e\xff\xa7' and struct.unpack('<H', exe[jmp_at + 3:jmp_at + 5])[0] == table
        return [cs + struct.unpack('<H', exe[cs + table + 2 * i:cs + table + 2 * i + 2])[0] for i in range(100)]
    gems, jewels = {}, {}
    for r, t in enumerate(targets(0x49D87, 0xB9C), 1):
        if exe[t:t + 3] != b'\xc7\x46\xf2':
            raise SystemExit('gem case %d at %x' % (r, t))
        gems[r] = struct.unpack('<H', exe[t + 3:t + 5])[0]
    for r, t in enumerate(targets(0x4A03C, 0xE51), 1):
        if exe[t] != 0xB8 or exe[t + 3] != 0x50 or exe[t + 4] != 0x9A or exe[t + 10] != 0x05:
            raise SystemExit('jewel case %d at %x' % (r, t))
        jewels[r] = (struct.unpack('<H', exe[t + 11:t + 13])[0], struct.unpack('<H', exe[t + 1:t + 3])[0])
    return gems, jewels

def main():
    game, path = sys.argv[1], sys.argv[2]
    b = open(path, 'rb').read()
    gems, jewels = queen(b) if game == 'queen' else chains(b)
    if sorted(gems) != list(range(1, 101)) or sorted(jewels) != list(range(1, 101)):
        raise SystemExit('rolls not all covered: gems %d jewels %d' % (len(gems), len(jewels)))
    out = sys.argv[3] if len(sys.argv) > 3 else '/tmp/claude-0/bt/oracle_appraise_%s.txt' % game
    with open(out, 'w') as f:
        for r in range(1, 101): f.write('gem %d %d\n' % (r, gems[r]))
        for r in range(1, 101): f.write('jewel %d %d %d\n' % (r, jewels[r][0], jewels[r][1]))
    print('appraise %s: 200 rows -> %s' % (game, out))

main()
