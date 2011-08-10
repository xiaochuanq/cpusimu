set terminal postscript eps enhanced font "Times-Roman" 20
set size 0.8, 1.0
set output "graph5-acc.eps"

set xrange[8.5:17.5]
set yrange[5:22]

set xlabel "Log(cache size)"
set ylabel "Avg. Misprediction Rate (%)"
#set title "Graph 5  table sizes"

plot 'graph5.dat' using ($1):($2) title 'gehl-tables=4' with linespoints lc rgbcolor 'red',\
     'graph5.dat' using ($1):($4) title 'gehl-tables=6' with linespoints lc rgbcolor 'green',\
     'graph5.dat' using ($1):($6) title 'gehl-tables=8' with linespoints lc rgbcolor 'blue',\
     'graph5.dat' using ($1):($8) title 'gehl-tables=10' with linespoints lc rgbcolor 'black',\
     'graph5.dat' using ($1):($10) title 'gehl-tables=12' with linespoints lc rgbcolor 'orange'
