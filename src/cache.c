/* 
 * ECE 521 - Computer Design Techniques, Fall 2014
 * Project 1B - L1, victim & L2 cache implementation.
 *
 * This module contains the majority of the cache implementation, like
 * cache and tagstore init with corresponding cleanup routines, 
 * replacement policy implementation (LRU), cache lookup and so on.
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
cache_generic_t     *g_cache;               /* currently under processing   */
cache_tagstore_t    *g_ts;                  /* currently under processing   */
const char          *g_dirty = "D";         /* used to denote dirty blocks  */
const char          *g_l1_name = "L1";//"cache_l1";
const char          *g_l2_name = "L2";//"cache_l2";
const char          *g_read = "READ";
const char          *g_write = "WRITE";
uint32_t            g_addr_count;


/*************************************************************************** 
 * Name:    cache_set_current_cache
 *
 * Desc:    Points the global current cache to the passed cache. Likewise 
 *          for tagstore.
 *
 * Params:  
 *  cache       ptr to the cache to be set as current cache
 *  tagstore    ptr to the ts to be set as current ts
 *
 * Returns: Nothing
 **************************************************************************/
inline void
cache_set_current_cache(cache_generic_t *cache, cache_tagstore_t *tagstore)
{
    g_cache = cache;
    g_ts = tagstore;

    dprint_info("current active cache changed to %s\n", g_cache->name);

    return;
}


/*************************************************************************** 
 * Name:    cache_get_current_cache
 *
 * Desc:    Fetches a ptr to the current cache under processing 
 *
 * Params:  None 
 *
 * Returns: Pointer to the currently active cache
 **************************************************************************/
inline cache_generic_t *
cache_get_current_cache(void)
{
    return g_cache;
}


/*************************************************************************** 
 * Name:    cache_get_current_ts
 *
 * Desc:    Fetches a ptr to the current tagstore under processing 
 *
 * Params:  None 
 *
 * Returns: Pointer to the currently active tagstore
 **************************************************************************/
inline cache_tagstore_t *
cache_get_current_tagstore(void)
{
    return g_ts;
}


/*************************************************************************** 
 * Name:    cache_init
 *
 * Desc:    Init code for cache. It sets up the cache parameters based on
 *          the user given cache configuration.
 *
 * Params:  
 *  l1_cache    ptr to the cache being initialized
 *  l2_cache    ptr to L2 cache
 *  num_args    # of input arguments
 *  input       ptr to input list
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
    dprint_info("%s init successful\n", CACHE_GET_NAME(l1_cache));

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
        dprint_info("%s init successful\n", CACHE_GET_NAME(l2_cache));
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
    tagstore->lru_block_id = calloc(1, num_sets * sizeof(uint8_t));
    tagstore->index = 
        calloc(1, (num_sets * sizeof(uint32_t)));
    tagstore->tags = 
        calloc(1, (num_sets * num_blocks_per_set * sizeof (uint32_t)));
    tagstore->tag_data = 
        calloc(1, (num_sets * num_blocks_per_set * 
                    sizeof (*(tagstore->tag_data))));
    tagstore->set_ref_count = calloc(1, (num_sets * sizeof(uint32_t)));

    if ((!tagstore->index) || (!tagstore->tags) || (!tagstore->tag_data)) {
        dprint("Error: Unable to allocate memory for cache %s tagstore.\n",
                CACHE_GET_NAME(cache));
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
    dprint_info("printing ts data for %s\n", CACHE_GET_NAME(cache));
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

    if (tagstore->lru_block_id)
        free(tagstore->lru_block_id);

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
#ifdef DBG_ON
    cache_generic_t     *cache = NULL;
#endif /* DBG_ON */

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
    tagstore->lru_block_id[line->index] = min_block_id;

