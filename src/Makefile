# 
# ECE 521 - Computer Design Techniques, Fall 2014
# Project 1A - Generic Cache Implementation
#
# This module contains all required dependencies to build the 
# cache simulator. Based on the input arguments, the debug code
# can be turned on or off. 
#
# Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
#


# Generic cache simulator Makefile
PROG = sim_cache
INCLS = -I.
SRCS = cache.c cache_utils.c cache_print.c
OBJS = $(SRCS:.c=.o)
CLEANFILES = $(PROG) $(OBJS)


# Command line options
# The debug code can be truned on using the following variable from
# the command line while invoking make.
#
# Use "make DEBUG="-g -pg -D DBG_ON"" to have debug code eneabled for
# the current build. The debug build has:
# 	1. Debug symbils (-g switch in command line)
# 	2. Program profile data collector
# 	3. Enable debug traces in teh actual progran code.
#
DEBUG = -g -pg -D DBG_ON
#DEBUG = 


# Compiler options
CC = gcc
OPTIMIZER = -O0
CFLAGS = -Wall -c $(DEBUG) $(OPTIMIZER) $(INCLS)
LFLAGS = -Wall $(DEBUG) $(OPTIMIZER) $(INCLS)

 
# Make directives
all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	\rm -f $(CLEANFILES)

