# ovpath.py <x,y> [blocked codes]: tokens for the shortest 8-way walk on Darkness's overland (ov17.txt) from the party
# (PEEK 0025 / 0026 / 0023) to x,y - "/M" then turns (R / L = 45 degrees) and F; codes in blocked (default 3,4) avoided
import subprocess,os,sys,collections
P='/tmp/claude-0/dplay'; S=os.environ.get('S','B')
tx,ty=map(int,sys.argv[1].split(',')); bl=set(map(int,(sys.argv[2] if len(sys.argv)>2 else '3,4').split(',')))
g=[list(map(int,l.split())) for l in open(P+'/ov17.txt')]
subprocess.run([P+'/bin/p.sh','',f'SAVE={S}','PEEK=0025,0026,0023'],env=dict(os.environ,TAILN='0'),capture_output=True)
v=[int(l.split()[3],16) for l in open(P+'/run.log') if l.startswith('  PEEK ')]
sx,sy,f=v
D=[(0,-1),(1,-1),(1,0),(1,1),(0,1),(-1,1),(-1,0),(-1,-1)]
prev={(sx,sy):None}; q=collections.deque([(sx,sy)])
while q:
    c=q.popleft()
    if c==(tx,ty): break
    for d,(dx,dy) in enumerate(D):
        n=(c[0]+dx,c[1]+dy)
        if 0<=n[0]<38 and 0<=n[1]<15 and n not in prev and ((g[n[1]][n[0]] not in bl and not 10<=g[n[1]][n[0]]<=int(os.environ.get("OVPLACEMAX","30"))) or n==(tx,ty)):
            prev[n]=(c,d); q.append(n)
if (tx,ty) not in prev: print('no way'); sys.exit(1)
steps=[]; c=(tx,ty)
while prev[c]: c,d=prev[c]; steps.append(d)
steps.reverse(); t='/M'
for d in steps:
    r=(d-f)%8; t+= 'R'*r if r<=4 else 'L'*(8-r); t+='F'; f=d
print(t)
