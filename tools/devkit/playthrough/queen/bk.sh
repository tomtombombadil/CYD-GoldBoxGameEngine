#!/bin/bash
# bk.sh save|load <slot> <name>: a Dark Queen save slot (SAVE/SAVGAMx.QSV, VAULTx.DAT) kept in /tmp/claude-0/qplay/bak/<name>
G="/tmp/claude-0/qplay/card/The Dark Queen of Krynn/SAVE"; B=/tmp/claude-0/qplay/bak/$3
if [ "$1" = save ]; then rm -rf $B; mkdir -p $B; cp "$G/SAVGAM$2.QSV" $B/SAVGAM.QSV; cp "$G/VAULT$2.DAT" $B/VAULT.DAT 2>/dev/null; ls $B | wc -l
else cp $B/SAVGAM.QSV "$G/SAVGAM$2.QSV"; rm -f "$G/VAULT$2.DAT"; [ -f $B/VAULT.DAT ] && cp $B/VAULT.DAT "$G/VAULT$2.DAT"; echo restored $3 as $2; fi
