set terminal postscript eps enhanced color font "Times-Roman" 20
set size 1.6, 1.0
set output "graph6.eps"

set boxwidth 1.0 absolute
set style fill solid
set style data histogram
set style histogram clustered gap 1
set style fill solid 1.00 border -1

set yrange [0:20]
set ylabel "Misprediction Rate (%)"
set xlabel "Trace"

plot 'graph6.dat' using 2 t "gehl-{/Symbol a}=1.2",\
               '' using 3 t "gehl-{/Symbol a}=2.0",\
               '' using 4:xtic(1) t "OGEHL-{/Symbol a}=1.8-no-dynamic-{/Symbol q}"