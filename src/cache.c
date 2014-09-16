/* adhanas */

/* 
 * Generic cache implementation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include "cache.h"
#include "cache_utils.h"

cache_generic_t     g_cache;        /* generic cache, l1 as of now  */
cache_tagstore_t    g_cache_ts;     /* gemeroc cache tagstore       */
const char          *g_dirty = "D"; /* used to denote dirty blocks  */

/*************************************************************************** 
 * Name:    cache_init
 *
 * Desc:    Init code for cache. It sets up the cache parameters based on
 *          the user given cache configuration.
 *
 * Params:  
 *  cache       ptr to the cache being initialized
 *  name        name of the cache
 *  trace_file  memory trace file used for simulation (for printing it later)
 *  level       level of the cache (1, 2 and so on)
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_init(cache_generic_t *pcache, const char *name, 
        const char *trace_file, uint8_t level)
{
    if ((NULL == pcache) || (NULL == name)) {
        cache_assert(0);
        goto exit;
    }

    memset(pcache, 0, sizeof *pcache);
    pcache->level = level;
    strncpy(pcache->name, name, (CACHE_NAME_LEN - 1));
    strncpy(pcache->trace_file, trace_file, (CACHE_TRACE_FILE_LEN -1));
    pcache->stats.cache = pcache;
    pcache->next_cache = NULL;
    pcache->prev_cache = NULL;

exit:
    return;
}


/*************************************************************************** 
 * Name:    cache_tagstore_init
 *
 * Desc:    Init code for a tagstore. Does the following:
 *          1. Calculates all the cache parameters based on the user 
 *              given specifications.
 *          2. Allocated memory for tags and tag_data. This will be a 
 *              contiguous allocation and the data should be accessed bu
 *              either 2D indices or by linearizing the 2D index to an 1D
 *              index. 
 *              1D_index = block_index + (set_index * blocks_per_set)
 *
 *                          blocks-->
 *                     0      1      2      3 
 *                   +------+------+------+------+ 
 *                   |      |      |      |      |  s
 *                   |      |      |      |      |  e
 *                 0 +------+------+------+------+  t
 *                   |      |      |    * |      |  s
 *                   |      |      |      |      |  |
 *                 1 +------+----- +------+------+  |
 *                   |      |      |      |      |  v
 *                   |      |      |      |      |
 *                 2 +------+------+------+------+
 *
 *              To get to the * block, 2D index would be [1][2]
 *              eg., 1D_index = 2 + (1 * 4) = 6
 *
 * Params:
 *  cache       ptr to the actual cache
 *  tagstore    ptr to the tagstore to be assoicated with the cache
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_tagstore_init(cache_generic_t *cache, cache_tagstore_t *tagstore)
{
    uint8_t     tag_bits = 0;
    uint8_t     index_bits = 0;
    uint8_t     blk_offset_bits = 0;
    uint32_t    num_sets = 0;
    uint32_t    num_blocks_per_set = 0;
    uint32_t    iter = 0;

    if ((!cache) || (!tagstore)) {
        cache_assert(0);
        goto exit;
    }

    /* 
     * # of tags that could be accomodated is same as the # of sets.
     * # of sets = (cache_size / (set_assoc * block_size))
     */
    tagstore->num_sets = num_sets = 
        (cache->size / (cache->set_assoc * cache->blk_size));

    /*
     * blk_offset_bits = log_b2 (blk_size)
     * index_bits = log_b2 (# of sets)
     * tag_bits = addr_bits - index_bits - blk_offset_bits
     * num_blocks = num_sets * set_assoc
     */
    tagstore->num_offset_bits = blk_offset_bits = 
        util_log_base_2(cache->blk_size);
    tagstore->num_index_bits = index_bits = 
        util_log_base_2(num_sets);
    tagstore->num_tag_bits = tag_bits = 32 - index_bits - blk_offset_bits;
    tagstore->num_blocks_per_set = num_blocks_per_set = cache->set_assoc;
    tagstore->num_blocks = num_sets * num_blocks_per_set;


    dprint_info("num_sets %u, tag_bits %u, index_bits %u, "     \
            "blk_offset_bits %u\n",
            num_sets, tag_bits, index_bits, blk_offset_bits);

    /* Allocate memory to store indices, tags and tag data. */ 
    tagstore->index = 
        calloc(1, (num_sets * sizeof(uint32_t)));
    tagstore->tags = 
        calloc(1, (num_sets * num_blocks_per_set * sizeof (uint32_t)));
    tagstore->tag_data = 
        calloc(1, (num_sets * num_blocks_per_set * 
                    sizeof (*(tagstore->tag_data))));
    tagstore->set_ref_count = calloc(1, (num_sets * sizeof(uint32_t)));

    if ((!tagstore->index) || (!tagstore->tags) || (!tagstore->tag_data)) {
        dprint("Error: Unable to allocate memory for cache tagstore.\n");
        cache_assert(0);
        goto fatal_exit;
    }

    /* Initialize indices and block IDs . */
    for (iter = 0; iter < num_sets; ++iter) {
        tagstore->index[iter] = iter;

#if 0
        for (blk_id = 0; blk_id < tagstore->num_blocks; ++blk_id) {
            tagstore->tag_data[iter][blk_id].index = iter;
            tagstore->tag_data[iter][blk_id].blk_id = blk_id;
        }
#endif
    }

    /* Assoicate the tagstore to the given cache and vice-versa. */
    cache->tagstore = tagstore;
    tagstore->cache = cache;

