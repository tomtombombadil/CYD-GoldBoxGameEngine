#!/usr/bin/env python3
# wrate_emu.py [game ...] - rules round 21 (2026-11-12): the computer's weapon
# rating of each DAX game RUN in Unicorn (emu16.py) on random items, for
# sweep's `behave rules4 ONLY=wrate` to compare with combat::ai_rating.
# The routine is found by wrating.py's way (its paralysing-gaze test); the
# item record, the fighter record (hands in use, alignment) and the target
# (Curse-type: fighter -> combatant -> target record, its undead byte) are
# laid out at the offsets the routine itself reads (parsed from its code);
# the item types come from the game's own ITEMS file, poked where the
# routine reads them. Darkness / Treasures' rating is a nested procedure:
# its parent's locals (the plus a blow / a missile needs, the two halvings)
# are set from the target's effects as the parent's code sets them (GAME.OVR
# 0xAFAA / 0xBE48 - read, not run: 0x7D 2, 0x3C 3, 0x50 / 0x51 / 0x67 1,
# the missile's the same or 1 for 0x29; 0x42 / 0x43 the halvings).
# Output /tmp/claude-0/bt/oracle_wrate_<game>.txt, a line each:
#   rate <type> <plus> <cursed> <x3d> <x3e> <align> <hands> <undead> <effects|-> <rating>
# Needs unicorn (emu16.py says how).
import os, random, re, struct, subprocess, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gb
import emu16

GAMES = {  # program, DS:0 in its image, ITEMS head / row
    'poolrad': ('START.EXE', 0xC7C0, 2, 16), 'curse': ('START.EXE', 0x48D0, 2, 16),
    'secret': ('START.EXE', 0xDE20, 2, 16), 'darkness': ('GAME.EXE', 0x9E70, 0, 16),
    'gateway': ('GAME.EXE', 0x4C70, 2, 24, 'ITEMS.1'), 'treasures': ('GAME.EXE', 0xC3E0, 0, 16),
    'champions': ('START.EXE', 0xBC60, 2, 16), 'knights': ('START.EXE', 0xBCE0, 2, 16),
}
FX = [0x7D, 0x3C, 0x50, 0x51, 0x67, 0x29, 0x42, 0x43]


def dis(data, start, n):
    out = subprocess.run(['ndisasm', '-b16', '-o', str(start), '-e', str(start), '-'], input=data[:start + n],
                         capture_output=True).stdout.decode().splitlines()
    return [(int(l[:8], 16), l[28:].strip()) for l in out if len(l) > 28]


