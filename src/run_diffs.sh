#
# ECE 521 - Computer Design Techniques, Fall 2014
# Project 1B - Victim Cache and L2 Cache Simulator
#
# Shell script to compare the output (generated using the script run_runs.sh) 
# of the two different user's cache_sim.
#
# Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
#

#!/bin/bash


NUM_PARAMS=2


function print_usage()
{
    echo "Usage: $0 <your-initials> <my-initials>"
    echo "Example: $0 ad yz"
}


function run_diffs()
{
    cd test_runs/

    echo "Running diffs.. start"

    diff -iw $1_vortex_6 $2_vortex_6
    diff -iw $1_vortex_7 $2_vortex_7
    diff -iw $1_vortex_8 $2_vortex_8
    diff -iw $1_vortex_9 $2_vortex_9
    diff -iw $1_vortex_10 $2_vortex_10

    diff -iw $1_l1l2_6 $2_l1l2_6
    diff -iw $1_l1l2_7 $2_l1l2_7
    diff -iw $1_l1l2_8 $2_l1l2_8
    diff -iw $1_l1l2_9 $2_l1l2_9
    diff -iw $1_l1l2_10 $2_l1l2_10

    diff -iw $1_vc_6 $2_vc_6
    diff -iw $1_vc_7 $2_vc_7
    diff -iw $1_vc_8 $2_vc_8
    diff -iw $1_vc_9 $2_vc_9
    diff -iw $1_vc_10 $2_vc_10

    diff -iw $1_all_6 $2_all_6
    diff -iw $1_all_7 $2_all_7
    diff -iw $1_all_8 $2_all_8
    diff -iw $1_all_9 $2_all_9
    diff -iw $1_all_10 $2_all_10

    echo "Running diffs.. stop"
}


function normal_exit()
{
    echo
    exit 0
}


if [ $# -ne "$NUM_PARAMS" ]
then
    echo "Error: Invalid usage."
    print_usage
    normal_exit
fi

run_diffs $1 $2
normal_exit

