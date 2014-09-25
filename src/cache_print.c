/*
 * ECE 521 - Computer Design Techniques, Fall 2014
 * Project 1B - L1, L2 & victime cache implementation.
 *
 * This module contains all print routines for cache simulators. 
 *
 * Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
 */

#include <stdio.h>
#include <stdint.h>

#include "cache.h"
#include "cache_utils.h"
#include "cache_print.h"

/*************************************************************************** 
 * Name:    cache_print_sim_config 
 *
 * Desc:    Prints the simulator configuration in TA's style. 
 *
 * Params:
 *  cache   ptr to the main cache data structure
 *
 * Returns: Nothing 
 **************************************************************************/
void
cache_print_sim_config(cache_generic_t *cache)
{
    dprint("  ===== Simulator configuration =====\n");
    dprint("  L1_BLOCKSIZE: %21u\n", cache->blk_size);
    dprint("  L1_SIZE: %26u\n", cache->size);
    dprint("  L1_ASSOC: %25u\n", cache->set_assoc);
    dprint("  L1_REPLACEMENT_POLICY: %12u\n", cache->repl_plcy);
    dprint("  L1_WRITE_POLICY: %18u\n", cache->write_plcy);
    dprint("  trace_file: %23s\n", cache->trace_file);
    dprint("  ===================================\n");

    return;
}


/*************************************************************************** 
 * Name:    cache_print_sim_stats
 *
 * Desc:    Prints the simulator statistics in TA's style. 
 *
 * Params:
 *  cache   ptr to the main cache data structure
 *
 * Returns: Nothing 
 **************************************************************************/
void
cache_print_sim_stats(cache_generic_t *cache)
{
    double          miss_rate = 0.0;
    double          hit_time = 0.0;
    double          miss_penalty = 0.0;
    double          avg_access_time = 0.0;
    double          total_access_time = 0.0;
    double          b_512kb = (512 * 1024);
    cache_stats_t   *stats = NULL;

    stats = &(cache->stats);

    /* 
     * Calculation of avg. access time (from project web-page):
     * Some fixed parameters to use in your project:
     *  1. L2 Miss_Penalty (in ns) = 20 ns + 0.5*(L2_BLOCKSIZE / 16 B/ns) 
     *      (in the case that there is only L1 cache, use
     *      L1 miss penalty (in ns) = 20 ns + 0.5*(L1_BLOCKSIZE / 16 B/ns))
     *  2. L1 Cache Hit Time (in ns) = 0.25ns + 2.5ns * (L1_Cache Size / 512kB)
     *      + 0.025ns * (L1_BLOCKSIZE / 16B) + 0.025ns * L1_SET_ASSOCIATIVITY
     * 3.  L2 Cache Hit Time (in ns) = 2.5ns + 2.5ns * (L2_Cache Size / 512kB) 
     *      + 0.025ns * (L2_BLOCKSIZE / 16B) + 0.025ns * L2_SET_ASSOCIATIVITY
     * 4.  Area Budget = 512kB for both L1 and L2 Caches
     */
    miss_rate =  ((double) (stats->num_read_misses + stats->num_write_misses) /
                (double) (stats->num_reads + stats->num_writes));
    hit_time = (0.25 + (2.5 * (((double) cache->size) / b_512kb)) + (0.025 *
            (((double) cache->blk_size) / 16)) + (0.025 * cache->set_assoc));
    miss_penalty = (20 + (0.5 * (((double) cache->blk_size) / 16)));
    total_access_time = (((stats->num_reads + stats->num_writes) * hit_time) +
            ((stats->num_read_misses + stats->num_write_misses) *
             miss_penalty));
#ifndef DBG_ON
    avg_access_time = (hit_time + (miss_rate * miss_penalty));
#else
    avg_access_time = (total_access_time /
            (stats->num_reads + stats->num_writes));
#endif /* DBG_ON */

    dprint("\n");
    dprint("  ====== Simulation results (raw) ======\n");
    dprint("  a. number of L1 reads: %15u\n", stats->num_reads);
    dprint("  b. number of L1 read misses: %9u\n", stats->num_read_misses);
    dprint("  c. number of L1 writes: %14u\n", stats->num_writes);
    dprint("  d. number of L1 write misses: %8u\n", stats->num_write_misses);
    dprint("  e. L1 miss rate: %21.4f\n", miss_rate);
    dprint("  f. number of writebacks from L1: %5u\n", stats->num_write_backs);
    dprint("  g. total memory traffic: %13u\n", stats->num_blk_mem_traffic);

    dprint("\n");
    dprint("  ==== Simulation results (performance) ====\n");
    dprint("  1. average access time: %14.4f ns\n", avg_access_time);

    return;
}


