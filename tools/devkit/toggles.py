#!/usr/bin/env python3
"""The toggle checks' report (toggles.sh's logs).

toggles.py <log dir> <base tag> <off tag | each> [fixlist file]

For every game / suite: each check's switches (the base run's [sw ...]), and
the checks whose result changed in the run with the switches held off. A
switch is "verified" in a game when a check that asked it passes with it on and
fails with it held off (the fixed / 1E behaviour seen - and the original's,
where the check tests it both ways). The fixlist (sweep FIXLIST=1's output)
gives the names and each switch's games.
Build of the binary: tools/devkit/build.sh tg "-DCYD_TEST_HOOKS -DBEHAVE_CLASSES
-DBEHAVE_ITEMS -DBEHAVE_SPELLS -DBEHAVE_MONSTERS -DBEHAVE_EVENTS -DBEHAVE_SAVES
-DBEHAVE_POOL -DBEHAVE_SECRET -DBEHAVE_DARKNESS -DBEHAVE_TREASURES
-DBEHAVE_CHAMPIONS -DBEHAVE_DEATHKNIGHTS -DBEHAVE_GATEWAY -DBEHAVE_QUEEN
-DBEHAVE_MATRIX -DBEHAVE_ROUND4 -DBEHAVE_LEVELS -DBEHAVE_ECONOMY
-DBEHAVE_FEATURES" tools/playsim/sweep.cpp
"""
import glob
import os
import re
import sys
from collections import defaultdict

GAMES = {"poolrad": 1, "curse": 2, "secret": 3, "darkness": 4, "champions": 5, "knights": 6, "queen": 7,
         "gateway": 8, "treasures": 9}
LINE = re.compile(r"^(PASS|FAIL) (.*?)(?: \[sw ([^\]]*)\])?$")


def read(path):
    """[(key, ok, switches)] - key = 'part: what' with its occurrence number"""
    out, seen = [], defaultdict(int)
    for ln in open(path, errors="replace"):
        m = LINE.match(ln.rstrip("\n"))
        if not m:
            continue
        body = m.group(2)
        what = body.split(" - ")[0]
        seen[what] += 1
        out.append((f"{what} #{seen[what]}", m.group(1) == "PASS", set(m.group(3).split(",")) if m.group(3) else set()))
    return out


def each(d, base, names, games_of):
    """toggles_each.sh's runs: s_<switch>_<game>_<part>.log against the base -
    every check that changed is that switch's doing"""
    keys = sorted({os.path.basename(p).split("_")[1] for p in glob.glob(os.path.join(d, "s_*.log"))},
                  key=lambda s: (s[0], int(s[1:], 16 if s[0] != "F" else 10)))
    for s in keys:
        mask = games_of.get(s, 0)
        per = defaultdict(list)
        other = defaultdict(list)
        for op in glob.glob(os.path.join(d, f"s_{s}_*.log")):
            tail = os.path.basename(op)[len(f"s_{s}_"):-4]
            g, part = tail.split("_", 1)
            bp = os.path.join(d, f"{base}_{tail}.log")
            if not os.path.exists(bp):
                continue
            b = {k: ok for k, ok, _ in read(bp)}
            for k, ok, _ in read(op):
                if k in b and b[k] != ok:
                    (per if not mask or mask >> GAMES[g] & 1 else other)[g].append(f"{part}: {k} ({'now passes' if ok else 'fails'})")
        own = [g for g, n in GAMES.items() if mask >> n & 1] if mask else sorted(per)
        print(f"{s:4} {names.get(s, '?'):32} " + " ".join(f"{g}:{len(per[g]) or '-'}" for g in own)
              + (f"   !! changes OUTSIDE its games: {dict((g, len(v)) for g, v in other.items())}" if other else ""))
        for g in own:
            for c in per[g][:4]:
                print(f"       {g}: {c}")
        for g, v in other.items():
            for c in v[:4]:
                print(f"    !! {g}: {c}")


def main():
    d, base, off = sys.argv[1], sys.argv[2], sys.argv[3]
    names, games_of = {}, {}
    if len(sys.argv) > 4:
        for ln in open(sys.argv[4]):
            m = re.match(r"^([FA][0-9a-f]+) (.*) games ([0-9a-f]+)$", ln.strip())
            if m:
                names[m.group(1)] = m.group(2)
                games_of[m.group(1)] = int(m.group(3), 16)
    names.setdefault("E1", "Coin Exchange")
    names.setdefault("E2", "Flasks of Oil")
    if off == "each":
        each(d, base, names, games_of)
        return
    asked = defaultdict(set)         # switch -> games where a check asked it
    flipped = defaultdict(lambda: defaultdict(list))    # switch -> game -> checks that failed with it off
    unattributed = []
    for bp in sorted(glob.glob(os.path.join(d, f"{base}_*.log"))):
        tail = os.path.basename(bp)[len(base) + 1:-4]
        g, part = tail.split("_", 1)
        op = os.path.join(d, f"{off}_{tail}.log")
        b = read(bp)
        for _, _, sw in b:
            for s in sw:
                asked[s].add(g)
        if not os.path.exists(op):
            continue
        o = {k: ok for k, ok, _ in read(op)}
        for k, ok, sw in b:
            if ok and k in o and not o[k]:
                if len(sw) == 1:
                    flipped[next(iter(sw))][g].append(f"{part}: {k}")
                else:
                    unattributed.append((g, part, k, ",".join(sorted(sw))))
    print("== per switch (games: its own; asked = a check asked it there; verified = a check failed with it off)")
    keys = sorted(set(names) | set(asked) | set(flipped), key=lambda s: (s[0], int(s[1:], 16)))
    for s in keys:
        mask = games_of.get(s, 0)
        own = [g for g, n in GAMES.items() if mask >> n & 1] if mask else sorted(asked[s])
        row = []
        for g in own:
            n = len(flipped[s].get(g, []))
            row.append(f"{g}:{'VERIFIED(' + str(n) + ')' if n else 'asked' if g in asked[s] else '-'}")
        print(f"{s:4} {names.get(s, '?'):32} {' '.join(row)}")
        for g in own:
            for c in flipped[s].get(g, [])[:3]:
                print(f"       {g}: {c}")
    print(f"== checks that failed with several switches asked: {len(unattributed)}")
    for u in unattributed[:200]:
        print("  ", *u)


main()
