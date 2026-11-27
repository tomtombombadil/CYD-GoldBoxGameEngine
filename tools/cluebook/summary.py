import json, sys, collections
for g in sys.argv[1:]:
    try: r=json.load(open(f'./res_{g}.json'))
    except Exception: continue
    best={}
    for sc,p,name,c,x,y in r:
        if name not in best or sc>best[name][0]: best[name]=(sc,p,c,x,y)
    found=[(n,v) for n,v in best.items() if v[0]>=0.5]
    print(f"{g}: {len(best)} maps, {len(found)} found in the cluebook (score>=0.5)")
    for n,v in sorted(best.items(), key=lambda a:-a[1][0]): print(f"   {n:14s} {v[0]:.2f} page {v[1]} scale {v[2]:.1f} at {v[3]},{v[4]}")
