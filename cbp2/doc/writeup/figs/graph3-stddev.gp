set terminal postscript eps enhanced font "Times-Roman" 20
set size 0.8, 1.0
set output "graph3-stddev.eps"

set xrange[8.5:17.5]
set yrange[5:14]

set xlabel "Log(cache size)"
set ylabel "Std. Dev. of Miss Rate"
#set title "Graph 3 various saturating bit sizes"

plot 'graph3.dat' using ($1):($3) title 'gehl-bits=3' with linespoints lc rgbcolor 'red',\
     'graph3.dat' using ($1):($5) title 'gehl-bits=4' with linespoints lc rgbcolor 'green',\
     'graph3.dat' using ($1):($7) title 'gehl-bits=5' with linespoints lc rgbcolor 'blue',\
     'graph3.dat' using ($1):($9) title 'gehl-bits=6' with linespoints lc rgbcolor 'black'
