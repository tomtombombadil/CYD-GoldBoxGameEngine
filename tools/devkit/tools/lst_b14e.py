#!/usr/bin/env python3
# lst_b14e.py <DQK.EXE> <ndisasm listing> [call text]
# Every call of the Dark Queen's fight-page print res 0xB14E (fighter, text, row,
# wait) - far call 0x77e:0x156e - with the arguments it pushes: wait, row
# (an immediate, or the instruction that makes it), the text (a DS string read
# from DQK.EXE when it is a constant). Facts only - prints addresses and the
# program's own short words for the reader, nothing is written.
import re, sys
exe = open(sys.argv[1], 'rb').read()
L = open(sys.argv[2]).read().split('\n')
call = sys.argv[3] if len(sys.argv) > 3 else 'call 0x77e:0x156e'
DS = 0x63A20

def ds_str(a):
    o = DS + a
    e = exe.find(b'\0', o, o + 80)
    return exe[o:e].decode('latin-1') if e > 0 else '?'

for i, l in enumerate(L):
    if call not in l:
        continue
    # walk back: collect the arguments pushed (the last pushed is the first)
    args = []
    j = i - 1
    last_val = None
    while j > i - 40 and len(args) < 6:
        t = L[j][28:].strip() if len(L[j]) > 28 else ''
        if t.startswith('call') or t.startswith('ret') or t.startswith('jmp'):
            break
        if t.startswith('push '):
            src = t[5:]
            # the value: an immediate loaded just before (mov al/ax,imm)
            k = j - 1
            val = src
            if src in ('ax', 'bx', 'dx', 'cx'):
                while k > j - 6:
                    tk = L[k][28:].strip()
                    m = re.match(r'mov (al|ax|bl|bx|dl|dx),(.+)$', tk)
                    if m and m.group(1)[0] == src[0]:
                        val = m.group(2)
                        break
                    if tk.startswith(('lea', 'inc', 'add', 'sub', 'les', 'mov')) and src[0] in tk.split(',')[0]:
                        val = tk
                        break
                    k -= 1
            args.append(val)
        j -= 1
    # args (nearest the call first): [0] fighter off, [1] fighter seg, [2] text off, [3] text seg, [4] row, [5] wait
    wait = args[5] if len(args) > 5 else '?'
    row = args[4] if len(args) > 4 else '?'
    text = args[2] if len(args) > 2 else '?'
    s = ''
    m = re.match(r'0x([0-9a-f]+)$', text)
    if m and len(args) > 3 and args[3] == 'ds':
        s = ds_str(int(m.group(1), 16))
    print('%s  wait %-6s row %-28s text %-14s %s' % (l[:8], wait, row, text, s))