#ifdef DBG_ON
    cache_util_print_tagstore(cache);
#endif /* DBG_ON */

exit:
    return;

fatal_exit:
    /* Fatal exit. Quit the program. */
    exit(-1);
}


/*************************************************************************** 
 * Name:    cache_cleanup
 *
 * Desc:    Cleanup code for cache. 
 *
 * Params:  
 *  cache   ptr to the cache to be cleaned up
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_cleanup(cache_generic_t *cache)
{
    if (!cache) {
        cache_assert(0);
        goto exit;
    }

    memset(cache, 0, sizeof(*cache));

exit:
    return;
}


/*************************************************************************** 
 * Name:    cache_tagstore_cleanup
 *
 * Desc:    Cleanup code for tagstore. Frees all allocated memory.
 *
 * Params:
 *  cache       ptr to the cache to which the current tagstore is part of
 *  tagstore    ptr to the tagstore to be cleaned up
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_tagstore_cleanup(cache_generic_t *cache, cache_tagstore_t *tagstore)
{
    if ((!cache) || (!tagstore)) {
        cache_assert(0);
        goto exit;
    }

    free(tagstore->index);
    free(tagstore->tags);
    free(tagstore->tag_data);
    free(tagstore->set_ref_count);

exit:
    return;
}


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
    dprint("\n");
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
    hit_time = (0.25 + (2.5 * (((double) cache->size) / 512000)) + (0.025 *
            (((double) cache->blk_size) / 16)) + (0.025 * cache->set_assoc));
    miss_penalty = (20 + (0.5 * (((double) cache->blk_size) / 16)));
    total_access_time = (((stats->num_reads + stats->num_writes) * hit_time) +
            ((stats->num_read_misses + stats->num_write_misses) * 
             miss_penalty));
    avg_access_time = (hit_time + (miss_rate * miss_penalty));
    //avg_access_time = (total_access_time / 
      //      (stats->num_reads + stats->num_writes));

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
 * Name:    cache_get_lru_block
 *
 * Desc:    Returns the LRU block ID for the given set.
 *
 * Params:
 *  tagstore    ptr to the cache tagstore
 *  mref        ptr to the incoming memory reference
 *  line        ptr to the decoded cache line
 *
 * Returns: int32_t
 *          ID of the frist valid LRU block for eviction
 *          CACHE_RV_ERR on error
 **************************************************************************/
