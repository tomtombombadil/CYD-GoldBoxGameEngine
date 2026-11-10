#!/usr/bin/env python3
# The computer's target at its turn (round 17), every DAX game: its GAME.OVR's
# calls of the target finder with range 0xFF, second argument 1, third 0
# [, fourth 0] - the AI loop's (a far call, then "or al,al / jz": found ->
# the move step, else the guard), the turn's first one (Darkness-type, Death
# Knights: the result kept in [bp-3], only when the combat target is empty)
# and the step back's (count 2, "jnz" - finder.py) - and the finder itself
# (resolved through the unpacked program's overlay stub), read by its shape:
#   keep     the target kept: the third argument 0, the target not empty, of
#            the other side (the side byte compared), on the field (the
#            active byte not 0) and seen (the seen routine) - no range, no walls
#   struck   a random pick of one already struck: "continue" (a try spent,
#            the next pick) or "end" (the pass over)
#   walls    the walls' flag (map +6) set only in the second pass: the first
#            pass's list is those in a sight line
#   taunt    Champions: on a pass's first try (tries 0x13 after the dec) a
#            holder of effect E takes the list's first entry without a roll
#            when it is the one who yelled
#   order    finder_first: the loop calls the finder, then tests the moves
#            (Curse-type); moves_first: moves > 0 and the active byte, then
#            the finder (Darkness-type)
# oracle_turnfind_<game>.txt one line each.
# usage: turnfind.py <GAME.OVR> <unpacked program image> <game> [outdir]
import re, struct, sys

ovr, img, game = sys.argv[1:4]
outdir = sys.argv[4] if len(sys.argv) > 4 else '/tmp/claude-0/bt'
d = open(ovr, 'rb').read()
im = open(img, 'rb').read()


def resolve(p):
    off, seg = struct.unpack_from('<HH', d, p + 1)
    s = seg * 16
    if im[s:s + 2] != b'\xcd\x3f':
        return None
    return struct.unpack_from('<I', im, s + 4)[0] + struct.unpack_from('<H', im, s + off + 2)[0]


loop = early = None
finder = None
nargs = 0
for m in re.finditer(re.escape(bytes.fromhex('B0FF50B00150B00050')), d):
    p, n = m.end(), 3
    if d[p:p + 3] == bytes.fromhex('B00050'):
        p, n = p + 3, 4
    if d[p] != 0x9A:
        continue
    t = resolve(p)
    if t is None:
        continue
    nxt = d[p + 5:p + 9]
    if nxt[:3] == bytes.fromhex('08C074'):
        before = d[m.start() - 0x30:m.start()]
        # (moves_first: "cmp byte [es:di+3],0 / jng" just before the pushes)
        loop = (m.start(), 'moves_first' if bytes.fromhex('26807D03007E') in before else 'finder_first')
        finder, nargs = t, n
    elif nxt[:3] == bytes.fromhex('8846FD'):
        before = d[m.start() - 0x12:m.start()]
        # (only when the target is empty: "or ax,[es:di+..] / jnz" over the call)
        if re.search(rb'\x26\x0B\x45.\x75', before, re.S):
            early = m.start()

lines = ['# oracle_turnfind_%s' % game]
if loop is None:
    lines.append('turn none')
else:
    body = d[finder:finder + 0x320]
    # keep: cmp byte [bp+3rd],0 / jnz; the target's side byte against its own
    # (mov al,[es:di+S] / les / cmp al,[es:di+S] / jz); its active byte (cmp
    # byte [es:di+A],0 / jz); the seen routine (near call / or al,al / jnz)
    kp = re.search(rb'\x26\x8A\x85(..)\xC4\x7E.\x26\x3A\x85(..)\x74.\xC4\x7E.\x26\x80\xBD(..)\x00\x74', body, re.S)
    seen = kp is not None and re.search(rb'\xE8..\x08\xC0\x75', body[kp.end():kp.end() + 0x20], re.S) is not None
    keep = 'keep side 0x%X active 0x%X seen %d' % (struct.unpack('<H', kp.group(1))[0],
                                                  struct.unpack('<H', kp.group(3))[0], int(seen)) if kp else 'keep none'
    # struck: cmp byte [di+L],0 / ja +3 / jmp X - X a jmp back to the loop's
    # test (continue) or the pass's end
    st = re.search(rb'\x80\xBD(..)\x00\x77\x03\xE9(..)', body, re.S)
    struck = 'unknown'
    if st:
        x = finder + st.end() + struct.unpack('<h', st.group(2))[0]
        rel = x - finder
        # Curse-type: X = "jmp <loop test>"; Darkness-type: X = the advance
        # ("cmp byte [bp-6],0 / jnz" then "inc byte [bp-4]") - both go on
        if d[x] == 0xE9:
            y = x + 3 + struct.unpack_from('<h', d, x + 1)[0]
            struck = 'continue' if d[y:y + 4] == bytes.fromhex('807EFD00') else 'end'
        elif d[x:x + 4] == bytes.fromhex('807EFA00') and d[x + 6:x + 9] == bytes.fromhex('FE46FC'):
            struck = 'continue'
    # walls: the map's flag +6 set after "cmp byte [bp-7],0 / jz" (the second pass only)
    walls = int(re.search(rb'\x80\x7E\xF9\x00\x74.\x80\x7E.\x00\x75.\xC4\x3E..\x26\xC6\x45\x06\x01', body, re.S) is not None)
    tn = re.search(rb'\xB0(.)\x50\x8D\x7E.\x16\x57\x9A....\x08\xC0\x74.\xA0..\x30\xE4.{12,32}\x80\x7E\xFD\x13', body, re.S)
    taunt = 'taunt 0x%02X first 1' % tn.group(1)[0] if tn else 'taunt 0 first 0'
    lines.append('turn call 0x%X finder 0x%X args %d order %s early %s %s struck %s pass1walls %d %s' % (
        loop[0], finder, nargs, loop[1], '0x%X' % early if early is not None else 'none', keep, struck, walls, taunt))
open('%s/oracle_turnfind_%s.txt' % (outdir, game), 'w').write('\n'.join(lines) + '\n')
print(game, lines[-1])
