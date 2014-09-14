# adhanas

# Generic cache simulator Makefile
PROG = sim_cache
INCLS = -I.
SRCS = cache.c cache_utils.c
OBJS = $(SRCS:.c=.o)
CLEANFILES = $(PROG) $(OBJS)

# Compiler options
CC = gcc
DEBUG = -g -pg -D DBG_ON
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
