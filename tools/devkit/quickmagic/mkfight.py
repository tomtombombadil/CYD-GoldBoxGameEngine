import sys
# mkfight.py out pathkeys(comma) pokes(comma SEG:OFF=V or '-') nq shots_every_ms total_ms
out, path, pokes, nq, every, total = sys.argv[1], sys.argv[2].split(','), sys.argv[3], int(sys.argv[4]), int(sys.argv[5]), int(sys.argv[6])
L=['wait 3000','quiet 1500 30000','key enter','quiet 1000 20000','key enter','quiet 1500 20000','wait 2000']
for k in path: L += ['key '+k,'wait 300','quiet 1000 15000','wait 500']
L += ['shot f00_start']
if pokes != '-':
    for p in pokes.split(','):
        a,v=p.split('='); L.append('poke %s %s'%(a,v))
for i in range(nq):
    L += ['key q','wait 300','quiet 800 15000','shot q%02d'%i]
n=0
for t in range(0,total,every):
    L += ['wait %d'%every,'shot r%03d'%n]; n+=1
L.append('quit')
open(out,'w').write('\n'.join(L)+'\n')
