# evpeek.py <dis file> [slot]: every ON GOTO on the square's event word (00BF) - each index's address and first texts (PEEK)
import re,subprocess,os,sys
P='/tmp/claude-0/dplay'; F=sys.argv[1]; S=sys.argv[2] if len(sys.argv)>2 else 'B'
dis=open(F,errors='replace').read().splitlines()
idx={}
for i,l in enumerate(dis):
    m=re.match(r'([0-9A-F]{4}) ',l)
    if m: idx.setdefault(m.group(1),i)
for l in dis:
    m=re.match(r'([0-9A-F]{4}) op 25: 1:00BF 0:([0-9A-F]{4})',l)
    if not m: continue
    at=int(m.group(1),16); n=int(m.group(2),16)
    A=','.join('%X'%(at+i) for i in range(6+3*n))
    subprocess.run([P+'/bin/p.sh','',f'SAVE={S}',f'PEEK={A}'],env=dict(os.environ,TAILN='0',MEMSET=''),capture_output=True)
    b=[int(x.split()[3],16) for x in open(P+'/run.log',errors='replace') if x.startswith('  PEEK ')]
    print('dispatch %04X, %d'%(at,n))
    for k in range(n):
        o=6+3*k; a='%04X'%(b[o+1]|b[o+2]<<8); t=[]
        if a in idx:
            for l2 in dis[idx[a]:idx[a]+60]:
                if '"' in l2 and (' op 12' in l2 or ' op 11' in l2): t.append(l2.split('"')[1][:42])
                if len(t)>=2: break
        print('  %02X %s %s'%(k,a,' | '.join(t)))