def overlay_base(img, foff):
    best = 0
    for seg in range(len(img) // 16):
        if img[seg * 16:seg * 16 + 2] == b'\xcd\x3f':
            fo = struct.unpack_from('<I', img, seg * 16 + 4)[0]
            if best < fo <= foff:
                best = fo
    return best


def run_game(game, n, out):
    exe, ds, head, row = GAMES[game][:4]
    tfile = GAMES[game][4] if len(GAMES[game]) > 4 else 'ITEMS'
    ovr = open(os.path.join(gb.folder(game), 'GAME.OVR'), 'rb').read()
    img = gb.image(game, exe)
    items = open(gb.find(game, tfile), 'rb').read()[head:]
    at = ovr.find(bytes.fromhex('26807d3d53'))
    fn = max(ovr.rfind(b'\x55\x89\xe5', 0, at), ovr.rfind(b'\x55\x8b\xec', 0, at))
    lines = []
    for a, ins in dis(ovr, fn, 0x200):
        lines.append(ins)
        if ins.startswith('retf') or ins.startswith('db 0xca'):
            break
    txt = '\n'.join(lines)
    nested = 'ss:di+0xa' in txt
    table = int(re.search(r'add di,(0x[0-9a-f]+)\npush ds', txt).group(1), 16)
    farcalls = re.findall(r'call (0x[0-9a-f]+):(0x[0-9a-f]+)', txt)
    k3 = lines.index('cmp ax,0x3')
    hands = int(re.search(r'\+(0x[0-9a-f]+)\]', [l for l in lines[:k3] if 'mov al,[es:di+' in l][-1]).group(1), 16)
    k84 = next(k for k, l in enumerate(lines) if 'cmp byte [es:di+0x3e],0x84' in l)
    align = int(re.search(r'cmp al,\[es:di\+(0x[0-9a-f]+)\]', '\n'.join(lines[k84:k84 + 8])).group(1), 16)
    holy = re.search(r'cmp byte \[es:di\+0x2e\],(0x[0-9a-f]+)', txt)
    chain = undead = None
    if holy:
        kh = next(k for k, l in enumerate(lines) if 'cmp byte [es:di+0x2e],' in l)
        seg_ = '\n'.join(lines[kh:kh + 14])
        chain = int(re.search(r'les di,\[es:di\+(0x[0-9a-f]+)\]', seg_).group(1), 16)
        undead = int(re.search(r'cmp byte \[es:di\+(0x[0-9a-f]+)\],0x0', seg_).group(1), 16)
    base = overlay_base(img, fn)
    r = emu16.Routine(ovr, base, img, ds, pascal=True)
    r.poke_ds(table, items[:0x10000 - table] if table + len(items) > 0x10000 else items)
    move = None
    for s, o in farcalls:
        s, o = int(s, 16), int(o, 16)
        if (s, o) in ((0x343, 0x4DF), (0xA77, 0x4DF)):
            r.helper(s, o, lambda e: 0, 0)          # (the stack check: nothing)
        elif move is None:
            move = (s, o)

            def h_move(e):
                cnt, doff, dseg, soff, sseg = e.args(5)
                e.mu.mem_write(dseg * 16 + doff, bytes(e.mu.mem_read(sseg * 16 + soff, cnt)))
                return 0
            r.helper(s, o, h_move, 10)
    rows = len(items) // row
    rng = random.Random(21)
    print('%s: rating 0x%x (overlay 0x%x), types DS 0x%x (%d), hands +0x%x, alignment +0x%x, holy %s chain %s undead %s%s'
          % (game, fn, base, table, rows, hands, align, holy.group(1) if holy else '-', hex(chain) if chain else '-',
             hex(undead) if undead else '-', ', nested' if nested else ''))
    for _ in range(n):
        t = rng.randrange(1, min(rows, 256))
        if holy and rng.random() < 0.15:
            t = int(holy.group(1), 16)
        plus = rng.choice([-2, -1, 0, 0, 0, 1, 1, 2, 3, 4, 5])
        cursed = 1 if rng.random() < 0.08 else 0
        x3e = 0x84 if rng.random() < 0.15 else rng.choice([0, 0, 0x10, 0x20])
        x3d = 0x53 if rng.random() < 0.1 else rng.randrange(0, 16)
        al = rng.randrange(0, 9)
        hu = rng.randrange(0, 4)
        und = rng.choice([0, 0, 1, 2]) if holy else 0
        effs = sorted(rng.sample(FX, rng.randrange(0, 3))) if nested else []
        item = bytearray(0x40)
        item[0x2E], item[0x32], item[0x36], item[0x3D], item[0x3E] = t, plus & 0xFF, cursed, x3d, x3e
        r.record(0, item)
        fig = bytearray(0x400)
        fig[hands], fig[align] = hu, al
        if chain:
            struct.pack_into('<HH', fig, chain, 0x200, emu16.R2)       # the combatant at R2:0x200
            struct.pack_into('<HH', fig, 0x200 + 0xA, 0x300, emu16.R2)  # its target's record at R2:0x300
            fig[0x300 + undead] = und
        r.record(1, fig)
        if nested:
            P = 0x8000
            need = 0
            if 0x7D in effs: need = 2
            if 0x3C in effs: need = 3
            if {0x50, 0x51, 0x67} & set(effs): need = 1
            far = need or (1 if 0x29 in effs else 0)
            loc = {P - 0x1C: need, P - 0x1D: far, P - 0x26: int(0x42 in effs), P - 0x25: int(0x43 in effs)}
            for a, v in loc.items():
                r.mu.mem_write(emu16.SS * 16 + a, bytes([v]))
            r.mu.mem_write(emu16.SS * 16 + P + 0xA, struct.pack('<HH', 0, emu16.R2))
            got = r.call(fn, [emu16.R1, 0, P]) & 0xFF
        else:
            got = r.call(fn, [emu16.R2, 0, emu16.R1, 0]) & 0xFF
        out.write('rate %d %d %d %d %d %d %d %d %s %d\n' % (t, plus, cursed, x3d, x3e, al, hu, und,
                                                            ','.join('%x' % e for e in effs) or '-', got))


def run_queen(n, out):
    # The Dark Queen (DQK.EXE, C): the rating 0x2F081 (fighter, item) - its
    # code segment taken at file 0x20000 (near calls land right; only the
    # offsets within it matter), DS:0 at file 0x63A20; the type table through
    # the far pointer DS 0xAE3E (put at 0x4000:0, ITEMS.DAT's rows), the game
    # block's through DS 0xB20F (0x4800:0, byte 0x3C the "no ranged" byte);
    # the chooser 0x2F1A6 sets DS 0xACA8-0xACAB from the target's effects as
    # Darkness's parent (read, not run); the item struct: type +0x28, plus
    # +0x30, cursed +0x34; the fighter's hands in use +0xCA; memcpy
    # 0xD7:0x2B69 (count in CX)
    exe = open(os.path.join(gb.folder('queen'), 'DQK.EXE'), 'rb').read()
    items = open(os.path.join(gb.folder('queen'), 'DISK1', 'ITEMS.DAT'), 'rb').read()
    from unicorn.x86_const import UC_X86_REG_CX
    fn = 0x2F081
    if exe[fn:fn + 3] != b'\x55\x8b\xec':
        raise SystemExit('queen: no routine at 0x%x' % fn)
    r = emu16.Routine(exe, 0x20000, exe, 0x63A20, pascal=False)
    TS, GS = 0x4000, 0x4800
    r.mu.mem_write(TS * 16, items[:0x8000])
    r.poke_ds(0xAE3E, struct.pack('<HH', 0, TS))
    r.poke_ds(0xB20F, struct.pack('<HH', 0, GS))

    def h_memcpy(e):
        soff, sseg, doff, dseg = e.args(4)
        cnt = e.mu.reg_read(UC_X86_REG_CX)
        e.mu.mem_write(dseg * 16 + doff, bytes(e.mu.mem_read(sseg * 16 + soff, cnt)))
        return 0
    r.helper(0xD7, 0x2B69, h_memcpy, 0)
    rows = len(items) // 16
    rng = random.Random(21)
    print('queen: rating 0x%x, types %d rows' % (fn, rows))
    for _ in range(n):
        t = rng.randrange(1, rows)
        plus = rng.choice([-2, -1, 0, 0, 0, 1, 1, 2, 3, 4, 5])
        cursed = 1 if rng.random() < 0.08 else 0
        hu = rng.randrange(0, 4)
        effs = sorted(rng.sample(FX, rng.randrange(0, 3)))
        ban = 1 if rng.random() < 0.15 else 0
        item = bytearray(0x40)
        item[0x28], item[0x30], item[0x34] = t, plus & 0xFF, cursed
        r.record(0, item)
        fig = bytearray(0x400)
        fig[0xCA] = hu
        r.record(1, fig)
        need = 0
        if 0x7D in effs: need = 2
        if 0x3C in effs: need = 3
        if {0x50, 0x51, 0x67} & set(effs): need = 1
        far = need or (1 if 0x29 in effs else 0)
        r.poke_ds(0xACA8, bytes([int(0x42 in effs), need, int(0x43 in effs), far]))
        r.mu.mem_write(GS * 16 + 0x3C, bytes([ban]))
        got = r.call(fn, [emu16.R1, 0, emu16.R2, 0]) & 0xFF
        # (the line as the others': no effect bytes, alignment or undead; the
        # "no ranged" byte after the effects)
        out.write('rate %d %d %d 0 0 0 %d 0 %s %d ban %d\n' % (t, plus, cursed, hu, ','.join('%x' % e for e in effs) or '-',
                                                              got, ban))


if __name__ == '__main__':
    games = sys.argv[1:] or list(GAMES) + ['queen']
    os.makedirs('/tmp/claude-0/bt', exist_ok=True)
    for g in games:
        with open('/tmp/claude-0/bt/oracle_wrate_%s.txt' % g, 'w') as f:
            (run_queen if g == 'queen' else lambda n, o: run_game(g, n, o))(int(os.environ.get('N', '2000')), f)
