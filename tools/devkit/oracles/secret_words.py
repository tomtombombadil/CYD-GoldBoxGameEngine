#!/usr/bin/env python3
# The sp_word rows of behave secret's allspells part (behave_secret_spells.inc
# words()): each spell's first "is ..." word - "is " + the string at a data
# segment offset (0) or at an offset in the spell overlay (GAME.OVR 0x2C64B)
# - from the Project's secret_facts_2.md section 14 (the handlers' words:
# "is " DS 0x1B4D + an effect's name DS 0x2227 + 38 x effect, or "Healed" DS
# 0x1C4D, "Affected" 0x1D4D, "Protected" 0x1F4D, or a string in the
# handler's code). Each string is read from the player's files and checked to
# be a Pascal string; appended to oracle_secret_spells.txt (run after
# secret_spells.py).
import os, sys
import gb

DSB, OVL = 0xDE20, 0x2C64B
img = gb.image('secret')
ovr = gb.raw('secret', 'GAME.OVR')
E = lambda n: (0, 0x2227 + 38 * n)
AFF, PROT, HEALED = (0, 0x1D4D), (0, 0x1F4D), (0, 0x1C4D)
C = lambda a: (1, a - OVL)
WORDS = {1: E(1), 2: E(2), 8: E(10), 10: E(11), 12: E(12), 14: E(14), 19: E(17), 24: E(20), 25: E(21), 27: E(11),
         32: E(28), 33: E(29), 38: E(33), 40: E(34), 42: E(49), 45: E(37), 48: E(39), 55: E(42), 56: E(34), 57: E(39),
         79: E(71), 80: E(25), 81: E(11), 82: E(35), 86: E(27), 96: E(11), 116: E(25), 117: E(106),
         30: C(0x2EEE5), 50: C(0x2EEE5), 36: C(0x2F671), 90: C(0x2F671), 43: C(0x2FD96), 89: C(0x2FD96),
         61: C(0x30752), 67: C(0x30914), 75: C(0x30CAD), 76: C(0x30D67), 113: C(0x32100), 114: C(0x32313),
         85: C(0x311F4), 62: HEALED, 99: HEALED}
for s in (23, 49, 94):
    WORDS[s] = E(52)
for s in (5, 11, 18, 22, 29, 77, 26, 41, 46, 73):
    WORDS[s] = AFF
for s in (6, 7, 16, 17, 52, 53, 54, 69, 88, 95, 112):
    WORDS[s] = PROT


def text(kind, off):
    a = DSB + off if kind == 0 else OVL + off
    b = img if kind == 0 else ovr
    n = b[a]
    t = b[a + 1:a + 1 + n]
    assert n and all(32 <= c < 127 for c in t), (kind, hex(off))
    return t.decode('latin-1')


path = os.path.join(gb.OUT, 'oracle_secret_spells.txt')
lines = [l for l in open(path).read().splitlines() if not l.startswith('sp_word')]
lines.append('# sp_word: tools/devkit/oracles/secret_words.py (secret_facts_2.md 14)')
for s in sorted(WORDS):
    k, off = WORDS[s]
    t = text(k, off)
    lines.append('sp_word %d %d %d   # is %s' % (s, k, off, t))
open(path, 'w').write('\n'.join(lines) + '\n')
print('sp_word: %d rows added to %s' % (len(WORDS), path), file=sys.stderr)
