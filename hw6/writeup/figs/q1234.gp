set terminal postscript eps enhanced font "Times-Roman" 16
set size 0.8, 1.0
set output 'q1234.eps'

set xlabel 'log_2(ROB Entries)'
set ylabel 'uIPC'
set title 'How ROB Size Affects IPC' font ',24'

set xrange [2:11]
set yrange [0:10]


plot 'q1.dat' using (log($1) / log(2)):($2) title 'best-case' with linespoints lc rgbcolor 'orange' ,\
'q2.dat' using (log($1) / log(2)):($2) title 'conservative-memory' with linespoints lc rgbcolor 'blue' ,\
'q3.dat' using (log($1) / log(2)):($2) title 'perfect-memory-and-branch' with linespoints lc rgbcolor 'red' ,\
'q4.dat' using (log($1) / log(2)):($2) title 'realistic-branch-prediction' with linespoints lc rgbcolor '#000000'