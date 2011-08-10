set terminal postscript eps enhanced font "Times-Roman" 20
set size 0.8, 1.0
set output "graph2-acc.eps"

set xrange[8.5:17.5]
set yrange[5:17]

set xlabel "Log(cache size)"
set ylabel "Avg. Misprediction Rate (%)"
#set title "Graph 2 Various values of theta"

plot 'graph2.dat' using ($1):($2) title 'gehl-{/Symbol q}=4' with linespoints lc rgbcolor 'red',\
     'graph2.dat' using ($1):($4) title 'gehl-{/Symbol q}=6' with linespoints lc rgbcolor 'green',\
     'graph2.dat' using ($1):($6) title 'gehl-{/Symbol q}=8' with linespoints lc rgbcolor 'blue',\
     'graph2.dat' using ($1):($8) title 'gehl-{/Symbol q}=10' with linespoints lc rgbcolor 'black'
