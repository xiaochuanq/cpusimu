set terminal postscript eps enhanced font "Times-Roman" 20
set size 0.8, 1.0
set output "graph1-stddev-zoom.eps"

set xrange[12:18]
set yrange[5:10]
set xlabel "Log(cache size)"
set ylabel "Std. Dev. of Miss Rate"

plot 'graph1.dat' using ($1):($3) title '2bc' with linespoints lc rgbcolor 'red',\
               '' using ($1):($5) title 'gshare' with linespoints lc rgbcolor 'green',\
               '' using ($1):($7) title 'tournament' with linespoints lc rgbcolor 'blue',\
               '' using ($1):($9) title 'gehl' with linespoints lc rgbcolor 'black',\
               '' using ($1):($11) title 'o-gehl' with linespoints lc rgbcolor 'orange'
