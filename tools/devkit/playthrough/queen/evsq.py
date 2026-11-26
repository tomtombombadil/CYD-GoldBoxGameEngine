# evsq.py [codes hex,...]: from run.log's MAPFLAGS (run p.sh with MAPFLAGS=1 first) each event code's squares
import re, sys
want = set(int(c, 16) for c in sys.argv[1].split(',')) if len(sys.argv) > 1 else None
rows = [l for l in open('/tmp/claude-0/qplay/run.log') if re.match(r'^\s+\d+: ', l) and len(l.split()) >= 9 and all(re.fullmatch(r'[0-9A-Fa-f]+', c) for c in l.split()[1:])]
sq = {}
for y, l in enumerate(rows):
    for x, c in enumerate(l.split()[1:]):
        v = int(c, 16)
        if v and (want is None or v in want): sq.setdefault(v, []).append((x, y))
for v in sorted(sq): print('%02X' % v, ' '.join('%d,%d' % p for p in sq[v]))