#ifdef DBG_ON
    cache = (cache_generic_t *) tagstore->cache;
    printf("%s, LRU index %u\n", CACHE_GET_NAME(cache), line->index);
    for (block_id = 0; block_id < num_blocks; ++block_id) {
        printf("    block %u, valid %u, age %lu\n",
                block_id, tag_data[block_id].valid, tag_data[block_id].age);
    }
 
    printf("%s, min_block %u, min_age %lu\n", CACHE_GET_NAME(cache), 
            min_block_id, min_age);
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
    cache_generic_t     *cache = NULL;

    if ((!tagstore) || (!line)) {
        cache_assert(0);
        goto error_exit;
    }

    cache = (cache_generic_t *) tagstore->cache;
    num_blocks = tagstore->num_blocks_per_set;
    tag_index = (line->index * num_blocks);
    tag_data = &tagstore->tag_data[tag_index];

    for (block_id = 0; block_id < num_blocks; ++block_id) {
        if (!tag_data[block_id].valid) {
            dprint_info("index %u, invalid block %u selected from %s\n", 
                    line->index, block_id, CACHE_GET_NAME(cache));
            return block_id;
        }
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
 *          set to write back, writes the block to next level of memory. 
 *
 * Params:
 *  cache       ptr to the cache containing the dirty block
 *  mem_ref     incoming memory reference
 *  block_id    ID of the block within the set which has to be evicted
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_handle_dirty_tag_evicts(cache_generic_t *cache, mem_ref_t *mem_ref, 
        uint32_t block_id)
{
    uint32_t            tag_index = 0;
    uint32_t            *tags = NULL;
    cache_line_t        line;
    cache_tagstore_t    *tagstore = NULL;
    cache_tag_data_t    *tag_data = NULL;

    if ((!cache) || (!mem_ref)) {
        cache_assert(0);
        goto exit;
    }
    tagstore = cache->tagstore;

    /* Decode the memmory reference to the current cache's cache line. */
    memset(&line, 0, sizeof(line));
    cache_util_decode_mem_addr(tagstore, mem_ref->ref_addr, &line);

    tag_index = (line.index * tagstore->num_blocks_per_set);
    tags = &tagstore->tags[tag_index];
    tag_data = &tagstore->tag_data[tag_index];


    /* 
     * If there's another level of cache, write the dirty block to the next
     * available cache.
     */
    if ((cache->next_cache) && (CACHE_WRITE_PLCY_WBWA == cache->write_plcy)) {
        mem_ref_t       write_ref;
        cache_line_t    write_line;

        /*
         * To write the dirty block to next cache, issue a write request to
         * the next cache by encoding the dirty tag to a memory ref addr and
         * the ref type as write.
         */
        memset(&write_line, 0, sizeof(write_line));
        memset(&write_ref, 0, sizeof(write_ref));
        write_line.tag = tags[block_id];;
        write_line.index = line.index;
        cache_util_encode_mem_addr(tagstore, &write_line, &write_ref);
        write_ref.ref_type = MEM_REF_TYPE_WRITE;

        dprint_dbg("LRU WRITE TO %s, INDEX %u, BLOCK %d, DIRTY %u\n",
            CACHE_GET_NAME(cache->next_cache), line.index, block_id, 
            cache_util_is_block_dirty(tagstore, &line, block_id));
        dprint_info("%s writing dirty block [%u, %d] to next level due "    \
                "to eviction", CACHE_GET_NAME(cache), line.index, block_id);

        cache_evict_and_add_tag(cache->next_cache, &write_ref);
    } else {
        dprint_dbg("LRU WRITE TO MEMORY, INDEX %u, BLOCK %d, DIRTY %u\n",
            line.index, block_id, 
            cache_util_is_block_dirty(tagstore, &line, block_id));
        dprint_info("%s writing dirty block [%u, %d] to memory due to eviction",
                CACHE_GET_NAME(cache), line.index, block_id);
    }

    /* Update the write back counter and clear the dirty bit on the block. */ 
    cache->stats.num_write_backs += 1;
    cache->stats.num_blk_mem_traffic += 1;
    tag_data[block_id].dirty = 0;

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

#if 0
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
#endif /* if 0 */
            break;

        default:
            cache_assert(0);
            goto error_exit;
    }
    dprint_dbg("LRU EVICT FROM %s, INDEX %u, BLOCK %d, DIRTY %u\n",
            CACHE_GET_NAME(cache), line->index, block_id, 
            cache_util_is_block_dirty(tagstore, line, block_id));
    dprint_info("LRU evict: %s, index %u , block %d, dirty %u\n", 
            CACHE_GET_NAME(cache), line->index, block_id, 
            cache_util_is_block_dirty(tagstore, line, block_id));
    
    /* If the block to be evicted is dirty, write it back if required. */
    if (cache_util_is_block_dirty(tagstore, line, block_id)) {
        dprint_info("selected a dirty block to evict in index %u, block %d\n",
                line->index, block_id);
        cache_handle_dirty_tag_evicts(cache, mref, block_id);
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
 *  in_cache    ptr to cache
 *  mem_ref     ptr to the memory reference (type and address)
 *
 * Returns: Nothing.
 **************************************************************************/
void
cache_evict_and_add_tag(cache_generic_t *cache, mem_ref_t *mref)
{
    uint8_t             read_flag = FALSE;
    int32_t             block_id = 0;
    uint32_t            tag_index = 0;
    uint32_t            *tags = NULL;
    uint64_t            curr_age;
    cache_line_t        line;
    cache_tag_data_t    *tag_data = NULL;
    cache_tagstore_t    *tagstore = NULL;

    if ((!cache) || (!mref)) {
        cache_assert(0);
        goto exit;
    }
    tagstore = cache->tagstore;

    /* Fetch the current time to be used for tag age (for LRU). */
    curr_age = util_get_curr_time(); 

    /* Decode the memmory reference to the current cache's cache line. */
    memset(&line, 0, sizeof(line));
    cache_util_decode_mem_addr(tagstore, mref->ref_addr, &line);

    /* Fetch the appropriate set within the tagstore. */
    tag_index = (line.index * tagstore->num_blocks_per_set);
    tags = &tagstore->tags[tag_index];
    tag_data = &tagstore->tag_data[tag_index];
    read_flag = (IS_MEM_REF_READ(mref) ? TRUE : FALSE);

    if (read_flag)
        cache->stats.num_reads += 1;
    else
        cache->stats.num_writes += 1;

    if (CACHE_RV_ERR != (block_id = cache_does_tag_match(tagstore, &line))) {
        /* 
         * Cache hit!
         * Tag is already present. Just update the counters and go fetch the
         * next memory reference.
         *
         * Life is good!
         */
        dprint_dbg("HIT %s\n", CACHE_GET_NAME(cache));
        dprint_info("cache hit for cache %s, tag 0x%x at index %u, block %u\n",
                CACHE_GET_NAME(cache), line.tag, line.index, block_id);
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
    } else {
        /* Cache miss! 
         * Well, life is not always so good..
         *
         * In multi level caches, do the following in case of a cache miss:
         *  1. If the next level in hierarchy is a cache, check if the 
         *     requested data is present there. If so, bring in the block
         *     and follow steps 2/3 for placing the block. i.e., find a
         *     free block or go for replacement.
         *  2. If the next level is not a cache (i.e., for last level of
         *     caches, the next level would be main memory), check if there
         *     are any free blocks. If so, use those blocks for new data.
         *  3. If there are no free blocks and if the cache is the last level,
         *     go for cache replacement.
         *
         * For all three cases, first find a block to place the to-be-feched
         * data block. 
         */
        dprint_dbg("MISS %s, TAG %x\n", CACHE_GET_NAME(cache), line.tag);
        dprint_info("cache miss for cache %s, tag 0x%x @ index %u\n",
                CACHE_GET_NAME(cache), line.tag, line.index);

        /* 
         * Find a block to place the to-be-fetcheed data. Go for block eviction
         * if no free blocks are available.
         */
        block_id = cache_get_first_invalid_block(tagstore, &line);
        if (CACHE_RV_ERR == block_id)
            block_id = cache_evict_tag(cache, mref, &line);
        dprint_info("cache %s, index %u, block %d selected for tag 0x%x\n",
                CACHE_GET_NAME(cache), line.index, block_id, line.tag);

        /* Check next level cache, if available. */ 
        if (cache->next_cache) {
            mem_ref_t       read_ref;

            /* 
             * For cache misses, issues a read reference for that address
             * to the next cache level.
             */
            memset(&read_ref, 0, sizeof(read_ref));
            memcpy(&read_ref, mref, sizeof(read_ref));
            read_ref.ref_type = MEM_REF_TYPE_READ;
            
            dprint_dbg("READ FROM %s %x\n", 
                    CACHE_GET_NAME(cache->next_cache), read_ref.ref_addr);
            cache_evict_and_add_tag(cache->next_cache, &read_ref);

            tags[block_id] = line.tag;
            cache->stats.num_blk_mem_traffic += 1;
            tag_data[block_id].valid = 1;
            tag_data[block_id].age = curr_age;
            tag_data[block_id].ref_count = 
                (util_get_block_ref_count(tagstore, &line) + 1);

            if (read_flag) {
                cache->stats.num_read_misses += 1;
            } else {
                cache->stats.num_write_misses += 1;

                /* Set the block to be dirty only for WBWA write policy. */
                if (CACHE_WRITE_PLCY_WBWA == CACHE_GET_WRITE_POLICY(cache))
                    tag_data[block_id].dirty = 1;
            }
            dprint_info("%s, tag 0x%x added to index %u, block %u\n", 
                    CACHE_GET_NAME(cache), line.tag, line.index, block_id);
        } else {
            /*
             * We are at the last cache and currently handling a miss. 
             * Read from memory and place it the previouly found block. 
             */
            tags[block_id] = line.tag;
            cache->stats.num_blk_mem_traffic += 1;
            tag_data[block_id].valid = 1;
            tag_data[block_id].age = curr_age;
            tag_data[block_id].ref_count = 
                (util_get_block_ref_count(tagstore, &line) + 1);

            if (read_flag) {
                cache->stats.num_read_misses += 1;
            } else {
                cache->stats.num_write_misses += 1;

                /* Set the block to be dirty only for WBWA write policy. */
                if (CACHE_WRITE_PLCY_WBWA == CACHE_GET_WRITE_POLICY(cache))
                    tag_data[block_id].dirty = 1;
            }
            dprint_info("%s, tag 0x%x added to index %u, block %u\n", 
                    CACHE_GET_NAME(cache), line.tag, line.index, block_id);
        }   /* End of last level cache processing */
    }   /* End of cache miss processing */

exit:
#ifdef DBG_ON
    cache_print_tags(cache, &line);
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
    cache_evict_and_add_tag(cache, mref);

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
        goto usage_exit;
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
        /* All requests start at L1 cache. */
        //cache_set_current_cache(&g_l1_cache, &g_l1_cache_ts);
        g_addr_count += 1;
        dprint_dbg("\n%u. Address %x %s\n", g_addr_count, mem_ref.ref_addr,
                CACHE_GET_REF_TYPE_STR((&mem_ref)));
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
    cache_print_cache_dbg_data(&g_l1_cache);
#endif /* DBG_ON */

    /* Dump the cache simulator configuration, cache state and statistics. */
    dprint_dbg("\n");
    cache_print_sim_config(&g_l1_cache);
    cache_print_cache_data(&g_l1_cache);
    if (cache_util_is_l2_present())
        cache_print_cache_data(&g_l2_cache);
    cache_print_sim_stats(&g_l1_cache);

    if (trace_fptr) 
        fclose(trace_fptr);

    if (cache_util_is_l2_present())
        cache_cleanup(&g_l2_cache);
    cache_cleanup(&g_l1_cache);

    return 0;

usage_exit:
    return -1;

error_exit:
    if (trace_fptr)
        fclose(trace_fptr);

    if (cache_util_is_l2_present())
        cache_cleanup(&g_l2_cache);
    cache_cleanup(&g_l1_cache);

    return -1;
}

