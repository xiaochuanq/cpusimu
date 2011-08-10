set terminal postscript eps enhanced font "Times-Roman" 20
set size 0.8, 1.0
set output "graph1-acc-zoom.eps"

set xrange[12:18]
set yrange[5:10]

set xlabel "Log(cache size)"
set ylabel "Avg. Misprediction Rate (%)"
#set title "Graph 1 Accuracy"

plot 'graph1.dat' using ($1):($2) title '2bc' with linespoints lc rgbcolor 'red',\
               '' using ($1):($4) title 'gshare' with linespoints lc rgbcolor 'green',\
               '' using ($1):($6) title 'tournament' with linespoints lc rgbcolor 'blue',\
               '' using ($1):($8) title 'gehl' with linespoints lc rgbcolor 'black',\
               '' using ($1):($10) title 'o-gehl' with linespoints lc rgbcolor 'orange'
