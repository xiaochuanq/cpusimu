set terminal postscript eps enhanced font "Times-Roman" 20
set size 0.8, 1.0
set output "graph3-acc.eps"

set xrange[8.5:17.5]
set yrange[5:18]

set xlabel "Log(cache size)"
set ylabel "Avg. Misprediction Rate (%)"
#set title "Graph 3 various saturating bit counters"

plot 'graph3.dat' using ($1):($2) title 'gehl-bits=3' with linespoints lc rgbcolor 'red',\
     'graph3.dat' using ($1):($4) title 'gehl-bits=4' with linespoints lc rgbcolor 'green',\
     'graph3.dat' using ($1):($6) title 'gehl-bits=5' with linespoints lc rgbcolor 'blue',\
     'graph3.dat' using ($1):($8) title 'gehl-bits=6' with linespoints lc rgbcolor 'black'
