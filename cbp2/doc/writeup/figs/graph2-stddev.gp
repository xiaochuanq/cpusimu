set terminal postscript eps enhanced font "Times-Roman" 20
set size 0.8, 1.0
set output "graph2-stddev.eps"

set xrange[8.5:17.5]
set yrange[5:14]

set xlabel "Log(cache size)"
set ylabel "Std. Dev. of Miss Rate"
#set title "Graph 2 Various values of theta"

plot 'graph2.dat' using ($1):($3) title 'gehl-{/Symbol q}=4' with linespoints lc rgbcolor 'red',\
     'graph2.dat' using ($1):($5) title 'gehl-{/Symbol q}=6' with linespoints lc rgbcolor 'green',\
     'graph2.dat' using ($1):($7) title 'gehl-{/Symbol q}=8' with linespoints lc rgbcolor 'blue',\
     'graph2.dat' using ($1):($9) title 'gehl-{/Symbol q}=10' with linespoints lc rgbcolor 'black'