/*************************************************************************** 
 * Name:    cache_print_sim_data
 *
 * Desc:    Prints the simulator data in TA's style. 
 *
 * Params:
 *  cache   ptr to the main cache data structure
 *
 * Returns: Nothing 
 **************************************************************************/
void
cache_print_sim_data(cache_generic_t *cache)
{
    uint32_t            index = 0;
    uint32_t            tag_index = 0;
    uint32_t            block_id = 0;
    uint32_t            num_sets = 0;
    uint32_t            num_blocks_per_set = 0;
    uint32_t            *tags = NULL;
    cache_tag_data_t    *tag_data = NULL;
    cache_tagstore_t    *tagstore = NULL;

    tagstore = cache->tagstore;
    num_sets = tagstore->num_sets;
    num_blocks_per_set = tagstore->num_blocks_per_set;

    dprint("\n===== L1 contents =====\n");
    for (index = 0; index < num_sets; ++index) {
        tag_index = (index * num_blocks_per_set);
        tags = &tagstore->tags[tag_index];
        tag_data = &tagstore->tag_data[tag_index];
        dprint("set%4u: ", index);
        
        for (block_id = 0; block_id < num_blocks_per_set;
                ++block_id) {
            dprint(" %7x %s", 
                    tags[block_id], (tag_data[block_id].dirty) ? g_dirty : "");
        }
        dprint("\n");
    }

    return;
}


