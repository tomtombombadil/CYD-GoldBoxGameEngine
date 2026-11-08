#!/bin/bash
# The DOSBox differ's batch: a base (autodiff.py base), each block's
# scenarios (mkscen.py) and the batch against the original (autodiff.py
# batch) -> /tmp/claude-0/differ/scen/rep_<block>.txt, the totals in
# /tmp/claude-0/differ/batch_report.txt.
# Needs: /tmp/claude-0/differ/bin/dosbox-auto (the Project's build_dosbox.sh +
# autodos.inc - GPL, not in this repo) and the playsim built with
#   build.sh simh0 "-DCYD_TEST_HOOKS -DCYD_MENU_HIGHLIGHT=0" tools/playsim/sim.cpp
# Usage: runall.sh [area:block ...]   (default: base 2:9 2:15 - the base save's own block, then two more)
D=$(cd "$(dirname "$0")" && pwd)
S=/tmp/claude-0/differ/scen
mkdir -p "$S"
[ -d /tmp/claude-0/differ/base/poolrad ] || python3 "$D/autodiff.py" base poolrad
: > /tmp/claude-0/differ/batch_report.txt
for ab in ${@:-base 2:9 2:15}; do
  name=b${ab/:/_}
  if [ "$ab" = base ]; then python3 "$D/mkscen.py" "$S/$name" > "$S/mk_$name.log" 2>&1
  else python3 "$D/mkscen.py" "$S/$name" "${ab%%:*}" "${ab##*:}" > "$S/mk_$name.log" 2>&1; fi
  python3 "$D/autodiff.py" batch "$S/$name" "$S/rep_$name.txt" > /dev/null 2>&1
  echo "$name $(tail -1 "$S/rep_$name.txt")" >> /tmp/claude-0/differ/batch_report.txt
done
cat /tmp/claude-0/differ/batch_report.txt
