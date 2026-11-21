# ovgrid.py: Darkness's overland code grid (block 17: rows y at 99B0 + 0x26*y, 38 columns) read by PEEK; written to ov17.txt
import subprocess,os,sys
P='/tmp/claude-0/dplay'; S=sys.argv[1] if len(sys.argv)>1 else 'B'; base=int(sys.argv[2],16) if len(sys.argv)>2 else 0x99B0
A=','.join('%X'%(base+i) for i in range(15*38))
subprocess.run([P+'/bin/p.sh','',f'SAVE={S}',f'PEEK={A}'],env=dict(os.environ,TAILN='0'),capture_output=True)
b=[int(l.split()[3],16)&0xFF for l in open(P+'/run.log') if l.startswith('  PEEK ')]
open(P+'/ov17.txt','w').write('\n'.join(' '.join('%d'%b[y*38+x] for x in range(38)) for y in range(15)))
