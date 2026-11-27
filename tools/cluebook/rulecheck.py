import sys, json, collections
sys.path.insert(0,'.')
from exemplar import run
g,pdf=sys.argv[1],sys.argv[2]; examples=json.loads(sys.argv[3])
rows=run(g,pdf,0.6,examples)
json.dump(rows,open(f'./named_{g}.json','w'))
tab=collections.Counter((r[4],r[6]>0,r[7],r[9]) for r in rows)
print(g, len(rows),'sides')
for k in sorted(tab, key=lambda k:(k[1],k[2],k[3],k[0])): print('  engine wall=%d door=%d plain=%d -> cluebook %-6s %d'%(k[1],k[2],k[3],k[0],tab[k]))
