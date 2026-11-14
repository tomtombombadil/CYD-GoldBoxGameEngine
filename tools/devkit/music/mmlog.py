#!/usr/bin/env python3
# mmlog.py <MUSIC.DAX> <song> <out.log> [max s] [quiet s]: the ORACLE for the music renderer - the
# player's own MicroMagic driver (MUSIC.DAX block 0) run in Unicorn (python with unicorn: /tmp/claude-0/uvenv),
# its OPL2 writes logged tick by tick ("T n" then "reg val"). mm_log (the engine) must give the same writes.
# mmlog.py <MUSIC.DAX> <song> <out.log> [max seconds]: runs the player's own MicroMagic driver (block 0)
# in Unicorn and logs its OPL2 writes tick by tick: "T <tick>" lines then "reg val"
import sys,struct
sys.path.insert(0,'/home/claude/cyd-goldboxgameengine/tools/devkit/oracles')
from gb import dax_block
from unicorn import *
from unicorn.x86_const import *
path=sys.argv[1]; song=int(sys.argv[2]); out=open(sys.argv[3],'w'); maxs=float(sys.argv[4]) if len(sys.argv)>4 else 600
drv=dax_block(path,0); sng=dax_block(path,song)
mu=Uc(UC_ARCH_X86,UC_MODE_16); mu.mem_map(0,0x100000)
DSEG=0x2000; mu.mem_write(DSEG*16+0x100,drv)
SSEG=0x4000; mu.mem_write(SSEG*16,sng)
RET=0x0800; mu.mem_write(RET*16,b'\xf4')
st={'idx':0,'pit':[], 'w':[]}
def hin(uc,port,size,ud): return 0
def hout(uc,port,size,val,ud):
    if port==0x388: st['idx']=val&0xFF
    elif port==0x389: st['w'].append((st['idx'],val&0xFF))
    elif port==0x40: st['pit'].append(val&0xFF)
mu.hook_add(UC_HOOK_INSN,hin,None,1,0,UC_X86_INS_IN)
mu.hook_add(UC_HOOK_INSN,hout,None,1,0,UC_X86_INS_OUT)
def hint(uc,n,ud):
    if n!=0x1a: print('INT',hex(n),file=sys.stderr)
    uc.reg_write(UC_X86_REG_AX,0); uc.reg_write(UC_X86_REG_CX,0); uc.reg_write(UC_X86_REG_DX,0)
mu.hook_add(UC_HOOK_INTR,hint)
def run(cs,ip,words):
    sp=0xFFF0
    for w in words: sp-=2; mu.mem_write(0x70000+sp,struct.pack('<H',w))
    mu.reg_write(UC_X86_REG_SS,0x7000); mu.reg_write(UC_X86_REG_SP,sp)
    mu.reg_write(UC_X86_REG_DS,0x3000); mu.reg_write(UC_X86_REG_ES,0x3000)
    mu.reg_write(UC_X86_REG_CS,cs); mu.reg_write(UC_X86_REG_IP,ip)
    mu.emu_start(cs*16+ip,RET*16,count=5000000)
def call(cmd,a,b): run(DSEG,0x100,[b,a,cmd,RET,0])
mu.mem_write(0,b"\xcf")   # old INT 1C = iret
call(0,0,SSEG); print("after0",bytes(mu.mem_read(0x70,4)).hex(),st["pit"],len(st["w"]),file=sys.stderr); hoff=struct.unpack('<H',bytes(mu.mem_read(0x70,2)))[0]; h=bytes(mu.mem_read(DSEG*16+hoff,16)); k=h.find(b'\x2e\xff\x06'); clk=struct.unpack_from('<H',h,k+3)[0]
mu.mem_write(DSEG*16+clk,b"\x01\x00"); call(1,0,0)   # the driver's clock running (its start waits for a non-zero time); print("after1",bytes(mu.mem_read(0x70,4)).hex(),"tracks",bytes(mu.mem_read(DSEG*16+0x2d10+0x29e,28)).hex(),"cs:ip %04X:%04X"%(mu.reg_read(UC_X86_REG_CS),mu.reg_read(UC_X86_REG_IP)),file=sys.stderr)
pit=st['pit']; div=(pit[-2]|pit[-1]<<8) if len(pit)>=2 else 0
hz=1193182/(div or 65536)
off,seg=struct.unpack('<HH',bytes(mu.mem_read(0x70,4)))
print('INT 1C -> %04X:%04X, PIT divisor %d = %.2f Hz'%(seg,off,div,hz),file=sys.stderr)
out.write('HZ %.4f\n'%hz)
for w in st['w']: out.write('%02X %02X\n'%w)
st['w'].clear()
last=0; n=int(maxs*hz)
for t in range(n):
    run(seg,off,[0x0200,RET,0])   # flags, cs, ip for the iret
    if st['w']:
        out.write('T %d\n'%t); last=t
        for w in st['w']: out.write('%02X %02X\n'%w)
        st['w'].clear()
    if t-last>hz*float(sys.argv[5] if len(sys.argv)>5 else 5): break
print('ticks',t,'last write',last,'= %.1f s'%(last/hz),file=sys.stderr)
