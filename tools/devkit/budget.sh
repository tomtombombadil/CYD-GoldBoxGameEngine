#!/bin/bash
# The fight budgets (CLAUDE.md memory rule 7): the playsim with heapprof linked
# (bin/sim), BUDGET=150 KB (153600 bytes), no out/ folder, a _CYD beside the game.
L=/tmp/claude-0/ag7/logs; mkdir -p $L
R=/tmp/claude-0/ag7/run.sh; S=/tmp/claude-0/ag7/bin/sim
one() {  # name short env...
  n=$1; s=$2; shift 2
  env "$@" HEAPPROF=1 BUDGET=150 TAG=bud timeout 900 bash $R $s $S > $L/bud_$n.log 2>&1
  echo "$n: $(grep 'highest point [0-9]' $L/bud_$n.log) fight: $(grep -c 'screen.*Fight\|fight start' $L/bud_$n.log)"
}
one curse curse SAVE=A RUNAT=8AC2 FIGHT=1
one poolrad poolrad SAVE=A RUNAT=85F9 FIGHT=1
one darkness darkness SAVE=A CHOICES=0 RUNAT=85BB FIGHT=1
one champions champions SAVE=A AREA=2 BLOCK=34 RUNAT=8441 FIGHT=1
one knights knights SAVE=A AREA=1 BLOCK=36 RUNAT=815B FIGHT=1
one queen queen SAVE=A RUNAT=8C11 FIGHT=1
one treasures treasures SAVE=A RUNAT=9C1D FIGHT=1
