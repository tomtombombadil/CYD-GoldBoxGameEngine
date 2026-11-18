#!/bin/bash
# Each switch held off on its own (toggles.sh with FIXOFF / ADNDOFF / ENHOFF of
# one bit), every game and suite: logs s_<switch>_<game>_<part>.log; then
# toggles.py <dir> base each <fixlist> reports switch by switch.
D=$(cd "$(dirname "$0")" && pwd)
sw=${*:-"$(seq -f 'F%g' 0 43) A1 A2 A4 A8 A10 A40 A80 A100 E1 E2"}
for s in $sw; do
  n=${s:1}
  case $s in
    F*) FIXOFF=$(printf '%x' $((1 << n))) bash "$D/toggles.sh" s_$s > /dev/null 2>&1 ;;
    A*) ADNDOFF=$n bash "$D/toggles.sh" s_$s > /dev/null 2>&1 ;;
    E*) ENHOFF=$n bash "$D/toggles.sh" s_$s > /dev/null 2>&1 ;;
  esac
  echo "$s done"
done
