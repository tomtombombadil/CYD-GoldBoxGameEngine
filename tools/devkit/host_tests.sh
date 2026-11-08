#!/bin/bash
# CI's tests job, locally (outputs in /tmp/claude-0/ag7/ht)
set -e
R=${REPO:-$(cd "$(dirname "$0")/../.." && pwd)}
O=/tmp/claude-0/ag7/ht; mkdir -p $O; cd "$R"
for t in test_dax test_hlib test_decoders; do
  g++ -std=c++17 -O1 -Wall -Werror -fsanitize=address,undefined -Isrc -o $O/$t tools/host_tests/$t.cpp src/engine/*.cpp
  $O/$t | tail -1
done
g++ -std=c++17 -O1 -Wall -Werror -Isrc -o $O/dax_inspect tools/dax_tool/dax_inspect.cpp src/engine/*.cpp
for f in play look walk companion; do g++ -std=c++17 -O1 -Wall -Wno-class-memaccess -Wno-format-truncation -Isrc -c src/game/$f.cpp -o $O/$f.o; done
g++ -std=c++17 -O1 -Wall -Werror -fsanitize=address,undefined -Itools/host -Isrc -o $O/test_platform tools/host_tests/test_platform.cpp tools/host/*.cpp src/engine/dax.cpp
$O/test_platform | tail -1
g++ -std=c++17 -O1 -Itools/host -Isrc -o $O/playsim tools/playsim/sim.cpp src/engine/*.cpp tools/host/*.cpp
g++ -std=c++17 -O1 -DCYD_TEST_HOOKS -DBEHAVE_MONSTERS -DBEHAVE_SPELLS -DBEHAVE_ITEMS -DBEHAVE_CLASSES -DBEHAVE_SAVES -DBEHAVE_EVENTS -Itools/host -Isrc -o $O/sweep tools/playsim/sweep.cpp src/engine/*.cpp tools/host/*.cpp
g++ -std=c++17 -O1 -DCYD_TEST_HOOKS -DBEHAVE_POOL -DBEHAVE_SPELLS -Itools/host -Isrc -o $O/sweep_pool tools/playsim/sweep.cpp src/engine/*.cpp tools/host/*.cpp
echo "host tests: all built and passed"
