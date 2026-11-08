#!/bin/bash
# Unzip Tom's game zips, each to its own empty folder (scratch only)
A=/mnt/user-data/attachments/56b06fb2-ff29-5a69-a3cb-cf59c23f04d4
G=/tmp/claude-0/games
mkdir -p "$G"
for p in "curse:7bcaff16-Curse_of_the_Azure_Bonds" "poolrad:1dfca2fe-Pool_of_Radiance" "secret:2a99e2da-Secret_of_the_Silver_Blades" "darkness:4044eed3-Pools_of_Darkness" "gateway:cf5e5d0c-Gateway_to_the_Savage_Frontier" "treasures:7356fa5d-Treasures_of_the_Savage_Frontier" "champions:fa7064d0-Champions_of_Krynn" "knights:4ad379cf-Death_Knights_of_Krynn" "queen:634fbf8c-The_Dark_Queen_of_Krynn"; do
  s=${p%%:*}; z=${p#*:}
  mkdir -p "$G/$s"
  unzip -q -o "$A/$z.zip" -d "$G/$s"
  echo "$s: $(ls "$G/$s")"
done
