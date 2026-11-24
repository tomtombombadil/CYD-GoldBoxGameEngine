import os,re,sys,glob
# tidy_check.py <dir> [game...] - see tidy_check.sh
# reads: every file the engine opened (FILELOG) - relative to the run's copy of the data folder
# removed: the dry run's "Would remove <folder>/<path>"
gog={'poolrad':'Pool of Radiance','curse':'Curse of the Azure Bonds','secret':'Secret of the Silver Blades',
 'darkness':'Pools of Darkness','champions':'Champions of Krynn','knights':'Death Knights of Krynn',
 'queen':'The Dark Queen of Krynn','gateway':'Gateway to the Savage Frontier','treasures':'Treasures of the Savage Frontier'}
dry=open(sys.argv[1]+'/dry_gog.txt').read().splitlines()
bad=0
for g,folder in gog.items():
    if len(sys.argv)>2 and g not in sys.argv[2:]: continue
    logs=glob.glob(sys.argv[1]+f'/fl/{g}.*.log')
    if not logs: continue
    reads=set()
    for l in logs:
        for p in open(l,errors='replace'):
            p=p.strip(); m=re.search(r'/run/[^/]+/(.*)$',p)
            if m: reads.add(m.group(1))
    # the run's copy: <data folder name>/<file>; the dry run: <folder>/<...>/<file>
    rem=set()
    for d in dry:
        m=re.match(r'\s+Would remove (.*)$',d)
        if m and m.group(1).startswith(folder+'/'): rem.add(m.group(1)[len(folder)+1:])
    data=('POOLRAD/' if g=='poolrad' else '')
    hit=[r for r in reads if (data+'/'.join(r.split('/')[1:]) if g!='poolrad' else r) in rem]
    names=sorted(set(r.split('/',1)[1] if '/' in r else r for r in reads if not r.startswith('_CYD')))
    print(f'{g}: {len(names)} game files read; removed-but-read: {hit}')
    bad+=len(hit)
print('PROBLEMS', bad)
sys.exit(1 if bad else 0)