int32_t
cache_get_lru_block(cache_tagstore_t *tagstore, mem_ref_t *mref, 
        cache_line_t *line)
{
    int32_t             block_id = 0;
    int32_t             min_block_id = 0;
    uint32_t            num_blocks = 0;
    uint32_t            tag_index = 0;
    uint64_t            min_age = 0;
    cache_tag_data_t    *tag_data = NULL;

    if ((!tagstore) || (!mref) || (!line)) {
        cache_assert(0);
        goto error_exit;
    }

    num_blocks = tagstore->num_blocks_per_set;
    tag_index = (line->index * num_blocks);
    tag_data = &tagstore->tag_data[tag_index];

    for (block_id = 0, min_age = tag_data[block_id].age; 
            block_id < num_blocks; ++block_id) {
        if ((tag_data[block_id].valid) && tag_data[block_id].age < min_age) {
            min_block_id = block_id;
            min_age = tag_data[block_id].age;
        }
    }

#ifdef DBG_ON
    printf("LRU index %u\n", line->index);
    for (block_id = 0; block_id < num_blocks; ++block_id) {
        printf("    block %u, valid %u, age %lu\n",
                block_id, tag_data[block_id].valid, tag_data[block_id].age);
    }
    printf("min_block %u, min_age %lu\n", min_block_id, min_age);
#endif /* DBG_ON */

    return min_block_id;

error_exit:
    return CACHE_RV_ERR;
}


/*************************************************************************** 
 * Name:    cache_get_lfu_block
 *
 * Desc:    Returns the LFU block ID for the given set.
 *
 * Params:
 *  tagstore    ptr to the cache tagstore
 *  mref        ptr to the incoming memory reference
 *  line        ptr to the decoded cache line
 *
 * Returns: int32_t
 *          ID of the frist valid LFU block for eviction
 *          CACHE_RV_ERR on error
 **************************************************************************/
int32_t
cache_get_lfu_block(cache_tagstore_t *tagstore, mem_ref_t *mref, 
        cache_line_t *line)
{
    int32_t             block_id = 0;
    int32_t             min_block_id = 0;
    uint32_t            num_blocks = 0;
    uint32_t            tag_index = 0;
    uint32_t            min_ref_count = 0;
#ifdef DBG_ON
    uint32_t            *tags = NULL;
#endif /* DBG_ON */
    cache_tag_data_t    *tag_data = NULL;

    if ((!tagstore) || (!mref) || (!line)) {
        cache_assert(0);
        goto error_exit;
    }

    num_blocks = tagstore->num_blocks_per_set;
    tag_index = (line->index * num_blocks);
#ifdef DBG_ON
    tags = &tagstore->tags[tag_index];
#endif /* DBG_ON */
    tag_data = &tagstore->tag_data[tag_index];

    for (block_id = 0, min_ref_count = tag_data[block_id].ref_count; 
            block_id < num_blocks; ++block_id) {
        if ((tag_data[block_id].valid) && 
                (tag_data[block_id].ref_count < min_ref_count)) {
            min_block_id = block_id;
            min_ref_count = tag_data[block_id].ref_count;
        }
    }

#ifdef DBG_ON
    printf("LFU index %u\n", line->index);
    for (block_id = 0; block_id < num_blocks; ++block_id) {
        printf("    block %u, tag 0x%x, valid %u, ref_count %u\n",
                block_id, tags[block_id], tag_data[block_id].valid, 
                tag_data[block_id].ref_count);
    }
    printf("min_block %u, min_ref_count %u\n", min_block_id, min_ref_count);
#endif /* DBG_ON */

    return min_block_id;

error_exit:
    return CACHE_RV_ERR;
}


/*************************************************************************** 
 * Name:    cache_get_first_invalid_block
 *
 * Desc:    Returns the first free (invalid) block for a set.
 *
 * Params:
 *  tagstore    ptr to the cache tagstore
 *  line        ptr to the decoded cache line
 *
 * Returns: int32_t
 *          ID of the frist available (invalid) block
 *          CAHCE_RV_ERR if no such block is available
 **************************************************************************/
