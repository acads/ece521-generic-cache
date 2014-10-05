#
# ECE 521 - Computer Design Techniques, Fall 2014
# Project 1 - Cache and Memory Hierarchy Design
#
# gnuplot script to plot L1 cache size vs. L1 cache miss rate vs. average access
# time for gcc, go, perl and vortex traces.
#
# Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
#

reset

set terminal postscript eps enhanced dashed 
set output 'l1_all_1.eps'

set title 'L1 Cache Size (SA = 1) vs. L1 Cache Miss Rate vs. Average Access Time'
set grid

set ylabel 'L1 Cache Miss Rate'
set y2label 'Average Access Time (in ns)'
set xlabel 'L1 Cache Size'
set yrange [:0.7]
set y2range [1:14]
set ytics out 0.01,0.05,0.7 nomirror
set y2tics out 1,1,13 nomirror

set style line 1 lc rgb '#FF0000' lt 1 lw 2 pt 2 ps 1.5     # cont-red, x pt
set style line 2 lc rgb '#0000FF' lt 1 lw 2 pt 4 ps 1.5     # cont-blue, shaded square pt
set style line 3 lc rgb '#000000' lt 1 lw 2 pt 6 ps 1.5     # cont-black, shaded circle pt
set style line 4 lc rgb '#FF8C00' lt 1 lw 2 pt 8 ps 1.5     # cont-darkorange, shaded 3angle
set style line 5 lc rgb '#FF0000' lt 3 lw 2 pt 3 ps 1.5     # dash-red, * pt
set style line 6 lc rgb '#0000FF' lt 3 lw 2 pt 5 ps 1.5     # dash-blue, square pt
set style line 7 lc rgb '#000000' lt 3 lw 2 pt 7 ps 1.5     # dash-black, circle pt
set style line 8 lc rgb '#FF8C00' lt 3 lw 2 pt 9 ps 1.5     # dash-darkorange, 3angle pt

plot 'l1_sa_1' using 1:2 index 0 with linespoints ls 1 axes x1y1 title 'mr-gcc',    \
     '' using 1:3 index 0 title 'aat-gcc' with linespoints ls 5 axes x1y2,          \
     '' using 1:2 index 1 title 'mr-go' with linespoints ls 2 axes x1y1,            \
     '' using 1:3 index 1 title 'aat-go' with linespoints ls 6 axes x1y2,           \
     '' using 1:2 index 2 title 'mr-perl' with linespoints ls 3 axes x1y1,          \
     '' using 1:3 index 2 title 'aat-perl' with linespoints ls 7 axes x1y2,         \
     '' using 1:2 index 3 title 'mr-vortex' with linespoints ls 4 axes x1y1,        \
     '' using 1:3 index 3 title 'aat-vortex' with linespoints ls 8 axes x1y2

reset

