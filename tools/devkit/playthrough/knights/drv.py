#!/usr/bin/env python3
# drv.py <slot> <start tokens> [seed]: a fight played by a simple policy of Tom's tactics: casters Magic
# Missile the hurt-most foe in reach (else Fireball a foe 4+ squares from every member, else wait), Bran
# Prayer then cures, fighters attack the weakest adjacent foe else guard; prints the result
import re,subprocess,sys,os
P='/tmp/claude-0/knplay'; slot=sys.argv[1]; T=sys.argv[2]; seed=sys.argv[3] if len(sys.argv)>3 else ''
env=dict(os.environ); 
if seed: env['SEED']=seed
mm={'MIRA':2,'SELENE':2}; mmidx={'MIRA':0,'SELENE':4}; fb={'MIRA':2,'SELENE':1}; prayed=False
def run(t):
    r=subprocess.run([P+'/bin/p.sh',t+'?[',f'SAVE={slot}','PAGELOG=1'],env=dict(env,TAILN='0'),stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
    if b'egmentation' in r.stderr or 'ERROR: AddressSanitizer' in open(P+'/run.log',errors='replace').read(): open(P+'/crash_tok.txt','w').write(t); print('CRASH at step',len(t))
    return open(P+'/run.log',errors='replace').read()
for step in range(int(os.environ.get("STEPS","120"))):
    log=run(T)
    if '[fight] won' in log: print('WON', re.findall(r'\[fight\] won.*',log)[-1][:90]); break
    st=re.findall(r'  \? screen (\d+) menu \[([^\]]*)\]',log)
    if not st: print('no state'); break
    scr,menu=st[-1]
    if scr!='24': print('OUT', scr, menu[:60]); break
    figs=re.findall(r'  FIG (\d+) (\S) (.+?) at (\d+),(\d+) team (\d) hp (\d+)',log)[-40:]
    n=len(set(f[0] for f in figs)); figs=figs[-n:] if n else figs
    F={int(f[0]):dict(c=f[1],name=f[2],x=int(f[3]),y=int(f[4]),team=int(f[5]),hp=int(f[6])) for f in figs}
    who=(re.findall(r'  \? turn (.+)',log) or [''])[-1].strip()
    me=[k for k,v in F.items() if v['name']==who and v['team']==0]
    foes=[k for k,v in F.items() if v['team']==1 and v['hp']>0]
    party=[k for k,v in F.items() if v['team']==0 and v['hp']>0]
    if 'Continue Battle' in menu: T+='/N,'; continue
    if 'Aim:' in menu:
        tgt=min(foes,key=lambda k:F[k]['hp']) if foes else None
        if 'Target' in menu or tgt is None: T+=f'#@{tgt};/T~/E/Y,~,' if tgt is not None else '/E/Y,'
        else: T+='/E/Y,'
        continue
    if 'Delay' in menu or 'Guard' in menu: T+='/E/Q,'; continue
    if not me: T+='/Q,'; continue
    m=F[me[0]]; d=lambda k: max(abs(F[k]['x']-m['x']),abs(F[k]['y']-m['y']))
    near=[k for k in foes if d(k)<=1]
    a='/Q,'
    if who in ('MIRA','SELENE') and 'Cast' in menu:
        reach=[k for k in foes if d(k)<=7]
        safe=[k for k in foes if all(max(abs(F[k]['x']-F[p]['x']),abs(F[k]['y']-F[p]['y']))>=4 for p in party)]
        if mm[who]>0 and reach:
            t=min(reach,key=lambda k:F[k]['hp']); a='/C'+'v'*mmidx[who]+'/C'+f'#@{t};/T~/E~,'; mm[who]-=1
        elif fb[who]>0 and safe:
            t=safe[0]; idx=7 if who=='MIRA' else 14 - (2-mm[who]) ; a='/C'+'v'*idx+'/C'+f'#@{t};/T~/E~,'; fb[who]-=1
        else: a='/Q,'
    elif who=='BRAN' and 'Cast' in menu and not prayed:
        a='/C'+'v'*7+'/C,'; prayed=True
    elif near:
        t=min(near,key=lambda k:F[k]['hp']); dx=F[t]['x']-m['x']; dy=F[t]['y']-m['y']
        dx=max(-1,min(1,dx)); dy=max(-1,min(1,dy)); dirs={(0,-1):0,(1,-1):1,(1,0):2,(1,1):3,(0,1):4,(-1,1):5,(-1,0):6,(-1,-1):7}
        a='/M}%d,'%dirs.get((dx,dy),0)
    T+=a
open(P+'/ftok.txt','w').write(T)
print('party', [(F[k]['name'],F[k]['hp']) for k in F if F[k]['team']==0])
