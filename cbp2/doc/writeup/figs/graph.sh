#!/bin/bash
set -o errexit
mkdir -p build
prefix=$1
for i in $(ls $1*.gp); do
    gnuplot $i
done

for i in $(ls $1*.eps); do
    epstopdf $i
done
mv *.eps build/
