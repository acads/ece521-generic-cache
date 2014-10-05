reset

set terminal postscript eps enhanced solid
set output 'l1_sa_8.eps'

#set size 1,1
#set origin 0,0
set grid

set title 'L1 Cache Size (set associativity = 8) vs. L1 Miss Rate'

set ylabel 'Miss Rate'
set xlabel 'L1 Cache Size'

set style line 1 lc rgb '#FF0000' lt 1 lw 2 pt 1 ps 1.5     # red
set style line 2 lc rgb '#0000FF' lt 1 lw 2 pt 2 ps 1.5     # blue
set style line 3 lc rgb '#000000' lt 1 lw 2 pt 3 ps 1.5     # black
set style line 4 lc rgb '#FF8C00' lt 1 lw 2 pt 4 ps 1.5     # darkorange

plot 'l1_sa_8' index 0 with linespoints ls 1 title 'gcc', '' index 1 title 'go' with linespoints ls 2, '' index 2 title 'perl' with linespoints ls 3, '' index 3 title 'vortex' with linespoints ls 4

reset

