import struct,sys,re
# memchk.py: the Magic flag (DS offset) and each named record's memorized list (MEMOFF from the name's length
# byte, default 0x1E = Curse; PoolRad's fight copies: print a wider window - see README)
# memchk.py <mem file> <flag DS offset hex> names...: the flag byte and each named record's memorized list (rec+0x1E)
d=open(sys.argv[1],'rb').read(); ds=struct.unpack_from('<H',d,0)[0]; m=d[2:]
fl=int(sys.argv[2],16); import os; OFF=int(os.environ.get("MEMOFF","0x1E"),16)
print('DS %04X flag[%04X]=%d'%(ds,fl,m[ds*16+fl]), end='')
for nm in sys.argv[3:]:
    b=nm.encode()
    hits=[x.start() for x in re.finditer(re.escape(bytes([len(b)])+b),m)]
    res=[]
    for h in hits:
        res.append(' '.join('%02x'%v for v in m[h+OFF:h+OFF+6]))
    print(' | %s: %s'%(nm,'; '.join(res)),end='')
print()
