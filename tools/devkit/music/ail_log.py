#!/usr/bin/env python3
# ail_log.py <ADLIB.ADV> <file.XMI> <INSTR.AD> <sequence>: the ORACLE for music::render_ail - the player's
# Miles AIL 2 driver run in Unicorn (python with unicorn: /tmp/claude-0/uvenv) by the AIL API's sequence
# (describe, init, state table, timbre cache, register, timbre requests / installs from the GTL, start,
# then serve at the driver's rate until the status isn't 1 = playing), printing "tick reg val" for every
# OPL2 write as ail_wav's log does. Compare: diff <(ail_log.py ...) <(ail_wav ... log).
import sys,struct
from unicorn import *
from unicorn.x86_const import *
adv=open(sys.argv[1],'rb').read(); xmi=open(sys.argv[2],'rb').read(); gtl=open(sys.argv[3],'rb').read(); seq=int(sys.argv[4])
DRV,XMI,GTL,STATE,CACHE,STK,RET=0x1000,0x2000,0x3000,0x4000,0x5000,0x7000,0x0800
mu=Uc(UC_ARCH_X86,UC_MODE_16); mu.mem_map(0,0x100000)
mu.mem_write(DRV*16,adv); mu.mem_write(XMI*16,xmi); mu.mem_write(GTL*16,gtl); mu.mem_write(RET*16,b'\xf4')
st={'idx':0,'w':[]}
def hin(uc,port,size,ud): return 0
def hout(uc,port,size,val,ud):
    if port==0x388: st['idx']=val&0xFF
    elif port==0x389: st['w'].append((st['idx'],val&0xFF))
mu.hook_add(UC_HOOK_INSN,hin,None,1,0,UC_X86_INS_IN)
mu.hook_add(UC_HOOK_INSN,hout,None,1,0,UC_X86_INS_OUT)
def hint(uc,n,ud):
    print('INT',hex(n),file=sys.stderr)
    uc.reg_write(UC_X86_REG_AX,0); uc.reg_write(UC_X86_REG_CX,0); uc.reg_write(UC_X86_REG_DX,0)
mu.hook_add(UC_HOOK_INTR,hint)
fns={}; i=adv[0]|adv[1]<<8
while True:
    f,o=struct.unpack_from('<HH',adv,i)
    if f==0xFFFF: break
    fns[f]=o; i+=4
def call(fn,*args):
    sp=0xFFF0
    for w in list(reversed(args))+[RET,0]: sp-=2; mu.mem_write(STK*16+sp,struct.pack('<H',w))
    mu.reg_write(UC_X86_REG_SS,STK); mu.reg_write(UC_X86_REG_SP,sp)
    mu.reg_write(UC_X86_REG_DS,XMI); mu.reg_write(UC_X86_REG_ES,XMI)
    mu.reg_write(UC_X86_REG_CS,DRV); mu.reg_write(UC_X86_REG_IP,fns[fn])
    mu.emu_start(DRV*16+fns[fn],RET*16,count=5000000)
    return mu.reg_read(UC_X86_REG_AX)
def rw(seg,off): return struct.unpack('<H',bytes(mu.mem_read(seg*16+off,2)))[0]
call(100,0); dseg=mu.reg_read(UC_X86_REG_DX); doff=mu.reg_read(UC_X86_REG_AX)
io,irq,dma,drq,rate=[rw(dseg,doff+k) for k in (12,14,16,18,20)]
call(102,0,io,irq,dma,drq)
call(150,0); cache=call(153,0)
if cache: call(154,0,0,CACHE,cache)
hs=call(151,0,0,XMI,seq,0,STATE,0,0)
if hs==0xFFFF: sys.exit('no such sequence')
while True:
    t=call(155,0,hs)
    if t==0xFFFF: break
    bank,patch=t>>8,t&0xFF; e=0; at=None
    while e+6<=len(gtl) and gtl[e+1]!=0xFF:
        if gtl[e]==patch and gtl[e+1]==bank: at=struct.unpack_from('<I',gtl,e+2)[0]; break
        e+=6
    call(156,0,bank,patch,at,GTL)
call(170,0,hs)
tick=0
def flush():
    for r,v in st['w']: print('%d %02X %02X'%(tick,r,v))
    st['w'].clear()
flush()
while True:
    call(103); flush(); tick+=1
    s=call(174,0,hs); flush()
    if s!=1 or tick>rate*600: break
print('rate',rate,'ticks',tick,file=sys.stderr)
