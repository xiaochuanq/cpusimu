set terminal postscript eps enhanced color font "Times-Roman" 20
set size 1.6, 1.0
set output "graph7.eps"

set boxwidth 1.0 absolute
set style fill solid
set style data histogram
set style histogram clustered gap 1
set style fill solid 1.00 border -1

set yrange [0:18]
set ylabel "Misprediction Rate (%)"
set xlabel "Trace"

plot 'graph7.dat' using 2 t "gehl-{/Symbol q}=4",\
               '' using 3 t "gehl-{/Symbol q}=8",\
               '' using 4:xtic(1) t "OGEHL-no-dynamic-history"