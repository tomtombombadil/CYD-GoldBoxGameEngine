#!/bin/bash
# qwin.sh <slot> <tokfile>: a fight up at the end of the tokens - WINFIGHT, /Q at each menu, "continue battle" No,
# the treasure left (Exit, No); the tokens written back to the file. (The Dark Queen kit.)
P=/tmp/claude-0/qplay; S=$1; F=$2; T=$(cat $F); export WINFIGHT=1
run() { TAILN=0 $P/bin/p.sh "$1" SAVE=$S SEED=${SEED:-1} ALLTEXT=1 > /dev/null; st=$(grep '? screen' $P/run.log | tail -1); }
run "$T"
for i in $(seq 1 30); do
  case "$st" in
    *"screen 24 menu [continue battle"*) T="$T/N";;
    *"screen 24"*) T="$T/Q";;
    *"screen 5 menu [|View Take"*) T="$T/Exit";;
    *"screen 11 menu [|Yes No]"*) T="$T/No";;
    *"wait 13"*|*"wait 1 "*) T="$T..";;
    *) break;;
  esac
  run "$T"
done
echo "$T" > $F; echo "$st" | cut -c1-120; grep '\[fight\] won' $P/run.log | tail -1
