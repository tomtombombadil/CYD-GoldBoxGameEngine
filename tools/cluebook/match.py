import sys, numpy as np
sys.path.insert(0,'.')
from read import load, edge_score
def geo_blocks(path):
    blocks=[];cur=None
    for line in open(path):
        if line.startswith('B '):
            _,rest=line.split(' ',1); f,i,pl=rest.rsplit(' ',2); cur={'name':f.split('/')[-1]+':'+i,'plain':int(pl),'sq':[]}; blocks.append(cur)
        else: cur['sq'].append(list(map(int,line.split())))
    return blocks
def geo_edges(bk):
    # H[j][i]: horizontal edge on line j between rows j-1 and j, col i; from square (i,j) north (dir0) or (i,j-1) south
    H=np.zeros((17,16),int); V=np.zeros((16,17),int)
    for y in range(16):
        for x in range(16):
            q=bk['sq'][y*16+x]
            w=[q[0],q[3],q[6],q[9]]  # dirs 0 N,2 E,4 S,6 W
            if w[0]: H[y][x]=1
            if w[2]: H[y+1][x]=1
            if w[1]: V[y][x+1]=1
            if w[3]: V[y][x]=1
    return H,V
def clue_edges(S,x0,y0,c):
    H=np.zeros((17,16)); V=np.zeros((16,17))
    for j in range(17):
        for i in range(16): H[j][i]=edge_score(S,x0,y0,c,i,j,True)
    for j in range(16):
        for i in range(17): V[j][i]=edge_score(S,x0,y0,c,i,j,False)
    return H>0.6,V>0.6
if __name__=='__main__':
    S=load(sys.argv[1]); x0,y0,c=map(float,sys.argv[2:5])
    CH,CV=clue_edges(S,x0,y0,c)
    res=[]
    for bk in geo_blocks(sys.argv[5]):
        H,V=geo_edges(bk)
        # inner edges only (outer ring: the game's maps wrap - compare all anyway)
        agree=(H==CH).sum()+(V==CV).sum(); tot=H.size+V.size
        res.append((agree/tot,bk['name']))
    for a,n in sorted(res,reverse=True)[:5]: print(f"{a:.3f} {n}")
