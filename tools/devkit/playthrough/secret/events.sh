#!/bin/bash
# events.sh <slot>: the current script's square events (event -> address -> first text) from its step dispatch
P=/tmp/claude-0/splay; S=$1
cd $P && TAILN=0 bin/p.sh '' SAVE=$S DISASM=8000,FFFF >/dev/null; cp run.log $P/ev_dis.txt
python3 - <<'PY'
import re,subprocess,os
P='/tmp/claude-0/splay'
dis=open(P+'/ev_dis.txt',errors='replace').read().splitlines()
idx={}
for i,l in enumerate(dis):
    m=re.match(r'([0-9A-F]{4}) ',l)
    if m: idx.setdefault(m.group(1),i)
cands=[]
for i,l in enumerate(dis):
    if 'op 2F:' in l and 'C04F' in l and '003F' in l and '7F79' in l:
        for j in range(i+1,min(i+8,len(dis))):
            m=re.match(r'([0-9A-F]{4}) op 25: 1:7F79 0:([0-9A-F]{4})',dis[j])
            if m: cands.append((int(m.group(1),16),int(m.group(2),16)))
for at,n in cands:
    addrs=','.join('%X'%(at+i) for i in range(6+3*n))
    subprocess.run(['bash',P+'/bin/p.sh','',f'SAVE={os.environ["S"]}',f'PEEK={addrs}'],env=dict(os.environ,TAILN='0',MEMSET=''),capture_output=True)
    b=[int(l.split()[3],16) for l in open(P+'/run.log',errors='replace') if l.startswith('  PEEK ')]
    print('dispatch at %04X, %d events'%(at,n))
    for k in range(n):
        o=6+3*k; a=b[o+1]|b[o+2]<<8; h='%04X'%a; t=''
        if h in idx:
            for l in dis[idx[h]:idx[h]+25]:
                if '"' in l: t=l.split('"')[1][:60]; break
        print('  %02X %s %s'%(k,h,t))
PY