int32_t
cache_get_first_invalid_block(cache_tagstore_t *tagstore, cache_line_t *line)
{
    int32_t             block_id = 0;
    uint32_t            num_blocks = 0;
    uint32_t            tag_index = 0;
    cache_tag_data_t    *tag_data = NULL;

    if ((!tagstore) || (!line)) {
        cache_assert(0);
        goto error_exit;
    }

    num_blocks = tagstore->num_blocks_per_set;
    tag_index = (line->index * num_blocks);
    tag_data = &tagstore->tag_data[tag_index];

    for (block_id = 0; block_id < num_blocks; ++block_id) {
        if (!tag_data[block_id].valid)
            return block_id;
    }

error_exit:
    return CACHE_RV_ERR;
}


/*************************************************************************** 
 * Name:    cache_does_tag_match
 *
 * Desc:    Compares the incoming tag in all the blocks representing the 
 *          index the tag belongs too.  
 *
 * Params:
 *  tagstore    ptr to the cache tagstore
 *  line        ptr to the decoded cache line
 *
 * Returns: int32_t 
 *  ID of the block on a match is found
 *  CACHE_RV_ERR if no match is found
 **************************************************************************/
int32_t
cache_does_tag_match(cache_tagstore_t *tagstore, cache_line_t *line)
{
    uint32_t            tag_index = 0;
    uint32_t            block_id = 0;
    uint32_t            num_blocks = 0;
    uint32_t            *tags = NULL;
    cache_rv            rc = CACHE_RV_ERR;
    cache_tag_data_t    *tag_data = NULL;

    if ((!tagstore) || (!line)) {
        cache_assert(0);
        goto error_exit;
    }

    num_blocks = tagstore->num_blocks_per_set;
    tag_index = (line->index * num_blocks);
    tags = &tagstore->tags[tag_index];
    tag_data = &tagstore->tag_data[tag_index];

    /*
     * Go over all the valid blocks for this set and compare the incoming tag
     * with the tag in tagstore. Return ture on a match and false otherwise.
     */
    for (block_id = 0; block_id < num_blocks; ++block_id) {
        if ((tag_data[block_id].valid) && (tags[block_id] == line->tag))
            return block_id;
    }

error_exit:
    rc = CACHE_RV_ERR;
    return rc;
}


/*************************************************************************** 
 * Name:    cache_handle_dirty_tag_evicts 
 *
 * Desc:    Handles dirty tag evicts from the cache. If the write policy is
 *          set to write back, writes the block to next level of memory (yet
 *          to be implemented) .
 *
 * Params:
 *  tagstore    ptr to the cache tagstore
 *  block_id    ID of the block within the set which has to be evicted
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_handle_dirty_tag_evicts(cache_tagstore_t *tagstore, cache_line_t *line, 
        uint32_t block_id)
{
    uint32_t            tag_index = 0;
    cache_generic_t     *cache = NULL;
    cache_tag_data_t    *tag_data = NULL;

    if (!tagstore) {
        cache_assert(0);
        goto exit;
    }

    cache = (cache_generic_t *) tagstore->cache;
    tag_index = (line->index * tagstore->num_blocks_per_set);
    tag_data = &tagstore->tag_data[tag_index];

    /* 
     * As of now, we just update the write back counter and clear the dirty
     * bit on the block. This code will be extended in the future to
     * implement actual memory write backs.
     */
    cache->stats.num_write_backs += 1;
    cache->stats.num_blk_mem_traffic += 1;
    tag_data[block_id].dirty = 0;

    dprint_info("writing dirty block, index %u, block %u to next level "    \
            "due to eviction\n", line->index, block_id);

    /* dan_todo: add code for handling dirty evicts. */

exit:
    return;
}


