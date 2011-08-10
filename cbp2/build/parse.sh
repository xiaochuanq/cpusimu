set -o errexit
set -o nounset
file=$1

cat $file | grep pred | grep -o [0-9]*
