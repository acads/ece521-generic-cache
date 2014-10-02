/*
 * ECE 521 - Computer Design Techniques, Fall 2014
 * Project 1B - L1, L2 & victime cache implementation.
 *
 * This module contains all print routines for cache simulators. 
 *
 * Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
    uint16_t  l2_assoc = 0; 
    uint32_t l2_size = 0;

    if (cache_util_is_l2_present()) {
        cache_generic_t *l2 = cache_util_get_l2();
        l2_size = l2->size;
        l2_assoc = l2->set_assoc;
    }

    dprint("===== Simulator configuration =====\n");
    dprint("BLOCKSIZE: %24u\n", cache->blk_size);
    dprint("L1_SIZE: %26u\n", cache->size);
    dprint("L1_ASSOC: %25u\n", cache->set_assoc);
    dprint("Victim_Cache_SIZE: %16u\n", cache->victim_size);
    dprint("L2_SIZE: %26u\n", l2_size);
    dprint("L2_ASSOC: %25u\n", l2_assoc);
    dprint("trace_file: %23s\n", cache->trace_file);
    dprint("===================================\n");

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
    double          l1_miss_rate = 0.0;
    double          l1_hit_time = 0.0;
    cache_stats_t   *l1_stats = NULL;

    uint32_t        vc_num_swaps = 0;
    uint32_t        vc_num_write_backs = 0;
    boolean         vc_present = FALSE;
    cache_stats_t   *vc_stats = NULL;
    cache_generic_t *vc = NULL;

    uint32_t        l2_num_reads = 0;
    uint32_t        l2_num_writes = 0;
    uint32_t        l2_num_misses = 0;
    uint32_t        l2_num_read_misses = 0;
    uint32_t        l2_num_write_misses = 0;
    uint32_t        l2_num_write_backs = 0;
    double          l2_hit_time = 0.0;
    double          l2_miss_rate = 0.0;
    double          l2_miss_penalty = 0.0;
    boolean         l2_present = FALSE;
    cache_stats_t   *l2_stats;
    cache_generic_t *l2 = NULL;

    double          miss_penalty = 0.0;
    double          avg_access_time = 0.0;
    double          total_access_time = 0.0;
    double          b_512kb = (512 * 1024);
    uint32_t        total_traffic = 0;

    l1_stats = &(cache->stats);

    if (cache_util_is_victim_present()) {
        vc_present = TRUE;
        vc = cache_util_get_vc();
        vc_stats = &vc->stats;
    }

    if (cache_util_is_l2_present()) {
        l2_present = TRUE;
        l2 = cache_util_get_l2();
        l2_stats = &l2->stats;
    }


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
     * 5. Average access time =  HTL1 + (MRL1 *(HTL2+MRL2*Miss PenaltyL2))
     */

    if (l2_present) {
        l2_num_reads = l2_stats->num_reads;
        l2_num_read_misses = l2_stats->num_read_misses;
        l2_num_writes = l2_stats->num_writes;
        l2_num_write_misses = l2_stats->num_write_misses;
        l2_num_misses = l2_stats->num_read_misses + l2_stats->num_write_misses;
        l2_num_write_backs = l2_stats->num_write_backs;
        l2_miss_penalty = (20 + (0.5 * (((double) l2->blk_size) / 16)));
        l2_miss_rate =
            ((double) (l2_stats->num_read_misses) /
                (double) (l2_stats->num_reads));
        l2_hit_time = (2.5 + (2.5 * (((double) l2->size) / b_512kb)) +
                (0.025 * (((double) l2->blk_size) / 16)) +
                (0.025 * l2->set_assoc));
    } 

    if (vc_present) {
        vc_num_swaps = vc_stats->num_swaps;
        vc_num_write_backs = vc_stats->num_write_backs;
    }

    l1_miss_rate =
       ((double) (l1_stats->num_read_misses + l1_stats->num_write_misses) /
        (double) (l1_stats->num_reads + l1_stats->num_writes));
    l1_hit_time = (0.25 + (2.5 * (((double) cache->size) / b_512kb)) + (0.025 *
            (((double) cache->blk_size) / 16)) + (0.025 * cache->set_assoc));
    miss_penalty = (20 + (0.5 * (((double) cache->blk_size) / 16)));
    total_access_time =
        (((l1_stats->num_reads + l1_stats->num_writes) * l1_hit_time) +
         ((l2_num_misses) * miss_penalty));

    if (l2_present) {
        total_traffic = (l2_stats->num_read_misses + 
                         l2_stats->num_write_misses +
                         l2_stats->num_write_backs);
        avg_access_time = (l1_hit_time +
            (l1_miss_rate * (l2_hit_time + l2_miss_rate * l2_miss_penalty)));
    }
    else if (vc_present) {
        total_traffic = (l1_stats->num_read_misses +
                         l1_stats->num_write_misses +
                         vc_stats->num_write_backs);
        avg_access_time = (l1_hit_time +
            (l1_miss_rate * (miss_penalty)));
    } else {
        total_traffic = l1_stats->num_blk_mem_traffic; 
        avg_access_time = (l1_hit_time +
            (l1_miss_rate * (miss_penalty)));
    }

    dprint("====== Simulation results (raw) ======\n");

    /* L1 cache data. */
    dprint("a. number of L1 reads: %20u\n", l1_stats->num_reads);
    dprint("b. number of L1 read misses: %14u\n", l1_stats->num_read_misses);
    dprint("c. number of L1 writes: %19u\n", l1_stats->num_writes);
    dprint("d. number of L1 write misses: %13u\n", l1_stats->num_write_misses);
    dprint("e. L1 miss rate: %26.4f\n", l1_miss_rate);

    /* Victim cache data. */
    dprint("f. number of swaps: %23u\n", vc_num_swaps);
    dprint("g. number of victim cache writeback: %6u\n", 
            vc_num_write_backs);

    /* L2 cache data. */
    dprint("h. number of L2 reads: %20u\n", l2_num_reads);
    dprint("i. number of L2 read misses: %14u\n", l2_num_read_misses);
    dprint("j. number of L2 writes: %19u\n", l2_num_writes);
    dprint("k. number of L2 write misses: %13u\n", l2_num_write_misses);

    /*
     * An ugly hack to match the weird format given by the TAs.
     * When L2 isn't present, L2 miss rate (which is a float), is printed
     * as just 0. No decimals!
     * */
    if (l2_present)
        dprint("l. L2 miss rate: %26.4f\n", l2_miss_rate);
    else
        dprint("l. L2 miss rate: %26u\n", 0);

    dprint("m. number of L2 writebacks: %15u\n", l2_num_write_backs);

    dprint("n. total memory traffic: %18u\n", total_traffic);

    dprint("==== Simulation results (performance) ====\n");
    dprint("1. average access time: %14.4f ns\n", avg_access_time);

    return;
}


