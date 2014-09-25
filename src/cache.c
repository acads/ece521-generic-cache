/* 
 * ECE 521 - Computer Design Techniques, Fall 2014
 * Project 1A - Generic Cache Implementation
 *
 * This module contains the majority of the cache implementation, like
 * cache and tagstore init with corresponding cleanup routines, 
 * replacement policy implementations (LRU and LFU), cache lookup
 * and so on.
 *
 * Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "cache.h"
#include "cache_utils.h"
#include "cache_print.h"

/* Globals */
boolean             g_l2_present = FALSE;       /* l2 cache present?        */
boolean             g_victim_present = FALSE;   /* victim cache present?    */
cache_generic_t     g_l1_cache;             /* primary l1 cache             */
cache_tagstore_t    g_l1_cache_ts;          /* primary cache tagstore       */
cache_generic_t     g_l2_cache;             /* l2 cache                     */
cache_tagstore_t    g_l2_cache_ts;          /* l2 cache tagstore            */
const char          *g_dirty = "D";         /* used to denote dirty blocks  */
const char          *g_l1_name = "L1 cache";
const char          *g_l2_name = "L2 cache";


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
cache_init(cache_generic_t *l1_cache, cache_generic_t *l2_cache, 
        int num_args, char **input)
{
    char        *trace_file = NULL;
    uint8_t     arg_iter = 1;
    uint32_t    blk_size = 0;
    uint32_t    l1_size = 0;
    uint16_t    l1_set_assoc = 0;
    uint32_t    l2_size = 0;
    uint16_t    l2_set_assoc = 0;
    uint32_t    victim_size = 0;

    if ((!l1_cache) || (!l2_cache) || (!input)) {
        cache_assert(0);
        goto exit;
    }

    memset(l1_cache, 0, sizeof(*l1_cache));
    memset(l2_cache, 0, sizeof(*l2_cache));

    blk_size = atoi(input[arg_iter++]);
    l1_size = atoi(input[arg_iter++]);
    l1_set_assoc = atoi(input[arg_iter++]);
    
    victim_size = atoi(input[arg_iter++]);
    g_victim_present = (victim_size ? TRUE : FALSE);

    l2_size = atoi(input[arg_iter++]);
    l2_set_assoc = atoi(input[arg_iter++]);
    g_l2_present = (l2_size ? TRUE : FALSE);
    
    trace_file = input[arg_iter++];

    /* Init L1 cache. */
    strncpy(l1_cache->name, g_l1_name, (CACHE_NAME_LEN - 1));
    strncpy(l1_cache->trace_file, trace_file, (CACHE_TRACE_FILE_LEN - 1));
    l1_cache->size = l1_size;
    l1_cache->level = CACHE_LEVEL_1;
    l1_cache->set_assoc = l1_set_assoc;
    l1_cache->blk_size = blk_size;
    l1_cache->victim_size = victim_size;
    l1_cache->prev_cache = NULL;
    l1_cache->next_cache = (cache_util_is_l2_present() ? l2_cache : NULL);
    l1_cache->stats.cache = l1_cache;

    /* 
     * Force set repl & write policies as they are the only ones 
     * supported as of now. 
     * */
    l1_cache->repl_plcy = CACHE_REPL_PLCY_LRU;
    l1_cache->write_plcy = CACHE_WRITE_PLCY_WBWA;

    /* Init L2 cache. */
    if (cache_util_is_l2_present()) {
        strncpy(l2_cache->name, g_l2_name, (CACHE_NAME_LEN - 1));
        l2_cache->size = l2_size;
        l2_cache->level = CACHE_LEVEL_2;
        l2_cache->set_assoc = l2_set_assoc;
        l2_cache->blk_size = blk_size;
        l2_cache->victim_size = 0;      /* No victim cache for L2 */
        l2_cache->repl_plcy = CACHE_REPL_PLCY_LRU;
        l2_cache->write_plcy = CACHE_WRITE_PLCY_WBWA;
        l2_cache->next_cache = NULL;
        l2_cache->prev_cache = l1_cache;
        l2_cache->stats.cache = l2_cache;
    }

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
 *                             blocks-->
 *                      0      1      2      3 
 *                   +------+------+------+------+ 
 *                0  |      |      |      |      |  s
 *                   |      |      |      |      |  e
 *                   +------+------+------+------+  t
 *                1  |      |      |    * |      |  s
 *                   |      |      |      |      |  |
 *                   +------+----- +------+------+  |
 *                2  |      |      |      |      |  v
 *                   |      |      |      |      |
 *                   +------+------+------+------+
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
        ((cache->size) / (cache->set_assoc * cache->blk_size));

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
    tagstore->num_tag_bits = tag_bits = (CACHE_ADDR_32BIT_LEN - 
            index_bits - blk_offset_bits);
    tagstore->num_blocks_per_set = num_blocks_per_set = cache->set_assoc;
    tagstore->num_blocks = num_sets * num_blocks_per_set;

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

    /* Initialize indices. */
    for (iter = 0; iter < num_sets; ++iter)
        tagstore->index[iter] = iter;

    /* Assoicate the tagstore to the given cache and vice-versa. */
    cache->tagstore = tagstore;
    tagstore->cache = cache;

#ifdef DBG_ON
    dprint_info("printing ts data..\n");
    cache_print_tagstore(cache);
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

    /* First cleanup the tagstore and then the actual cache. */
    cache_tagstore_cleanup(cache, cache->tagstore);
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

    if (tagstore->index)
        free(tagstore->index);

    if (tagstore->tags)
        free(tagstore->tags);

    if (tagstore->tag_data)
        free(tagstore->tag_data);

    if (tagstore->set_ref_count)
        free(tagstore->set_ref_count);

    memset(tagstore, 0, sizeof(*tagstore));

exit:
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
#ifdef DBG_ON
    cache_print_debug_data(cache, line);
#endif /* DBG_ON */
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
    const char      *trace_fpath = NULL;
    FILE            *trace_fptr = NULL;
    mem_ref_t       mem_ref;

    /* Error out in case of invalid arguments. */
    if (FALSE == cache_util_validate_input(argc, argv)) {
        printf("Error: Invalid input(s). See usage for help.\n");
        cache_print_usage(argv[0]);
        goto error_exit;
    }
    trace_fpath = argv[argc - 1];

    memset(&mem_ref, 0, sizeof(mem_ref));

    /* 
     * Parse arguments and populate the data structure with 
     * cache attributes. 
     */
    cache_init(&g_l1_cache, &g_l2_cache, argc, argv);

    /* Initialize a tagstore for L1 & L2 caches. */
    cache_tagstore_init(&g_l1_cache, &g_l1_cache_ts);
    if (cache_util_is_l2_present())
        cache_tagstore_init(&g_l2_cache, &g_l2_cache_ts);

#ifdef DBG_ON
    cache_print_cache_data(&g_l1_cache);
    if (cache_util_is_l2_present())
        cache_print_cache_data(&g_l2_cache);

    return 0;
#endif /* DBG_ON */

    /* Try opening the trace file. */
    trace_fptr = fopen(trace_fpath, "r");
    if (!trace_fptr) {
        printf("Error: Unable to open trace file %s.\n", trace_fpath);
        dprint_err("unable to open trace file %s.\n", trace_fpath);
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
        if (!cache_handle_memory_request(&g_l1_cache, &mem_ref)) {
            dprint_err("Error: Unable to handle memory reference request for "\
                    "type %c, addr 0x%x.\n", 
                    mem_ref.ref_type, mem_ref.ref_addr);
            goto error_exit;
        }
    }

#ifdef DBG_ON
    cache_print_cache_data(&g_l1_cache);
#endif /* DBG_ON */

    /* Dump the cache simulator configuration, cache state and statistics. */
    cache_print_sim_config(&g_l1_cache);
    cache_print_sim_data(&g_l1_cache);
    cache_print_sim_stats(&g_l1_cache);

    if (trace_fptr) 
        fclose(trace_fptr);

    if (cache_util_is_l2_present())
        cache_cleanup(&g_l2_cache);
    cache_cleanup(&g_l1_cache);

    return 0;

error_exit:
    if (trace_fptr)
        fclose(trace_fptr);

    if (cache_util_is_l2_present())
        cache_cleanup(&g_l2_cache);
    cache_cleanup(&g_l1_cache);

    return -1;
}

