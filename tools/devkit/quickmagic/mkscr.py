import sys
# mkscr.py out keys... : boot (2 enters) then each key with a shot
out=sys.argv[1]; keys=sys.argv[2:]
lines=['wait 3000','quiet 1500 30000','key enter','quiet 1000 20000','key enter','quiet 1500 20000','wait 2000']
n=0
for k in keys:
    if k.startswith('W'): lines.append('wait '+k[1:]); continue
    if k.startswith('P'): lines.append(k[1:].replace('_',' ')); continue
    lines += ['key '+k,'wait 300','quiet 1000 15000','wait 500','shot s%02d_%s'%(n,k)]; n+=1
lines.append('quit')
open(out,'w').write('\n'.join(lines)+'\n')
