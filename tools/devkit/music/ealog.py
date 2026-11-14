#!/usr/bin/env python3
# ealog.py <MIDI.DAX> <driver block> <SOUND.DAX> <song block> [seconds]: the ORACLE for music::render_ea -
# Champions' Electronic Arts driver (MIDI.DAX block 3 = AdLib) run in Unicorn (/tmp/claude-0/uvenv) as
# START.EXE's sound unit runs it: AH=1 CX=song segment, then AH=0 at every timer tick (PIT 0x5555);
# "T tick" then "reg val" lines - ea_wav's log must be identical.
import sys,struct
sys.path.insert(0,'/home/claude/cyd-goldboxgameengine/tools/devkit/oracles')
from gb import dax_block
from unicorn import *
from unicorn.x86_const import *
drv=dax_block(sys.argv[1],int(sys.argv[2])); song=dax_block(sys.argv[3],int(sys.argv[4]))
mu=Uc(UC_ARCH_X86,UC_MODE_16); mu.mem_map(0,0x100000)
DRV,SNG,RET=0x2000,0x3000,0x0800
mu.mem_write(DRV*16,drv); mu.mem_write(SNG*16,song); mu.mem_write(RET*16,b'\xf4')
st={'idx':0,'w':[],'ports':set()}
def hin(uc,port,size,ud): st['ports'].add(('in',port)); return 0
def hout(uc,port,size,val,ud):
    if port==0x388: st['idx']=val&0xFF
    elif port==0x389: st['w'].append((st['idx'],val&0xFF))
    else: st['ports'].add(('out',port))
mu.hook_add(UC_HOOK_INSN,hin,None,1,0,UC_X86_INS_IN)
mu.hook_add(UC_HOOK_INSN,hout,None,1,0,UC_X86_INS_OUT)
def hint(uc,n,ud):
    print('INT',hex(n),hex(uc.reg_read(UC_X86_REG_AX)),file=sys.stderr)
mu.hook_add(UC_HOOK_INTR,hint)
def call(ah,cx=0):
    sp=0xFFF0
    for w in [RET,0]: sp-=2; mu.mem_write(0x70000+sp,struct.pack('<H',w))
    mu.reg_write(UC_X86_REG_SS,0x7000); mu.reg_write(UC_X86_REG_SP,sp)
    mu.reg_write(UC_X86_REG_DS,0x4000); mu.reg_write(UC_X86_REG_ES,0x4000)
    mu.reg_write(UC_X86_REG_AX,ah<<8); mu.reg_write(UC_X86_REG_CX,cx)
    mu.reg_write(UC_X86_REG_CS,DRV); mu.reg_write(UC_X86_REG_IP,0)
    mu.emu_start(DRV*16,RET*16,count=2000000)
    return mu.reg_read(UC_X86_REG_AX)
call(1,SNG)
print('after play',len(st['w']),file=sys.stderr)
last=0; n=0
for t in range(int(1193182/21845*float(sys.argv[5] if len(sys.argv)>5 else 300))):
    call(0)
    if st['w']:
        print('T %d'%t)
        for w in st['w']: print('%02X %02X'%w)
        n+=len(st['w']); last=t; st['w'].clear()
print('last write tick',last,'=%.1fs'%(last/54.62),'writes',n,st['ports'],file=sys.stderr)
