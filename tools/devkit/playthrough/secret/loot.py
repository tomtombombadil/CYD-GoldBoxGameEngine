#!/usr/bin/env python3
# loot.py <slot> [members]: Tom's treasure rule (2026-11-16) through the Merchant cheat, member by member:
#  - an unreadied item with a plain name the shops sell (Long Sword, Chain Mail ...): Sell's offer read - more
#    than the plain item's (its shop value / 2) means magic: No, and Id it; a plain offer: Yes (sold);
#  - an unidentified generic one (Potion, Ring, Wand, Scroll, Dust, Bracers, Cloak ...): Id;
#  - missile weapons and ammunition, named magic items and readied items are kept.
# Each step is its own run (load, act, leave the shop, save to the slot). Prints what it did.
import re, subprocess, sys
P = '/tmp/claude-0/splay'; S = sys.argv[1]
members = [int(x) for x in sys.argv[2].split(',')] if len(sys.argv) > 2 else [0, 1, 2, 3, 4, 5]
KEEP = ('Composite Long Bow', 'Arrow', 'Sling', 'Quarrel', 'Dart', 'Scroll', 'Mirror', 'Potion', 'Javelin', 'Chain Mail')
GENERIC = ('Potion', 'Ring', 'Wand', 'Scroll', 'Dust', 'Bracers', 'Cloak', 'Necklace', 'Gauntlets', 'Boots',
           'Girdle', 'Robe', 'Helm', 'Stone', 'Book', 'Rod', 'Staff', 'Amulet', 'Medallion', 'Jewelry', 'Gem')


def run(t, *env):
    subprocess.run([P + '/bin/p.sh', t] + [f'SAVE={S}'] + list(env), env={'TAILN': '0', 'PATH': '/usr/bin:/bin'},
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return open(P + '/run.log', errors='replace').read()


log = run('', 'CHEAT=1', 'GOODSLOG=1')
plain = {}
for m in re.finditer(r'  good +\d+: (.+?) +type +(\d+) plus 0 value (\d+)', log):
    nm = m.group(1).strip()
    if (any(nm.startswith(g) for g in GENERIC) and not nm.endswith(' Mail')) or 'of ' in nm: continue   # (potions, wands ... never sold blind)
    plain[nm] = int(m.group(3))


def items_of(k):
    log = run('', 'ITEMDUMP=1')
    blk = re.search(r'  member %d .*?\n((?:    .*\n)*)' % k, log)
    out = []
    if not blk: return out
    for line in blk.group(1).splitlines():
        out.append((line[4] == '*', line[6:].strip()))
    return out


EXIT = '/E/E/E'
import os
SELL = [x for x in os.environ.get('SELL', '').split(',') if x]
# SELLN="m:Name:n,...": sell at most n of that name from member m (the rest kept - e.g. potions past two)
SELLN = {}
for x in [y for y in os.environ.get('SELLN', '').split(',') if y]:
    m_, nm_, n_ = x.split(':'); SELLN[(int(m_), nm_)] = int(n_)   # (names sold whatever they are: magic no one uses)
for k in members:
    i = len(items_of(k)) - 1
    while i >= 0:
        its = items_of(k)
        if i >= len(its): i = len(its) - 1; continue
        ready, name = its[i]
        base = re.sub(r'^\d+ ', '', name)
        known = bool(re.search(r' [+-]\d', base))            # (identified magic: kept for the hand-out)
        base = re.sub(r'^Drow ', '', re.sub(r' [+-]\d.*$', '', base))
        sel = '"1%%%d/V/I' % k + 'v' * i
        # (Tom, 2026-11-19: magic armour and protection are never sold - SELL passes them over: a spare suit,
        # ring or bracers is the next upgrade for someone)
        protect = known and re.search(r'Mail|Armor|Shield|Ring|Bracers|Cloak|Robe|Girdle|Gauntlets|Boots|Helm', base)
        if not ready and name in SELL and protect:
            print(f'  {k}: {name} - magic armour / protection, kept'); i -= 1; continue
        if not ready and SELLN.get((k, name), 0) > 0:
            SELLN[(k, name)] -= 1
            run(sel + '/S/Y' + EXIT, f'SAVENOW={S}'); print(f'  {k}: {name} sold (SELLN)'); i -= 1; continue
        if not ready and name in SELL:
            run(sel + '/S/Y' + EXIT, f'SAVENOW={S}'); print(f'  {k}: {name} sold (SELL)'); i -= 1; continue
        if ready or known or any(w in name for w in KEEP):
            i -= 1; continue
        if base in plain:
            log = run(sel + '/S', 'ALLTEXT=1')
            offer = re.findall(r'sell offer (\d+)', log)
            off = int(offer[-1]) if offer else -1
            normal = plain[base] // 2
            if off < 0: print(f'  {k}: {name} - no offer read, left'); i -= 1; continue
            if off == 0 and 'Drow' in name and sum(1 for r, n in its if 'Drow' in n and n.split()[-1] == name.split()[-1]) < 2:
                print(f'  {k}: {name} - the only one, kept'); i -= 1; continue
            if off > normal:
                run(sel + '/S/N/I/Y' + EXIT, f'SAVENOW={S}')
                print(f'  {k}: {name} offered {off} (plain {normal}) - identified as "{items_of(k)[i][1]}"')
            else:
                run(sel + '/S/Y' + EXIT, f'SAVENOW={S}')
                print(f'  {k}: {name} sold for {off}')
        elif base in GENERIC or base.split()[0] in GENERIC and len(base.split()) == 1 or os.environ.get('IDALL'):
            # (IDALL=1: a name the shops don't sell - Secret's shops sell little - is identified too)
            run(sel + '/I/Y' + EXIT, f'SAVENOW={S}')
            print(f'  {k}: {name} identified as "{items_of(k)[i][1]}"')
        i -= 1
