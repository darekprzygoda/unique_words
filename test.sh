#!/bin/bash
#set -x

#exec="build/debug/uwc"
exec="build/release/uwc"

files="t1.txt ala.txt r25.txt"
# files="r20-100M.txt"
for p in $files
do
    $exec test/$p -simple
    $exec test/$p

    $exec test/$p -simple -inbuf 16M
    $exec test/$p -inbuf 16M

done