/*************************************************************************** 
 * Name:    cache_evict_tag
 *
 * Desc:    Responsible for tag eviction (based on the replacement poliy set) 
 *          and to write back the block, if the selected eviction block
 *          happens to be dirty.
 *
 * Params:
 *  tagstore    ptr to the cache tagstore
 *  line        ptr to the decoded cache line
 *
 * Returns: int32_t 
 *  ID of the block on a match is found
 *  CACHE_RV_ERR if no match is found
 **************************************************************************/
int32_t
cache_evict_tag(cache_generic_t *cache, mem_ref_t *mref, cache_line_t *line)
{
    int32_t             block_id = 0;
    uint32_t            tag_index;
    cache_tagstore_t    *tagstore = NULL;

    if ((!cache) || (!mref) || (!line)) {
        cache_assert(0);
        goto error_exit;
    }

    tagstore = cache->tagstore;
    tag_index = (line->index * tagstore->num_blocks_per_set);

    switch (CACHE_GET_REPLACEMENT_POLICY(cache)) {
        case CACHE_REPL_PLCY_LRU:
            block_id = cache_get_lru_block(tagstore, mref, line);
            if (CACHE_RV_ERR == block_id)
                goto error_exit;
            break;

        case CACHE_REPL_PLCY_LFU:
            block_id = cache_get_lfu_block(tagstore, mref, line);
            if (CACHE_RV_ERR == block_id)
                goto error_exit;

            /*
             * According to LFU policy, the row ref count should be set to
             * the ref count of the block being evicted and the evicted block 
             * ref count should be reset.
             */
            tagstore->set_ref_count[line->index] = 
                tagstore->tag_data[tag_index + block_id].ref_count;
            tagstore->tag_data[tag_index + block_id].ref_count = 0;
            
#ifdef DBG_ON
            printf("set_ref_count %u, tag_ref_count %u\n",
                    tagstore->set_ref_count[line->index],
                    tagstore->tag_data[tag_index + block_id].ref_count);
#endif /* DBG_ON */
            break;

        default:
            cache_assert(0);
            goto error_exit;
    }
    dprint_info("selected index %u , block %d for eviction\n", 
            line->index, block_id);
    
    /* If the block to be evicted is dirty, write it back if required. */
    if (cache_util_is_block_dirty(tagstore, line, block_id)) {
        dprint_info("selected a dirty block to evict in index %u, block %d\n",
                line->index, block_id);
        cache_handle_dirty_tag_evicts(tagstore, line, block_id);
    }

    return block_id;

error_exit:
    return CACHE_RV_ERR;
}


/*************************************************************************** 
 * Name:    cache_evict_and_add_tag 
 *
 * Desc:    Core processing routine. It does one (and only one) of the 
 *          following for every memory reference:
 *          1. If the tag is already present, we are done here. Might have to
 *              write thru for a write request if the write policy is set to 
 *              WTNA.
 *          2. If a free block is available, place the tag in that block. 
 *          3. Evict a block (based on the eviction policy set), do a write
 *              back (if evicted block is dirty) and place the incoming tag
 *              on the block.
 *
 *          For all three operations above, we need to update read/write, 
 *          miss/hit counters, valid, dirty (for writes) and age for the block.
 *
 * Params:
 *  cache       ptr to cache
 *  mem_ref     ptr to the memory reference (type and address)
 *  line        ptr to the decoded cache line
 *
 * Returns: Nothing.
 **************************************************************************/
