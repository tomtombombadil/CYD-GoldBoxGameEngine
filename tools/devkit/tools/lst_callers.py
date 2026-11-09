#!/usr/bin/env python3
import sys, re
# lst_callers.py <listing> <call text> [n before]: each call site with the lines before it
L = open(sys.argv[1]).read().split('\n')
pat = sys.argv[2]; nb = int(sys.argv[3]) if len(sys.argv) > 3 else 10
for i, l in enumerate(L):
    if pat in l:
        print('=====')
        print('\n'.join(L[max(0, i - nb):i + 1]))
