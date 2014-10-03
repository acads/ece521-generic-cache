#
# ECE 521 - Computer Design Techniques, Fall 2014
# Project 1 - Generic Cache Implementation
#
# Shell script to automate sim_cache runs with output suitable for graphing 
# cache performance measurements.
# 
# Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
# 

#!/bin/bash

RED_COLOR='\e[91m \e[1m'
NO_COLOR='\e[0m'

sa="1"
sa_iter="2"
#sa_iter="-1"
SA_LIMIT="4"

csize="0"
csize_iter="6" 
CSIZE_LIMIT="16384"

BSIZE="32"
RPLCY="0"
WPLCY="0"
CSIM="./sim_cache"

OFILE="l1_sa"


function run_tests()
{
    while [ $sa -lt $SA_LIMIT ]; do
        sa=$((2**($sa_iter+1)))

        echo "# CACHE PERFORMANCE TABLE: L1 SIZE vs. MISS RATE" >> $2_$sa
        echo "# BLOCK SIZE: $BSIZE" >> $2_$sa
        echo "# SET ASSOCIATIVITY: $sa" >> $2_$sa
        echo " " >> $2_$sa
        echo "# $3" >> $2_$sa

        while [ $csize -lt $CSIZE_LIMIT ]; do
            csize=$((2**($csize_iter+1)))

            if [ $[$csize/$[$BSIZE*$sa]] -ne 0 ]; then
                echo "$CSIM $BSIZE $csize $sa $RPLCY $WPLCY $1 >> $2_$sa"
                $CSIM $BSIZE $csize $sa $RPLCY $WPLCY $1 >> $2_$sa
            fi
            csize_iter=$[$csize_iter+1]
        done

        csize="0"
        csize_iter="6"
        sa_iter=$[$sa_iter+1]
    done

    echo "# $3" >> $2_$sa
    echo " " >> $2_$sa
    echo " " >> $2_$sa
    sa="1"
    sa_iter="2"
    csize="0"
    csize_iter="6"
}


echo -e "${RED_COLOR}runs for gcc_trace.txt.. start${NO_COLOR}"
run_tests gcc_trace.txt $OFILE gcc
echo -e "${RED_COLOR}runs for gcc_trace.txt.. stop${NO_COLOR}"
echo " "

echo "runs for go_trace.txt.. start"
echo -e "${RED_COLOR}runs for go_trace.txt.. start${NO_COLOR}"
run_tests go_trace.txt $OFILE go
echo -e "${RED_COLOR}runs for go_trace.txt.. stop${NO_COLOR}"
echo " "

echo "runs for perl_trace.txt.. start"
echo -e "${RED_COLOR}runs for perl_trace.txt.. start${NO_COLOR}"
run_tests perl_trace.txt $OFILE perl
echo -e "${RED_COLOR}runs for perl_trace.txt.. stop${NO_COLOR}"
echo " "

echo -e "${RED_COLOR}runs for vortex_trace.txt.. start${NO_COLOR}"
run_tests vortex_trace.txt $OFILE vortex
echo -e "${RED_COLOR}runs for vortex_trace.txt.. stop${NO_COLOR}"
echo " "
