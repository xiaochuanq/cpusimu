set terminal postscript eps enhanced font "Times-Roman" 20
set size 0.8, 1.0
set output "graph4-stddev.eps"

set xrange[8.5:17.5]
set yrange[5:15]

set xlabel "Log(cache size)"
set ylabel "Std. Dev. of Miss Rate"
#set title "Graph 4 Various values of alpha"

plot 'graph4.dat' using ($1):($3) title 'gehl-{/Symbol a}=1.2' with linespoints lc rgbcolor 'red',\
     'graph4.dat' using ($1):($5) title 'gehl-{/Symbol a}=1.5' with linespoints lc rgbcolor 'green',\
     'graph4.dat' using ($1):($7) title 'gehl-{/Symbol a}=2.0' with linespoints lc rgbcolor 'blue',\
     'graph4.dat' using ($1):($9) title 'gehl-{/Symbol a}=3.0' with linespoints lc rgbcolor 'black'
