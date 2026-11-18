#!/bin/bash
# The toggle checks on the host tests (synthetic data, no game files): test_dax
# built with CYD_TEST_HOOKS, run with each Game Fix / AD&D 1E Rule / Enhancement
# held off by itself; the CHECK lines that fail then are that switch's.
# Output: one line a switch (its failing CHECKs' lines), the base run first.
R=$(cd "$(dirname "$0")/../.." && pwd)
O=${OUT:-/tmp/claude-0/tg/host}; mkdir -p "$O"; cd "$R"
g++ -std=c++17 -O1 -DCYD_TEST_HOOKS -Isrc -o "$O/test_dax" tools/host_tests/test_dax.cpp src/engine/*.cpp || exit 1
echo "base: $("$O/test_dax" | grep -c '^FAIL')"
for n in $(seq 0 43); do
  echo "F$n: $(FIXOFF=$(printf '%x' $((1 << n))) "$O/test_dax" | grep '^FAIL' | sed 's/.*test_dax.cpp://; s/ .*//' | tr '\n' ' ')"
done
for a in 1 2 4 8 10 40 80 100; do echo "A$a: $(ADNDOFF=$a "$O/test_dax" | grep '^FAIL' | sed 's/.*test_dax.cpp://; s/ .*//' | tr '\n' ' ')"; done
for e in 1 2; do echo "E$e: $(ENHOFF=$e "$O/test_dax" | grep '^FAIL' | sed 's/.*test_dax.cpp://; s/ .*//' | tr '\n' ' ')"; done
