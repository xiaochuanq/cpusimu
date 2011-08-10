#!/bin/bash
set -o errexit
set -o nounset
# Test the tournament predictor on the gcc-10M.trace and compare to the results
# provided by the CIS 501 staff.

TRACES=$1

required_files=("gcc-10M.trace")

for file in $required_files; do
    if [ ! -e $TRACES/$file ]; then
        echo "Put the '$file' file in the traces/ folder"
    fi
done

TRACE=${TRACES}/gcc-1K.trace

TYPE="debug"

TABLES=4
IDX=3
L1=2
ALPHA=1.8
THETA=4
CTR_BIT=5

cat << EOF > test-config
# Tables	IdxBits	L1	Alpha	Theta	  CtrBit	AddHist	TCBit	AC	DynT	DynH	Verbose
${TABLES}       ${IDX}	${L1}   ${ALPHA} ${THETA} ${CTR_BIT}    0	1	1     	0	0	0
EOF


cat ${TRACE} | ./bin/predict stdin ogehl ${TYPE} test-config 2>&1 > ogehl.output
cat ${TRACE} | ./bin/predict stdin gehl ${TYPE} ${TABLES} ${IDX} ${L1} ${ALPHA} ${THETA} ${CTR_BIT} 2>&1 > gehl.output

