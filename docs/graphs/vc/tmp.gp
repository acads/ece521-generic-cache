#
# ECE 521 - Computer Design Techniques, Fall 2014
# Project 1 - Cache and Memory Hierarchy Design
#
# gnuplot script to plot VC size vs. L1 cache miss rate vs. average access
# time for gcc, go, perl and vortex traces.
#
# Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
#

reset

set terminal postscript eps enhanced dashed 
set output 'vc_aat.eps'

set title 'Victim Cache Size vs. Average Access Time'
set grid

set ylabel 'Average Access Time'
set xlabel 'Victim Cache Size'
set yrange [0:1.4]
set ytics out 0,0.2,1.4 nomirror
#set yrange [:0.08]
#set y2range [:1.2]
#set y2tics out 0,0.1,1 nomirror

set style line 1 lc rgb '#FF0000' lt 1 lw 2 pt 2 ps 1.5     # cont-red, x pt
set style line 2 lc rgb '#0000FF' lt 1 lw 2 pt 4 ps 1.5     # cont-blue, shaded square pt
set style line 3 lc rgb '#000000' lt 1 lw 2 pt 6 ps 1.5     # cont-black, shaded circle pt
set style line 4 lc rgb '#FF8C00' lt 1 lw 2 pt 8 ps 1.5     # cont-darkorange, shaded 3angle
set style line 5 lc rgb '#FF0000' lt 5 lw 2 pt 3 ps 1.5     # dash-red, * pt
set style line 6 lc rgb '#0000FF' lt 5 lw 2 pt 5 ps 1.5     # dash-blue, square pt
set style line 7 lc rgb '#000000' lt 5 lw 2 pt 7 ps 1.5     # dash-black, circle pt
set style line 8 lc rgb '#FF8C00' lt 5 lw 2 pt 9 ps 1.5     # dash-darkorange, 3angle pt

plot 'tmp.dat' using 1:3 index 0 with linespoints ls 1 title 'gcc',    \
     '' using 1:3 index 1 title 'go' with linespoints ls 2,     \
     '' using 1:2 index 2 title 'perl' with linespoints ls 3,   \
     '' using 1:3 index 3 title 'vortex' with linespoints ls 4 

#plot 'l2l1' using 1:2 index 0 with linespoints ls 1 axes x1y1 title 'l1-mr-gcc',    \
#'' using 1:3 index 0 title 'l2-mr-gcc' with linespoints ls 5 axes x1y2,        \
#'' using 1:2 index 1 title 'l1-mr-go' with linespoints ls 2 axes x1y1,         \
#'' using 1:3 index 1 title 'l2-mr-go' with linespoints ls 6 axes x1y2, 
#'' using 1:2 index 2 title 'l1-mr-perl' with linespoints ls 3 axes x1y1,          \
#'' using 1:3 index 2 title 'l2-mr-perl' with linespoints ls 7 axes x1y2,         \
#'' using 1:2 index 3 title 'l1-mr-vortex' with linespoints ls 4 axes x1y1,        \
#'' using 1:3 index 3 title 'l2-mr-vortex' with linespoints ls 8 axes x1y2

reset

