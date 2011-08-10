set terminal postscript eps enhanced font "Times-Roman" 16
set size 0.8, 1.0
set output 'q5.eps'

set xlabel 'log_2(ROB Entries)'
set ylabel 'Avg. Number of Lost Cycles'
set title 'Cost of Branch Misprediction' font ',24'

set xrange [2:11]
set yrange [4:14]
plot 'q5.dat' using (log($1) / log(2)):(($2-$3)/102577) notitle with linespoints lc rgbcolor 'red'