void
cache_evict_and_add_tag(cache_generic_t *cache, mem_ref_t *mem_ref, 
        cache_line_t *line)
{
    uint8_t             read_flag = FALSE;
    int32_t             block_id = 0;
    uint32_t            tag_index = 0;
    uint32_t            *tags = NULL;
    uint64_t            curr_age;
    cache_tag_data_t    *tag_data = NULL;
    cache_tagstore_t    *tagstore = NULL;

    /* Fetch the current time to be used for tag age (for LRU). */
    curr_age = util_get_curr_time(); 

    tagstore = cache->tagstore;
    tag_index = (line->index * tagstore->num_blocks_per_set);
    tags = &tagstore->tags[tag_index];
    tag_data = &tagstore->tag_data[tag_index];
    read_flag = (IS_MEM_REF_READ(mem_ref) ? TRUE : FALSE);

    if (read_flag)
        cache->stats.num_reads += 1;
    else
        cache->stats.num_writes += 1;


    if (CACHE_RV_ERR != (block_id = cache_does_tag_match(tagstore, line))) {
        /* 
         * Tag is already present. Just update the tag_data and write thru 
         * if required.
         */
        tag_data[block_id].valid = 1;
        tag_data[block_id].age = curr_age;
        tag_data[block_id].ref_count += 1;
        if (read_flag) {
            cache->stats.num_read_hits += 1;
        } else {
            cache->stats.num_write_hits += 1;
            
            /* Set the block to be dirty only for WBWA write policy. */
            if (CACHE_WRITE_PLCY_WBWA == CACHE_GET_WRITE_POLICY(cache)) {
                tag_data[block_id].dirty = 1;
             } else {
                cache->stats.num_blk_mem_traffic += 1;
             }
        }
        dprint_info("tag 0x%x already present in index %u, block %u\n",
                line->tag, line->index, block_id);
    } else if (CACHE_RV_ERR != 
            (block_id = cache_get_first_invalid_block(tagstore, line))) {
        /* 
         * All blocks are invalid. This usually happens when the cache is
         * being used for the first time. Place the tag in the 1st available 
         * block. 
         */

        if ((!read_flag) && 
                (CACHE_WRITE_PLCY_WTNA == CACHE_GET_WRITE_POLICY(cache))) {
            /* Don't bother for writes when the policy is set to WTNA. */
            cache->stats.num_write_misses += 1;
            cache->stats.num_blk_mem_traffic += 1;
            goto exit;
        }

        tags[block_id] = line->tag;

        cache->stats.num_blk_mem_traffic += 1;
        tag_data[block_id].valid = 1;
        tag_data[block_id].age = curr_age;
        tag_data[block_id].ref_count = 
            (util_get_block_ref_count(tagstore, line) + 1);
        if (read_flag) {
            cache->stats.num_read_misses += 1;
        } else {
            cache->stats.num_write_misses += 1;

            /* Set the block to be dirty only for WBWA write policy. */
            if (CACHE_WRITE_PLCY_WBWA == CACHE_GET_WRITE_POLICY(cache))
                tag_data[block_id].dirty = 1;
        }
        dprint_info("tag 0x%x added to index %u, block %u\n", 
                line->tag, line->index, block_id);
    } else {
        /*
         * Neither a tag match was found nor a free block to place the tag.
         * Evict an existing tag based on the replacement policy set and place
         * the new tag on that block.
         */
        
        if ((!read_flag) && 
                (CACHE_WRITE_PLCY_WTNA == CACHE_GET_WRITE_POLICY(cache))) {
            /* Don't bother with write misses for WTNA write policy. */
            cache->stats.num_write_misses += 1;
            cache->stats.num_blk_mem_traffic += 1;
            goto exit;
        }

        block_id = cache_evict_tag(cache, mem_ref, line);
        tags[block_id] = line->tag;


        cache->stats.num_blk_mem_traffic += 1;
        tag_data[block_id].valid = 1;
        tag_data[block_id].age = curr_age;
        tag_data[block_id].ref_count = 
            (util_get_block_ref_count(tagstore, line) + 1);

#ifdef DBG_ON
        printf("set %u, block %u, tag 0x%x, block_ref_count %u\n",
                line->index, block_id, tags[block_id], 
                tag_data[block_id].ref_count);
#endif /* DBG_ON */

        if (read_flag) {
            cache->stats.num_read_misses += 1;
        } else {
            cache->stats.num_write_misses += 1;
            tag_data[block_id].dirty = 1;
        }
        dprint_info("tag 0x%x added to index %u, block %d after eviction\n",
                line->tag, line->index, block_id);
    }

exit:
    //cache_util_print_debug_data(cache, line);
    return;
}


