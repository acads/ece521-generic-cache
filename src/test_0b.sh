#!/bin/bash

#
# ECE 521 - Computer Design Techniques, Fall 2014
# Project 1B - Generic Cache Implementation
#
# Shell script to automate sim_cache runs against TA given validation traces
# and to compare the sime_cache output with TA given validatiaon runs.
# 
# Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
# 


red_bold='\e[91m \e[1m'
no_color='\e[0m'


function run_start()
{
    echo -e ${red_bold}
    echo "Running sim_cache against TA given validation traces.. start"
    echo -e ${no_color}
}


function run_end()
{
    echo -e ${red_bold}
    echo "Running sim_cache against TA given validation traces.. end"
    echo "__________________________________________________________________________________"
    echo -e ${no_color}
}


function diff_start()
{
    echo -e ${red_bold}
    echo "Diff'ing sim_cache output against TA given validation runs.. start"
    echo -e ${no_color}
}


function diff_end()
{
    echo -e ${red_bold}
    echo "Diff'ing sim_cache output against TA given validation runs.. end"
    echo "__________________________________________________________________________________"
    echo -e ${no_color}
}


# Check validation runs
function run6()
{
    run_start
    echo "time ./sim_cache 16 16384 1 0 0 gcc_trace.txt > tmp1"
    time ./sim_cache 16 16384 1 0 0 gcc_trace.txt > b6
    run_end

    echo " "

    diff_start
    diff -iw b6 old_b6
    diff_end

}


function run7()
{
    run_start
    echo "time ./sim_cache 128 2048 8 0 1 go_trace.txt > tmp2"
    time ./sim_cache 128 2048 8 0 1 go_trace.txt > b7
    run_end

    echo " "

    diff_start
    diff -iw b7 old_b7 
    diff_end
}


function run8()
{
    run_start
    echo "time ./sim_cache 32 4096 4 0 1 perl_trace.txt > tmp3"
    time ./sim_cache 32 4096 4 0 1 perl_trace.txt > b8
    run_end

    echo " "

    diff_start
    diff -iw b8 old_b8
    diff_end
}


function run9()
{
    run_start
    echo "time ./sim_cache 64 8192 2 1 0 gcc_trace.txt > tmp4"
    time ./sim_cache 64 8192 2 1 0 gcc_trace.txt > b9
    run_end

    echo " "

    diff_start
    diff -iw b9 old_b9
    diff_end
}


function run10()
{
    run_start
    echo "time ./sim_cache 32 1024 4 1 1 go_trace.txt > tmp5"
    time ./sim_cache 32 1024 4 1 1 go_trace.txt > b10
    run_end

    echo " "

    diff_start
    diff -iw b10 old_b10
    diff_end
}


# Main driver
# Call the appropriate test function based on passed input
if ["$1" = "all" ]
then
    run6
    run7
    run8
    run9
    run10
fi

case "$1" in
    6) run6 ;;
    7) run7 ;;
    8) run8 ;;
    9) run9 ;;
    10) run10 ;;
esac

