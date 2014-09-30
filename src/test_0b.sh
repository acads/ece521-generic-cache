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
    echo -e "${red_bold}Running sim_cache against TA given "    \
        "validation traces.. start${no_color}"
}


function run_end()
{
    echo -e "${red_bold}Running sim_cache against TA given "    \
        "validation traces.. end${no_color}"
}


function diff_start()
{
    echo -e "${red_bold}Diff'ing sim_cache output against TA given "   \
        "validation runs.. start${no_color}"
}


function diff_end()
{
    echo -e "${red_bold}Diff'ing sim_cache output against TA given "    \
        "validation runs.. end${no_color}"
}


function print_line()
{
    echo -e "${red_bold}-------------------------------------------------------------------${no_color}"
    echo " "
}

# Check validation runs
function run6()
{
    run_start
    echo "time ./sim_cache 32 2048 4 0 4096 8 gcc_trace.txt > b6"
    time ./sim_cache 32 2048 4 0 4096 8 gcc_trace.txt > b6
    run_end

    echo " "

    diff_start
    diff -iw b6 ../docs/Validation6_PartB.txt
    diff_end

}


function run7()
{
    run_start
    echo "time ./sim_cache 16 1024 8 0 8192 4 go_trace.txt > b7"
    time ./sim_cache 16 1024 8 0 8192 4 go_trace.txt > b7
    run_end

    echo " "

    diff_start
    diff -iw b7 ../docs/Validation7_PartB.txt
    diff_end
}


function run8()
{
    run_start
    echo "time ./sim_cache 32 1024 8 256 0 0 perl_trace.txt > b8"
    time ./sim_cache 32 1024 8 256 0 0 perl_trace.txt > b8
    run_end

    echo " "

    diff_start
    diff -iw b8 ../docs/Validation8_PartB.txt
    diff_end
}


function run9()
{
    run_start
    echo "time ./sim_cache 128 1024 2 1024 4096 4 gcc_trace.txt > b9"
    time ./sim_cache 128 1024 2 1024 4096 4 gcc_trace.txt > b9
    run_end

    echo " "

    diff_start
    diff -iw b9 ../docs/Validation9_PartB.txt
    diff_end
}


function run10()
{
    run_start
    echo "time ./sim_cache 64 8192 2 1024 16384 4 perl_trace.txt > b10"
    time ./sim_cache 64 8192 2 1024 16384 4 perl_trace.txt > b10
    run_end

    echo " "

    diff_start
    diff -iw b10 ../docs/Validation10_PartB.txt
    diff_end
}


# Main driver
# Call the appropriate test function based on passed input
if [ "$1" = "all" ]
then
    run6
    print_line
    run7
    print_line
    run8
    print_line
    run9
    print_line
    run10
    print_line
fi

case "$1" in
    6) run6 ;;
    7) run7 ;;
    8) run8 ;;
    9) run9 ;;
    10) run10 ;;
esac

