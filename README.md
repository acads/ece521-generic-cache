ece521-mp0
==========
Fall 2014 ECE 521
Projec 1A - Generic Cache Implementation


Compilation
===========
    1. Goto src/
    2, Issue "make clean && make" to build a non-QA version on the build. This
       should be used for final testing.
    3. For debug purposes (which has debug traces, debug symbols and profiling
       enabled), issue, "make clean && make DEBUG=DBG_ON".
    4. The executable generated will be named 'sim_cache'.


Running the Generic Cache Simulator
===================================
The generic cache simulator has to be configured with cache parameters in the 
runtime. The parameters include cache size, cache block size, cache set 
associativity, replacement policy, write policy and memory refernce requests 
(read/write) trace file.

$ ./sim_cache 
Error: Invalid input(s). See usage for help.
Usage: src/./sim_cache <block-size> <cache-size> <set-assoc> <replacement-policy> <write-policy> <trace-file>
    block-size         : size of each cache block in bytes; must be a power of 2.
    cache-size         : size of the entire cahce in bytes.
    set-assoc          : set associativity of the cache.
    replacement-policy : 0 LRU, 1 LFU.
    write-policy       : 0 WBWA, 1 WTNA.
    trace-file         : CPU memory access file with full path.

The above help text is self explanatory. Refer to docs/pa0_parta_spec.pdf for 
more details.


Example Runs
============
$ ./sim_cache 16 16384 1 0 0 ../docs/gcc_trace.txt > tmp1
Configures the block size as 16 B, cache size as 16KB, set associavity to be 1 
(direct mapped), LRU replacement policy, WBWA write policy and 
../docs/gcc_trace.txt as the memory reference trace file.


Interpretting the Output
========================
A sample output of the cache simulator would look similar to what iss given 
below.

---------------simulator output start------------------------
  ===== Simulator configuration =====
  L1_BLOCKSIZE:                    16   
  L1_SIZE:                      16384
  L1_ASSOC:                         1    
  L1_REPLACEMENT_POLICY:            0    
  L1_WRITE_POLICY:                  0    
  trace_file:   ../docs/gcc_trace.txt
  ===================================

  ===== L1 contents =====
  set   0:    10015 D
  set   1:    10015 D
  set   2:    10015 D
  set   3:    10015 D
  set   4:    10015 D
  set   5:    10015 D
  set   6:    10015 D
  set   7:    10015 D
  ...
  ...
  ...
  set1015:    10011 
  set1016:    10011 
  set1017:    10014 D
  set1018:    10014 D
  set1019:    10014 D
  set1020:    10014 D
  set1021:    10014 D
  set1022:    10014 D
  set1023:    10014 D

  ====== Simulation results (raw) ======
  a. number of L1 reads:           63640
  b. number of L1 read misses:      2138 
  c. number of L1 writes:          36360
  d. number of L1 write misses:     4579 
  e. L1 miss rate:                0.0672
  f. number of writebacks from L1:  3939 
  g. total memory traffic:         10656

  ==== Simulation results (performance) ==== 
  1. average access time:         1.7570 ns

 ---------------simulator output end-------------------------