/*************************************************************************** 
 * Name:    cache_print_usage
 *
 * Desc:    Prints simulator execution instructions. To be displayed upon
 *          wrong input parameters.
 *
 * Params:  
 *  prog    ptr to user's executable name
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_print_usage(const char *prog)
{
    dprint("Usage: %s <block-size> <l1-cache-size> <l1-set-assoc> "         \
            "<victim-cache-size>\n"                                         \
            "                   <l2-cache-size> <l2-set-assoc> "            \
            "<trace-file>\n", prog);
    dprint("    block-size          : size of each cache block in "         \
            "bytes; must be a power of 2.\n");
    dprint("    l1-cache-size       : size of the L1 cahce in bytes.\n");
    dprint("    l1-set-assoc        : set associativity of the L1 cache.\n");
    dprint("    victim-cache-size   : size of the victim cache in bytes; "  \
            "0 disables victim cache.\n");
    dprint("    l2-cache-size       : size of the L2 cache in bytes; 0 "    \
            "disables L2 cache.\n");
    dprint("    l2-set-assoc        : set associativity of the L2 cache.\n");
    dprint("    trace-file          : CPU memory access file with full "     \
            "path.\n");

    return;
}


#ifdef DBG_ON
/*************************************************************************** 
 * Name:    cache_print_cache_data
 *
 * Desc:    Prints the cache details, configuration and statistics.
 *
 * Params:
 *  pcache  ptr to the cache whose details are to be printed
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_print_cache_data(cache_generic_t *pcache)
{
    if (NULL == pcache) {
        cache_assert(0);
        goto exit;
    }

    printf("\n");
    printf("Cache Details\n");
    printf("-------------\n");
    printf("Name               : %s\n", pcache->name);
    printf("Level              : %u\n", pcache->level);
    printf("Block Size         : %u\n", pcache->blk_size);
    printf("Total Size         : %u\n", pcache->size);
    printf("Replacement Policy : %s\n", pcache->repl_plcy ? "LFU" : "LRU");
    printf("Write Policy       : %s\n", pcache->write_plcy ? "WTNA" : "WBWA");
    dprint("Prev Cache         : %s\n", 
            (pcache->prev_cache ? pcache->prev_cache->name : "None"));
    dprint("Next Cache         : %s\n", 
            (pcache->next_cache ? pcache->next_cache->name : "None"));
    printf("Statistics\n");
    dprint("---------\n");
    cache_print_stats((cache_stats_t *) &(pcache->stats), FALSE);

exit:
    return;
}


/*************************************************************************** 
 * Name:    cache_print_stats
 *
 * Desc:    Prints the cache statistics.
 *
 * Params:
 *  pstats  ptr to the cache statistics data
 *  detail  flag to denote whether parent cache details are to be printed
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_print_stats(cache_stats_t *pstats, boolean detail)
{
    char    *spacing = "    ";

    if (NULL == pstats) {
        cache_assert(0);
        goto exit;
    }

    if (TRUE == detail) {
        if (!pstats->cache) {
            cache_assert(0);
            goto exit;
        }

        cache_generic_t *cache = pstats->cache;
        printf("\n");
        spacing = "";
        printf("Cache Statistics\n");
        printf("----------------\n");
        printf("Name: %s\n", cache->name);
    }

    printf("%s# reads            : %u\n", spacing, pstats->num_reads);
    dprint("%s# read hits        : %u\n", spacing, pstats->num_read_hits);
    printf("%s# read misses      : %u\n", spacing, pstats->num_read_misses);
    printf("%s# writes           : %u\n", spacing, pstats->num_writes);
    printf("%s# write misses     : %u\n", spacing, pstats->num_write_misses);
    printf("%sMemory traffic     : %u blocks\n", spacing,
            pstats->num_blk_mem_traffic);

    if (TRUE == detail)
       printf("\n");

exit:
    return;
}


/*************************************************************************** 
 * Name:    cache_print_tagstore
 *
 * Desc:    Pretty prints the cache tagstore details
 *
 * Params:
 *  pcache  ptr to cache whose details are to be printed
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_print_tagstore(cache_generic_t *cache)
{
    if (!cache)
        cache_assert(0);

    dprint("Cache Tag Store Statistics\n");
    dprint("--------------------------\n");
    dprint("# of sets                 : %u\n", cache->tagstore->num_sets);
    dprint("# of blocks               : %u\n", cache->tagstore->num_blocks);
    dprint("# of blocks/set           : %u\n",
            cache->tagstore->num_blocks_per_set);
    dprint("# of tag index block bits : %u %u %u\n",
            cache->tagstore->num_tag_bits, cache->tagstore->num_index_bits,
            cache->tagstore->num_offset_bits);

    return;

}


/*************************************************************************** 
 * Name:    cache_print_debug_data
 *
 * Desc:    Prints cache state for a given cache line.
 *
 * Params:
 *  pcache  ptr to cache whose details are to be printed
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_print_debug_data(cache_generic_t *cache, cache_line_t *line)
{
    char                *dirty_str = NULL;
    uint32_t            *tags = NULL;
    uint32_t            num_blocks = 0;
    uint32_t            block_id = 0;
    uint32_t            tag_index = 0;
    cache_tagstore_t    *tagstore = NULL;
    cache_tag_data_t    *tag_data = NULL;

    tagstore = cache->tagstore;
    num_blocks = tagstore->num_blocks_per_set;
    tag_index = (line->index * num_blocks);
    tags = &tagstore->tags[tag_index];
    tag_data = &tagstore->tag_data[tag_index];

    dprint("Changed set %u: ", line->index);
    for (block_id = 0; block_id < num_blocks; ++block_id) {
        dirty_str = ((tag_data[block_id].dirty) ? "D" : "");
        if (tags[block_id])
            dprint("%8x %s", tags[block_id], dirty_str);
        else
            dprint("%8s %s", "-", dirty_str);
    }
    dprint("\n");

    return;
}

#endif /* DBG_ON */

