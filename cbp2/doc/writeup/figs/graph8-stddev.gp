set terminal postscript eps enhanced font "Times-Roman" 20
set size 0.8, 1.0
set output 'graph8-stddev.eps'

set xlabel 'log_2(ROB Entries)'
set ylabel 'Std. Dev. of uIPC'
# set title 'How ROB Size Affects IPC' font ',24'

set xrange [2:11]
set yrange [0:3.2]

plot 'graph8.dat' using ($1):($9) title 'best-case' with linespoints lc rgbcolor 'black' ,\
               '' using ($1):($3) title 'realistic-branch-gshare' with linespoints lc rgbcolor 'red',\
               '' using ($1):($5) title 'realistic-branch-gehl' with linespoints lc rgbcolor 'green',\
               '' using ($1):($7) title 'realistic-branch-o-gehl' with linespoints lc rgbcolor 'blue'