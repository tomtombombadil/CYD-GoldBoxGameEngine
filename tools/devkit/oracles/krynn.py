#!/usr/bin/env python3
# The Krynn games' creation roll as their createPlayer codes it (Champions,
# Death Knights): the rows b_krynn_stats (behave_oracle.inc) checks -
# statlim, agebr, classmin (data segment tables), ageeff (each stat's age
# effects: the bracket word compared with the age, jnc = older than it, jc
# on the first = the young), raceadj (the adjustment of each 3d6 + 1 roll,
# run through the code for every race and stat), adjtested (the race the
# code really tests: the race-list loop's last counter), wis13 (the classes
# in the Pascal set the Wisdom-13 rule tests).
import re, sys
import x86


def stats_rows(w, ds, c, ovr_file, ovr_base, *, statlim, agebr, classmin, stat_switch, adj, tested, wis_set,
               races=7, classes=17):
    for race in range(races):
        w.row('statlim', race, list(ds.bytes(statlim + race * 16, 16)))
        w.row('agebr', race, [ds.u16(agebr + race * 10 + i * 2) for i in range(5)])
    for cls in range(classes):
        w.row('classmin', cls, list(ds.bytes(classmin + cls * 6, 6)))
    # the stat blocks: cmp al,s / jz block / jmp next
    lo, hi = stat_switch
    blocks = {}
    for a in c.find(r'^cmp al,0x[0-5]$', lo, hi):
        j = c.after(a)
        if j[2].startswith('jz') and c.after(a, 2)[2].startswith('jmp'):
            blocks.setdefault(c.imm(a), (x86.num(j[2].split()[-1]), x86.num(c.after(a, 2)[2].split()[-1])))
    words = ['0x%x' % (agebr + 2 * b) for b in range(4)]
    for s in range(6):
        st, en = blocks[s]
        ef = [0, 0, 0, 0, 0]
        for a in c.find(r'^mov ax,\[di\+(%s)\]$' % '|'.join(words), st, en):
            b = words.index(re.search(r'(0x[0-9a-f]+)\]', c.text(a)).group(1))
            k = 1
            while not c.after(a, k)[2].startswith('j'):
                k += 1
            j = c.after(a, k)[2].split()[0]
            while not re.match(r'(inc|dec|add|sub) byte \[es:di\+0x11\]', c.after(a, k)[2]):
                k += 1
            t = c.after(a, k)[2]
            d = (1 if t.startswith('inc') else -1 if t.startswith('dec') else
                 (1 if t.startswith('add') else -1) * x86.num(t.split(',')[-1]))
            if j == 'jnc':
                ef[b] += d
            elif j == 'jc' and b == 0:
                ef[4] += d
            else:
                sys.exit('age effect: unexpected %s at %X' % (j, a))
        w.row('ageeff', s, ef)
    tlo, thi, tvar = tested
    w.row('adjtested', c.imm(c.find(r'^cmp byte \[%s\],0x' % re.escape(tvar), tlo, thi)[0]))
    alo, ahi, race_var, stat_var, roll_var = adj
    for race in range(races):
        row = []
        for s in range(6):
            m = x86.run(c, alo, ahi, {'al': 0}, {race_var: race, stat_var: s})
            row.append(m.get(roll_var, 0))
        w.row('raceadj', race, row)
    at = ovr_base + wis_set
    bits = open(ovr_file, 'rb').read()[at:at + 32]
    w.row('wis13', [k for k in range(256) if bits[k // 8] >> (k % 8) & 1])
