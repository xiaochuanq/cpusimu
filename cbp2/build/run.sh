set -o errexit
set -o nounset
trace=$1
num=$2
type=$3

name=$(basename $trace)
output=$type-${name/.trace.gz/.txt}
zcat $trace | head -$num | ./bin/predict stdin $type release $(seq 2 20) > ${output}
echo Wrote $output