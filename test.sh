#!/bin/bash
# set -x

# binaries build by clang-10
dir="build_old/release"

# binaries build by clang-14
# dir="build/release"

repeats=(5 50 95)
sizes=(1M 10M 100M)
for r in "${!repeats[@]}"; do
    for s in "${!sizes[@]}"; do
        rep=${repeats[r]}
        siz=${sizes[s]}
        name="r${rep}-${siz}.txt"
        echo "-------------------------------------------------------------"
        $dir/gen -repeat=$rep test/$name $siz

        $dir/uwc test/$name -simple
        $dir/uwc test/$name -agg single
        $dir/uwc test/$name -agg multi
        $dir/uwc test/$name -agg delayed-single
        $dir/uwc test/$name -agg delayed-multi
    done
done

# $dir/gen -repeat=20 test/r20-1G.txt 1G
# $dir/uwc test/r20-1G.txt -simple
# $dir/uwc test/r20-1G.txt -agg single
# $dir/uwc test/r20-1G.txt -agg multi
# $dir/uwc test/r20-1G.txt -agg delayed-single
# $dir/uwc test/r20-1G.txt -agg delayed-multi


