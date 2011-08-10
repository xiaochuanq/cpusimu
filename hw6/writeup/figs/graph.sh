#!/bin/bash
set -o errexit
for i in $(ls *.gp); do
    gnuplot $i
done

for i in $(ls *.eps); do
    epstopdf $i
done