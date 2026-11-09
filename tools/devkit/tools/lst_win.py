#!/usr/bin/env python3
# lst_win.py <ndisasm listing> <address hex> <lines before> <lines after>: the lines round an address
import sys
L=open(sys.argv[1]).read().split('\n')
a=sys.argv[2].upper().zfill(8); b=int(sys.argv[3]); c=int(sys.argv[4])
for i,l in enumerate(L):
    if l.startswith(a):
        print('\n'.join(L[max(0,i-b):i+c])); break
