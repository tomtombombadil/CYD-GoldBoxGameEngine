# evfull.py <dis file> <dispatch addr hex> [slot]: an ON GOTO / GOSUB's whole table read from memory (PEEK) - the
# disassembly shows only its first 22 entries; each entry's address and first texts
import re,subprocess,os,sys
P='/tmp/claude-0/knplay'; at=int(sys.argv[2],16); S=sys.argv[3] if len(sys.argv)>3 else 'B'
dis=open(sys.argv[1],errors='replace').read().splitlines()
idx={}
for i,l in enumerate(dis):
    m=re.match(r'([0-9A-F]{4}) ',l)
    if m: idx.setdefault(m.group(1),i)
A=','.join('%X'%(at+i) for i in range(7))
subprocess.run([P+'/bin/p.sh','',f'SAVE={S}',f'PEEK={A}'],env=dict(os.environ,TAILN='0',MEMSET=''),capture_output=True)
b=[int(x.split()[3],16) for x in open(P+'/run.log',errors='replace') if x.startswith('  PEEK ')]
n=b[5]
A=','.join('%X'%(at+i) for i in range(6+3*n))
subprocess.run([P+'/bin/p.sh','',f'SAVE={S}',f'PEEK={A}'],env=dict(os.environ,TAILN='0',MEMSET=''),capture_output=True)
b=[int(x.split()[3],16) for x in open(P+'/run.log',errors='replace') if x.startswith('  PEEK ')]
print('dispatch %04X, %d entries'%(at,n))
for k in range(n):
    o=6+3*k; a='%04X'%(b[o+1]|b[o+2]<<8); t=[]
    for l2 in dis[idx.get(a,len(dis)):idx.get(a,len(dis))+60]:
        if '"' in l2 and (' op 12' in l2 or ' op 11' in l2): t.append(l2.split('"')[1][:44])
        if len(t)>=2: break
    print('  %02X %s %s'%(k,a,' | '.join(t)))
