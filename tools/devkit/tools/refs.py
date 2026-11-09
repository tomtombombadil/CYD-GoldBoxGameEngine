#!/usr/bin/env python3
# refs.py <listing> <text> [before] [after]: every line containing text, with lines round it
import sys
L = open(sys.argv[1]).read().split('\n')
t = sys.argv[2]
b = int(sys.argv[3]) if len(sys.argv) > 3 else 12
a = int(sys.argv[4]) if len(sys.argv) > 4 else 2
for i, l in enumerate(L):
    if t in l:
        print('====')
        print('\n'.join(L[max(0, i - b):i + a]))
