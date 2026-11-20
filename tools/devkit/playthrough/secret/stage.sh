#!/bin/bash
# stage.sh <slot> <x,y> <dir> <to-slot>: walk.sh to x,y, face dir (N E S W), step once more (an edge / exit), save in to-slot
P=/tmp/claude-0/splay; S=$1; G=$2; D=$3; B=$4; shift 4
AUTOPICK=${AUTOPICK-"Leave,No"} bash $P/bin/walk.sh $S "" $G "$@" > /dev/null 2>&1
S=$(cat $P/slot.txt); T=$(cat $P/tok.txt)
TAILN=0 $P/bin/p.sh "$T" SAVE=$S > /dev/null
f=$(grep now $P/run.log | tail -1 | awk '{print $3}')
declare -A turn=( [NN]=F [NE]=RF [NS]=BF [NW]=LF [EE]=F [ES]=RF [EW]=BF [EN]=LF [SS]=F [SW]=RF [SN]=BF [SE]=LF [WW]=F [WN]=RF [WE]=BF [WS]=LF )
t=${turn[$f$D]}
TAILN=0 $P/bin/p.sh "$T$t," SAVE=$S SAVENOW=$B > /dev/null
grep "saved now" $P/run.log
