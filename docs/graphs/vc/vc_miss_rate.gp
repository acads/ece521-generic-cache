#
# ECE 521 - Computer Design Techniques, Fall 2014
# Project 1 - Cache and Memory Hierarchy Design
#
# gnuplot script to plot VC size vs. L1 cache miss rate for gcc, go, perl 
# and vortex traces.
#
# Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
#

reset

set terminal postscript eps enhanced dashed 
set output 'vc_miss_rate.eps'

#set size 1,1
#set origin 0,0
set grid

set title 'Victim Cache Size vs. L1 Cache Miss Rate'

set ylabel 'L1 Cache Miss Rate'
set xlabel 'Victim Cache Size'

set style line 1 lc rgb '#FF0000' lt 1 lw 2 pt 1 ps 1.5     # red
set style line 2 lc rgb '#0000FF' lt 1 lw 2 pt 2 ps 1.5     # blue
set style line 3 lc rgb '#000000' lt 1 lw 2 pt 3 ps 1.5     # black
set style line 4 lc rgb '#FF8C00' lt 1 lw 2 pt 4 ps 1.5     # darkorange
set style line 5 lc rgb '#FF0000' lt 2 lw 2 pt 1 ps 1.5     # red

plot [:] [:] 'vc_miss_rate' index 0 with linespoints ls 1 title 'gcc', '' index 1 title 'go' with linespoints ls 2, '' index 2 title 'perl' with linespoints ls 3, '' index 3 title 'vortex' with linespoints ls 4 

reset

