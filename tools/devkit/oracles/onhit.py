#!/usr/bin/env python3
# The on-hit specials' damage test (round 17), every game: where the blow,
# after its damage routine, calls the hook event slot + 1 (attack 1's hit 2,
# attack 2's 3) on the attacker - GAME.OVR: "mov al,[bp-x] / xor ah,ah /
# inc ax / push ax / lea di,[bp+y] / push ss / push di (or push word
# [bp+y+2] / push word [bp+y]) / call", the Dark
# Queen's DQK.EXE: "mov al,[bp-x] / inc al / push ax / call far" after
# "cmp word [dmg],0 / jg" - and what stands between the target's in-the-
# fight test and the call:
#   needs_damage 1  "cmp byte|word [D],0 / jna|jng" (D the damage the blow
#                   passed to its damage routine); 0: the target's test only
#   free 0xNN       (the Dark Queen) the hook also when the attacker has that
#                   effect (the effect finder 0x77E:0x1734 called with it)
# oracle_onhit_<game>.txt: "onhit at 0x.. needs_damage N damage 0x.. free 0x.."
# usage: onhit.py <GAME.OVR | DQK.EXE> <game> [outdir]
import re, struct, sys

path, game = sys.argv[1:3]
outdir = sys.argv[3] if len(sys.argv) > 3 else '/tmp/claude-0/bt'
d = open(path, 'rb').read()
line = 'onhit none'
if game == 'queen':
    # cmp word [D],0 / jg +N / (push ss / lea ax,[bp-x] / push ax / mov al,E / push ax / ... call far) / or al,al / jz
    # then the hook: mov al,[bp-x] / inc al / push ax / call far
    m = re.search(rb'\x83\x3E(..)\x00\x7F.\x16\x8D\x46.\x50\xB0(.)\x50.{6}\x9A....\x83\xC4.\x0A\xC0\x74.'
                  rb'\x16\x8D\x46.\x50\x8A\x46.\xFE\xC0\x50\x9A', d, re.S)
    if m:
        line = 'onhit at 0x%X needs_damage 1 damage 0x%X free 0x%02X' % (
            m.start(), struct.unpack('<H', m.group(1))[0], m.group(2)[0])
else:
    for m in re.finditer(rb'\x8A\x46.\x30\xE4\x40\x50(\x8D\x7E.\x16\x57|\xFF\x76.\xFF\x76.)(\x9A|\x0E\xE8)', d, re.S):
        before = d[m.start() - 0x20:m.start()]
        # the target's in-the-fight test just before: les di,[bp+..] / cmp byte [es:di+A],0 / jz
        t = re.search(rb'\xC4\x7E.\x26\x80\xBD(..)\x00\x74(.)', before, re.S)
        if not t:
            continue
        rest = before[t.end():]
        dm = re.match(rb'(\x80\x3E(..)\x00\x76.|\x83\x3E(..)\x00\x7E.)', rest, re.S)
        if dm:
            dv = struct.unpack('<H', dm.group(2) or dm.group(3))[0]
            line = 'onhit at 0x%X needs_damage 1 damage 0x%X free 0x00' % (m.start() - len(rest), dv)
        elif rest == b'':
            line = 'onhit at 0x%X needs_damage 0 damage 0 free 0x00' % m.start()
        else:
            continue
        break
open('%s/oracle_onhit_%s.txt' % (outdir, game), 'w').write('# oracle_onhit_%s\n%s\n' % (game, line))
print(game, line)
