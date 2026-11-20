import os,sys
for root,ds,fs in os.walk('/tmp/claude-0/games'):
    for f in fs:
        if f.upper() in ('GAME.OVR','DQK.EXE','GAME.EXE'):
            p=os.path.join(root,f); d=open(p,'rb').read()
            r=[hex(d.find(w)) for w in [b'\x04Bash',b'\x05 Pick',b'\x06 Knock',b'\x05 Exit',b'\x08Locked. ']]
            print(p.replace('/tmp/claude-0/games/',''), r[:3], hex(d.find(b'\x06 Knock\x05 Exit')+7) if b'\x06 Knock\x05 Exit' in d else '-', r[4])
