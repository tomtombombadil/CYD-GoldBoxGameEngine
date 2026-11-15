import sys
# mk.py out keys... : load save A in the Dark Queen, then each key with a shot (W<ms> waits, S<name> shot only)
out=sys.argv[1]; keys=sys.argv[2:]
L=['wait 4000','key enter','wait 3000','key p','wait 4000','key l','wait 3000','key t','wait 3000','key a','wait 6000']
n=0
for k in keys:
    if k.startswith('W'): L.append('wait '+k[1:]); continue
    if k.startswith('S'): L.append('shot '+k[1:]); continue
    L += ['key '+k.replace('+',' '),'wait 300','quiet 800 15000','wait 300','shot s%03d_%s'%(n,k)]; n+=1
L.append('quit')
open(out,'w').write('\n'.join(L)+'\n')
