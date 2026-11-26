# evinline.py: the big ON GOTO / GOSUB dispatches (op 25 with the addresses inline, 8 or more) of ev_dis.txt -
# event number -> address -> first text (the Queen's later scripts dispatch their squares so: script 14's 1:009B)
import re
dis=open('/tmp/claude-0/qplay/ev_dis.txt',errors='replace').read().splitlines()
idx={}
for i,l in enumerate(dis):
    m=re.match(r'([0-9A-F]{4}) ',l)
    if m: idx.setdefault(m.group(1),i)
for l in dis:
    m=re.match(r'([0-9A-F]{4}) op 25: 1:([0-9A-F]{4}) 0:([0-9A-F]{4})',l)
    if not m or 'RAW' not in l: continue
    n=int(m.group(3),16)
    if n<8: continue
    raw=[int(x,16) for x in l.split('RAW')[1].split()]
    p=6; print('dispatch at %s on %s, %d events'%(m.group(1),m.group(2),n))
    for k in range(n):
        if p+2>=len(raw): break
        a=raw[p+1]|raw[p+2]<<8; p+=3; h='%04X'%a; t=''
        if h in idx:
            for q in dis[idx[h]:idx[h]+25]:
                if '"' in q: t=q.split('"')[1][:70]; break
        print('  %02X %s %s'%(k,h,t))
