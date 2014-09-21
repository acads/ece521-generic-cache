#!/bin/bash

red_bold='\e[91m \e[1m'
no_color='\e[0m'

# Check validation runs
echo -e ${red_bold}
echo "Running sim_cache against TA given validation traces.. start"
echo -e ${no_color}
echo "time ./sim_cache 16 16384 1 0 0 gcc_trace.txt > tmp1"
time ./sim_cache 16 16384 1 0 0 gcc_trace.txt > tmp1
echo " "
echo "time ./sim_cache 128 2048 8 0 1 go_trace.txt > tmp2"
time ./sim_cache 128 2048 8 0 1 go_trace.txt > tmp2
echo " "
echo "time ./sim_cache 32 4096 4 0 1 perl_trace.txt > tmp3"
time ./sim_cache 32 4096 4 0 1 perl_trace.txt > tmp3
echo " "
echo "time ./sim_cache 64 8192 2 1 0 gcc_trace.txt > tmp4"
time ./sim_cache 64 8192 2 1 0 gcc_trace.txt > tmp4
echo " "
echo "time ./sim_cache 32 1024 4 1 1 go_trace.txt > tmp5"
time ./sim_cache 32 1024 4 1 1 go_trace.txt > tmp5
echo " "
echo -e ${red_bold}
echo "Running sim_cache against TA given validation traces.. end"
echo "##################################################################################"
echo -e ${no_color}

# Diff o/p with validation runs
echo -e ${red_bold}
echo "Diff'ing sim_cache output against TA given validation runs.. start"
echo -e ${no_color}
diff -iw tmp1 ../docs/ValidationRun1.txt
diff -iw tmp2 ../docs/ValidationRun2.txt
diff -iw tmp3 ../docs/ValidationRun3.txt
diff -iw tmp4 ../docs/ValidationRun4.txt
diff -iw tmp5 ../docs/ValidationRun5.txt
echo -e ${red_bold}
echo "Diff'ing sim_cache output against TA given validation runs.. end"
echo "##################################################################################"
echo -e ${no_color}

# Check validation traces, but for FA cache config
echo -e ${red_bold}
echo "Running sim_cache against TA given validation traces, FA config.. start"
echo -e ${no_color}
echo "time ./sim_cache 16 16384 1024 0 0 gcc_trace.txt > tmp1fa"
time ./sim_cache 16 16384 1024 0 0 gcc_trace.txt > tmp1fa
echo " "
echo "time ./sim_cache 128 2048 16 0 1 go_trace.txt > tmp2fa"
time ./sim_cache 128 2048 16 0 1 go_trace.txt > tmp2fa
echo " "
echo "time ./sim_cache 32 4096  128 0 1 perl_trace.txt > tmp3fa"
time ./sim_cache 32 4096  128 0 1 perl_trace.txt > tmp3fa
echo " "
echo "time ./sim_cache 64 8192 128 1 0 gcc_trace.txt > tmp4fa"
time ./sim_cache 64 8192 128 1 0 gcc_trace.txt > tmp4fa
echo " "
echo "time ./sim_cache 32 1024 32 1 1 go_trace.txt > tmp5fa"
time ./sim_cache 32 1024 32 1 1 go_trace.txt > tmp5fa
echo " "
echo -e ${red_bold}
echo "Running sim_cache against TA given validation traces, FA config.. end"
echo "##################################################################################"
echo -e ${no_color}

# Diff o/p with MV runs
echo -e ${red_bold}
echo "Diff'ing sim_cache output against TA given validation traces, FA config.. start"
echo -e ${no_color}
diff -iw tmp1fa ../docs/test1fa.txt
diff -iw tmp2fa ../docs/test2fa.txt
diff -iw tmp3fa ../docs/test3fa.txt
diff -iw tmp4fa ../docs/test4fa.txt
diff -iw tmp5fa ../docs/test5fa.txt
echo -e ${red_bold}
echo "Diff'ing sim_cache output against TA given validation traces, FA config.. end"
echo "##################################################################################"
echo -e ${no_color}

