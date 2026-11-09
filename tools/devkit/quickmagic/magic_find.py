#!/usr/bin/env python3
# magic_find.py <GAME.OVR / unpacked image ...>: each game's Alt-M toggle (cmp al,0x32 then the flag),
# every cmp / mov of that flag - the AI spell gate and the fight-start reset (DQK.EXE: see README)
import sys,re,struct
for path in sys.argv[1:]:
    d=open(path,'rb').read()
    flags=set()
    for m in re.finditer(rb'\x3c\x32(\x75.|\x0f\x85..)\x80\x3e(..)\x00',d,re.S):
        fl=struct.unpack('<H',m.group(2))[0]; flags.add(fl)
        print(path.split('/')[-2][:12], 'toggle at 0x%X flag 0x%X'%(m.start(),fl))
    for fl in flags:
        for m in re.finditer(re.escape(b'\x80\x3e'+struct.pack('<H',fl)+b'\x00'),d):
            print('   cmp [flag] at 0x%X'%m.start(), d[m.start()-14:m.start()+7].hex())
        for m in re.finditer(re.escape(b'\xc6\x06'+struct.pack('<H',fl)),d):
            print('   mov [flag],imm at 0x%X val %d'%(m.start(),d[m.start()+4]))
