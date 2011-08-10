#!/bin/bash
set -o errexit
set -o nounset
# Test the tournament predictor on the gcc-10M.trace and compare to the results
# provided by the CIS 501 staff.

TRACES=$1

required_files=("gcc-10M.trace tournament3-bimodal3-gshare4-4.output")

for file in $required_files; do
    if [ ! -e $TRACES/$file ]; then
        echo "Put the '$file' file in the traces/ folder"
    fi
done

head -2000 ${TRACES}/gcc-10M.trace > gcc-short.trace
./bin/predict gcc-short.trace tournament debug | head -200 >  tournament.output
diff -q tournament.output ${TRACES}/tournament3-bimodal3-gshare4-4.output