/*************************************************************************** 
 * Name:    cache_handle_memory_request 
 *
 * Desc:    Cache processing entry point for the main driver. Decodes the
 *          incoming memory reference into cache understandable data and
 *          calls further cache processing routines.
 *
 * Params:
 *  cache   ptr to L1 cache
 *  mref    ptr to incoming memory reference
 *
 * Returns: boolean
 *  TRUE if all goes well
 *  FALSE otherwise 
 **************************************************************************/
boolean
cache_handle_memory_request(cache_generic_t *cache, mem_ref_t *mref)
{
    cache_line_t line;

    if ((!cache) || (!mref)) {
        cache_assert(0);
        goto error_exit;
    }

    /*
     * Decode the incoming memory reference into <tag, index, offset> based
     * on the cache configuration.
     */
    memset(&line, 0, sizeof(line));
    cache_util_decode_mem_addr(cache->tagstore, mref->ref_addr, &line);

    /* Cache pipeline starts here. */
    cache_evict_and_add_tag(cache, mref, &line);

    return TRUE;

error_exit:
    return FALSE;
}


/* 42: Life, the Universe and Everything; including caches. */
int
main(int argc, char **argv)
{
    char            newline = '\n';
    uint8_t         arg_iter = 1;
    const char      *trace_fpath = NULL;
    FILE            *trace_fptr = NULL;
    mem_ref_t       mem_ref;

    /* Error out in case of invalid arguments. */
    if (FALSE == cache_util_validate_input(argc, argv)) {
        printf("Error: Invalid input(s). See usage for help.\n");
        cache_util_print_usage(argv[0]);
        goto error_exit;
    }

    memset(&mem_ref, 0, sizeof mem_ref);

    /* 
     * Parse arguments and populate the data structure with 
     * cache attributes. 
     */
    trace_fpath = argv[argc - 1];
    cache_init(&g_cache, "generic cache", trace_fpath, CACHE_LEVEL_1);
    g_cache.blk_size = atoi(argv[arg_iter++]);
    g_cache.size = atoi(argv[arg_iter++]);
    g_cache.set_assoc = atoi(argv[arg_iter++]);
    g_cache.repl_plcy = atoi(argv[arg_iter++]);
    g_cache.write_plcy = atoi(argv[arg_iter++]);

    /* Initialize a tagstore for the given cache. */
    cache_tagstore_init(&g_cache, &g_cache_ts);

    /* Try opening the trace file. */
    trace_fptr = fopen(trace_fpath, "r");
    if (!trace_fptr) {
        printf("Error: Unable to open trace file %s.\n", trace_fpath);
        goto error_exit;
    }

    /* 
     * Read the trace file, fetch the address and process the memory access
     * request for every request in the trace file. 
     */
    while (fscanf(trace_fptr, "%c %x%c", 
                &(mem_ref.ref_type), &(mem_ref.ref_addr), &newline) != EOF) {
        dprint_info("mem_ref %c 0x%x\n", 
                mem_ref.ref_type, mem_ref.ref_addr);
        if (!cache_handle_memory_request(&g_cache, &mem_ref)) {
            dprint_err("Error: Unable to handle memory reference request for "\
                    "type %c, addr 0x%x.\n", 
                    mem_ref.ref_type, mem_ref.ref_addr);
            goto error_exit;
        }
    }

#ifdef DBG_ON
    cache_util_print(&g_cache);
#endif /* DBG_ON */

    /* Dump the cache simulator configuration, cache state and statistics. */
    cache_print_sim_config(&g_cache);
    cache_print_sim_data(&g_cache);
    cache_print_sim_stats(&g_cache);

    if (trace_fptr) 
        fclose(trace_fptr);

    return 0;

error_exit:
    if (trace_fptr)
        fclose(trace_fptr);

    return -1;
}