/*************************************************************************** 
 * Name:    cache_print_cache_data
 *
 * Desc:    Prints the simulator data in TA's style. 
 *
 * Params:
 *  cache   ptr to the main cache data structure
 *
 * Returns: Nothing 
 **************************************************************************/
void
cache_print_cache_data(cache_generic_t *cache)
{
    char                *title = NULL;
    uint32_t            index = 0;
    uint32_t            tag_index = 0;
    uint32_t            id = 0;
    uint32_t            block_id = 0;
    uint32_t            num_sets = 0;
    uint32_t            num_blocks_per_set = 0;
    uint32_t            *tags = NULL;
    uint64_t            *tag_ages = NULL;
    cache_tag_data_t    *tag_data = NULL;
    cache_tagstore_t    *tagstore = NULL;

    tagstore = cache->tagstore;
    num_sets = tagstore->num_sets;
    num_blocks_per_set = tagstore->num_blocks_per_set;
    tag_ages = (uint64_t *) calloc(1, (num_blocks_per_set * sizeof(uint64_t)));

    switch (cache->level) {
        case CACHE_LEVEL_1:
            title = "===== L1 contents =====";
            break;

        case CACHE_LEVEL_L1_VICTIM:
            title = "===== Victim Cache contents =====";
            break;

        case CACHE_LEVEL_2:
            title = "===== L2 contents =====";
            break;
        default:
            cache_assert(0);
    }

    dprint("%s\n", title);
    for (index = 0; index < num_sets; ++index) {
        tag_index = (index * num_blocks_per_set);
        tags = &tagstore->tags[tag_index];
        tag_data = &tagstore->tag_data[tag_index];

        /*
         * Copy the tag ages for sorting. TAs decided to print the tags by
         * their ages. Recent tags goes first.
         *
         * qsort the tags, match them back to the block IDs and print the
         * tags accordingly!
         */
        for (block_id = 0; block_id < num_blocks_per_set; ++block_id)
            tag_ages[block_id] = tag_data[block_id].age;

        qsort(tag_ages, num_blocks_per_set,
                sizeof(uint64_t), util_compare_uint64);
        
        dprint("set%4u: ", index);
        for (id = 0; id < num_blocks_per_set; ++id) {
            for (block_id = 0; block_id < num_blocks_per_set; ++block_id) {
                if ((tag_ages[id]) &&
                        (tag_data[block_id].age == tag_ages[id])) {
                    dprint(" %7x %s",
                        tags[block_id],
                        (tag_data[block_id].dirty) ? g_dirty : " ");
                    tag_ages[id] = 0;
                }
            }
        }
        dprint("\n");
    }
    free(tag_ages);

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
 * Name:    cache_print_cache_dbg_data
 *
 * Desc:    Prints the cache details, configuration and statistics.
 *
 * Params:
 *  pcache  ptr to the cache whose details are to be printed
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_print_cache_dbg_data(cache_generic_t *pcache)
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
    dprint("Set Associativity  : %u\n", pcache->set_assoc);
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
    dprint("# of blocks/set           : %u\n",
            cache->tagstore->num_blocks_per_set);
    dprint("# of blocks               : %u\n", cache->tagstore->num_blocks);
    dprint("# of tag index block bits : %u %u %u\n",
            cache->tagstore->num_tag_bits, cache->tagstore->num_index_bits,
            cache->tagstore->num_offset_bits);

    return;

}

#endif /* DBG_ON */

/*************************************************************************** 
 * Name:    cache_print_tags
 *
 * Desc:    Prints cache state for a given cache line.
 *
 * Params:
 *  cache       ptr to cache whose details are to be printed
 *  line        cache_line for the given cache & referenced memory addr
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_print_tags(cache_generic_t *cache, cache_line_t *line)
{
    char                *dirty_str = NULL;
    int32_t             lru_id = -1;
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
    lru_id = cache_util_get_lru_block_id(tagstore, line);

    dprint("%6u %s [%2u, %d, %7x]: ",
            g_addr_count, CACHE_GET_NAME(cache),
            line->index, lru_id, line->tag);

    for (block_id = 0; block_id < num_blocks; ++block_id) {
        dirty_str = ((tag_data[block_id].dirty) ? "D" : "");
        if (tags[block_id])
            dprint("%8x %1s", tags[block_id], dirty_str);
        else
            dprint("%8s %1s", "-", dirty_str);
    }
    dprint("\n");

    return;
}

