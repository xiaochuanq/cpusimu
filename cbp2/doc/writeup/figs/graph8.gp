set terminal postscript eps enhanced font "Times-Roman" 20
set size 0.8, 1.0
set output 'graph8.eps'

set xlabel 'log_2(ROB Entries)'
set ylabel 'uIPC'
# set title 'How ROB Size Affects IPC' font ',24'

set xrange [2:11]
set yrange [1:10]

plot 'graph8.dat' using ($1):($8) title 'best-case' with linespoints lc rgbcolor 'black' ,\
               '' using ($1):($2) title 'realistic-branch-gshare' with linespoints lc rgbcolor 'red',\
               '' using ($1):($4) title 'realistic-branch-gehl' with linespoints lc rgbcolor 'green',\
               '' using ($1):($6) title 'realistic-branch-o-gehl' with linespoints lc rgbcolor 'blue'