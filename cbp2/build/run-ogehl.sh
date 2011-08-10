#!/bin/bash
set -o errexit
set -o nounset
trap "{ echo Usage: $0 trace config-file; }" EXIT

trace=$1
config=$2

num=10000000

name=$(basename $trace)
output=ogehl-${name/.trace.gz/.txt}

zcat $trace | head -${num} | ./bin/predict stdin ogehl release ${config} > ${output}
echo Wrote $output