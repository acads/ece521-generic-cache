# 
# ECE 521 - Computer Design Techniques, Fall 2014
# Project 1B - Victim Cache and L2 Cache Simulator
#
# Shell script to run sim_cache against different possible configrations
# of L1, victim and L2 cache. This will be mostly used for generating cache
# outputs which will be then compared with the output of others.
#
# Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
#

#!/bin/bash


NUM_PARAMS=1


function print_usage()
{
    echo "Usage: $0 <ouput-file-perepend>"
    echo "Example: $0 ad"
}


function run_tests()
{
    echo "Running all tests.. start"
    echo "It's gonna take a wile.. grab a coffee"
    
    mkdir -p test_runs

    # Original configurationm, but with vortex trace.
    ./sim_cache 32 2048 4 0 4096 8 vortex_trace.txt > test_runs/$1_vortex_6
    ./sim_cache 16 1024 8 0 8192 4 vortex_trace.txt > test_runs/$1_vortex_7
    ./sim_cache 32 1024 8 256 0 0 vortex_trace.txt > test_runs/$1_vortex_8
    ./sim_cache 128 1024 2 1024 4096 4 vortex_trace.txt > test_runs/$1_vortex_9
    ./sim_cache 64 8192 2 1024 16384 4 vortex_trace.txt > test_runs/$1_vortex_10

    # L1 and L2 only configuration.
    ./sim_cache 32 2048 4 0 4096 8 gcc_trace.txt > test_runs/$1_l1l2_6
    ./sim_cache 16 1024 8 0 8192 4 go_trace.txt > test_runs/$1_l1l2_7
    ./sim_cache 32 1024 8 0 2048 4 perl_trace.txt > test_runs/$1_l1l2_8
    ./sim_cache 128 1024 2 0 4096 4 gcc_trace.txt > test_runs/$1_l1l2_9
    ./sim_cache 64 8192 2 0 16384 4 perl_trace.txt > test_runs/$1_l1l2_10

    # L1 and victim only configuration.
    ./sim_cache 32 2048 4 512 0 0 gcc_trace.txt > test_runs/$1_vc_6
    ./sim_cache 16 1024 8 128 0 0 go_trace.txt > test_runs/$1_vc_7
    ./sim_cache 32 1024 8 256 0 0 perl_trace.txt > test_runs/$1_vc_8
    ./sim_cache 128 1024 2 1024 0 0 gcc_trace.txt > test_runs/$1_vc_9
    ./sim_cache 64 8192 2 1024 0 0 perl_trace.txt > test_runs/$1_vc_10

    # L1, victim and L2 configuration.
    ./sim_cache 32 2048 4 512 4096 8 gcc_trace.txt > test_runs/$1_all_6
    ./sim_cache 16 1024 8 128 8192 4 go_trace.txt > test_runs/$1_all_7
    ./sim_cache 32 1024 8 256 2048 4 perl_trace.txt > test_runs/$1_all_8
    ./sim_cache 128 1024 2 1024 4096 4 gcc_trace.txt > test_runs/$1_all_9
    ./sim_cache 64 8192 2 1024 16384 4 perl_trace.txt > test_runs/$1_all_10
    
    echo "Running all tests.. stop"
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

run_tests $1
normal_exit

