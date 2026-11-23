#!/bin/bash
# runev.sh <slot> <addr> <tokens> [answer]: Darkness kit - a script event started by hand (RUNAT, RUNSET / EXTRA as
# env), then adv.sh's loop (auto tokens, fights on Quick, Yes / No, treasure, PICK=) and saved
P=/tmp/claude-0/knplay; S=$1; A=$2; T="${3:-.}"
export EVENV="RUNAT=$A ${RUNSET:+RUNSET=$RUNSET}" EVTOK="${T:0:1}"
$P/bin/adv.sh $S x "$T" ${4:-Y}
