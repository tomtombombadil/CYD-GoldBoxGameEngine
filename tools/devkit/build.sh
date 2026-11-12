#!/bin/bash
# Build the playsim / sweep variants from the repo (objects per flag set).
# Usage: build.sh <name> "<extra flags>" <main .cpp> [extra link]
#   build.sh sim  ""  tools/playsim/sim.cpp tools/playsim/heapprof.cpp  (budget: needs wrap flags, see below)
set -e
R=${REPO:-$(cd "$(dirname "$0")/../.." && pwd)}
B=/tmp/claude-0/ag7/bin
name=$1; flags=$2; main=$3; shift 3
O=/tmp/claude-0/ag7/obj/$name
mkdir -p "$O" "$B"
cd "$R"
objs=()
# (a header newer than the last build rebuilds everything: a changed struct
# with old objects linked boots into nonsense - "a release the engine doesn't
# know", 2026-11-12, twice)
stamp=$O/.stamp
if [ -f "$stamp" ] && [ -n "$(find src tools/host -name '*.h' -newer "$stamp" -print -quit)" ]; then REBUILD=1; fi
for f in src/engine/*.cpp tools/host/*.cpp "$@"; do
  o=$O/$(echo "$f" | tr '/' '_').o
  objs+=("$o")
  if [ ! -f "$o" ] || [ "$f" -nt "$o" ] || [ -n "$REBUILD" ]; then
    echo "g++ $f"
    g++ -std=c++17 -O1 -g $flags -Itools/host -Isrc -c "$f" -o "$o" &
    while [ "$(jobs -r | wc -l)" -ge 2 ]; do wait -n; done
  fi
done
wait
echo "g++ $main"
g++ -std=c++17 -O1 -g $flags -Itools/host -Isrc -c "$main" -o "$O/main.o"
g++ -std=c++17 -O1 -g $flags $LDEXTRA -o "$B/$name" "$O/main.o" "${objs[@]}" -ldl
touch "$stamp"
echo "built $B/$name"
