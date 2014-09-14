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

void
cache_init(cache_generic_t *pcache, const char *name, uint8_t level)
{
    if ((NULL == pcache) || (NULL == name)) {
        cache_assert(0);
        goto exit;
    }

    memset(pcache, 0, sizeof *pcache);
    pcache->level = level;
    strncpy(pcache->name, name, CACHE_NAME_LEN - 1);
    pcache->stats.cache = pcache;
    pcache->next_cache = NULL;
    pcache->prev_cache = NULL;

exit:
    return;
}


void
cache_tagstore_init(cache_generic_t *cache, cache_tagstore_t *tagstore)
{
    uint8_t     tag_bits = 0;
    uint8_t     index_bits = 0;
    uint8_t     blk_offset_bits = 0;
    //uint32_t    blk_id = 0;
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

    /* Assoicate the tagstore to the given cache. */
    cache->tagstore = tagstore;

#ifdef DBG_ON
    cache_util_print_tagstore(cache);
#endif /* DBG_ON */

exit:
    return;

fatal_exit:
    /* Fatal exit. Quit the program. */
    exit(-1);
}


void
cache_cleanup(cache_generic_t *pcache)
{
    if (NULL == pcache) {
        cache_assert(0);
        goto exit;
    }

    memset(pcache, 0, sizeof *pcache);

exit:
    return;
}


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

exit:
    return;
}


boolean
cache_is_any_block_valid(cache_tagstore_t *tagstore, cache_line_t *line)
{
    uint32_t            block_id = 0;
    uint32_t            index = 0;
    uint32_t            num_blocks = 0;
    cache_tag_data_t    *tag_data = NULL;

    if ((!tagstore) || (!line)) {
        cache_assert(0);
        goto exit;
    }   

    index = line->index;
    num_blocks = tagstore->num_blocks_per_set;
    tag_data = &tagstore->tag_data[index];

    /*  
     * Go over all the blocks for the given index. If any of the blocks is
     * valid, return it's index. Else, return CACHE_RV_ERR.
     */
    for (block_id = 0; block_id < num_blocks; ++block_id) {
        if (tag_data[block_id].valid)
            return TRUE;
    }   

exit:
    return FALSE;
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
    uint64_t            min_age = 0;
    cache_tag_data_t    *tag_data = NULL;

    if ((!tagstore) || (!mref) || (!line)) {
        cache_assert(0);
        goto error_exit;
    }

    num_blocks = tagstore->num_blocks_per_set;
    tag_data = &tagstore->tag_data[line->index];

    for (block_id = 0; block_id < num_blocks; ++block_id) {
        if ((tag_data[block_id].valid) && tag_data[block_id].age < min_age) {
            min_block_id = block_id;
            min_age = tag_data[block_id].age;
        }
    }
    
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
    /* dan_todo: code for LFU policy. */
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
 *          ID of the frist available blocl
 *          CAHCE_RV_ERR if not such block is available
 **************************************************************************/
int32_t
cache_get_first_invalid_block(cache_tagstore_t *tagstore, cache_line_t *line)
{
    int32_t             block_id = 0;
    uint32_t            num_blocks = 0;
    cache_tag_data_t    *tag_data = NULL;

    if ((!tagstore) || (!line)) {
        cache_assert(0);
        goto error_exit;
    }

    num_blocks = tagstore->num_blocks_per_set;
    tag_data = &tagstore->tag_data[line->index];

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
    uint32_t            index = 0;
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
    index = line->index;
    tags = &tagstore->tags[index];
    tag_data = &tagstore->tag_data[index];

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
cache_handle_dirty_tag_evicts(cache_tagstore_t *tagstore, uint32_t block_id)
{
    /* dan_todo: add code for handling dirty evicts. */
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
    cache_tagstore_t    *tagstore = NULL;

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
        cache_handle_dirty_tag_evicts(tagstore, block_id);
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
    uint32_t            block_id = 0;
    uint8_t             read_flag = FALSE;
    uint32_t            *tags = NULL;
    uint64_t            curr_age;
    cache_tag_data_t    *tag_data = NULL;
    cache_tagstore_t    *tagstore = NULL;
    struct timeval      curr_time;

    /* Fetch the current time to be used for tag age. */
    memset(&curr_time, 0, sizeof(curr_time));
    gettimeofday(&curr_time, NULL);
    curr_age = ((curr_time.tv_sec * 1000000) + curr_time.tv_usec);

    tagstore = cache->tagstore;
    tags = &tagstore->tags[line->index];
    tag_data = &tagstore->tag_data[line->index];
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
        if (read_flag) {
            cache->stats.num_read_hits += 1;
        } else {
            cache->stats.num_write_hits += 1;
            tag_data[block_id].dirty = 1;
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
        tags[block_id] = line->tag;

        tag_data[block_id].valid = 1;
        tag_data[block_id].age = curr_age;
        if (read_flag) {
            cache->stats.num_read_misses += 1;
        } else {
            cache->stats.num_write_misses += 1;
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
        block_id = cache_evict_tag(cache, mem_ref, line);
        tags[block_id] = line->tag;

        tag_data[block_id].valid = 1;
        tag_data[block_id].age = curr_age;
        if (read_flag) {
            cache->stats.num_read_misses += 1;
        } else {
            cache->stats.num_write_misses += 1;
            tag_data[block_id].dirty = 1;
        }
        dprint_info("tag 0x%x added to index %u, block %d after eviction\n",
                line->tag, line->index, block_id);
    }

    return;
}


void
cache_handle_read_request(cache_generic_t *cache, mem_ref_t *mem_ref, 
        cache_line_t *line)
{
    if ((!cache) || (!mem_ref) || (!line)) {
        cache_assert(0);
        goto exit;
    }

    cache_evict_and_add_tag(cache, mem_ref, line);
#if 0
    cache_util_increment_reads(cache);
    /*
     * 1. For this index, check if all blocks are invalid. If so, it's a 
     *    cache miss.
     * 2. If there is even a single valid block, check for tag match. If 
     *    there's a match:
     *      a. Check for dirty bit (only for WBWA policy).  If dirty, write
     *         the block and mark block as not-dirty.
     */
    if (FALSE == cache_is_any_block_valid(cache->tagstore, line)) {
        dprint_info("cache miss for %c 0x%x\n", 
                mem_ref->ref_type, mem_ref->ref_addr);
        cache_util_increment_read_miss(cache);

        /* All blocks are invalid. So, place the tag in the 1st block. */
        cache_add_tag_to_block(cache->tagstore, mem_ref, line, 0);
    }
#endif

exit:
    return;
}


void
cache_handle_write_request(cache_generic_t *cache, mem_ref_t *mem_ref, 
        cache_line_t *line)
{
    return;
}


boolean
cache_handle_memory_request(cache_generic_t *cache, mem_ref_t *mem_ref)
{
    cache_line_t cache_line;

    if ((!cache) || (!mem_ref)) {
        cache_assert(0);
        goto error_exit;
    }

    memset(&cache_line, 0, sizeof cache_line);

    cache_util_decode_mem_addr(cache->tagstore, mem_ref->ref_addr, 
            &cache_line);

    if (IS_MEM_REF_READ(mem_ref)) {
        cache_handle_read_request(cache, mem_ref, &cache_line);
    } else if (IS_MEM_REF_WRITE(mem_ref)) {
        cache_handle_write_request(cache, mem_ref, &cache_line);
    } else {
        cache_assert(0);
        goto error_exit;
    }

    return TRUE;

error_exit:
    return FALSE;
}


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
    cache_init(&g_cache, "generic cache", CACHE_LEVEL_1);
    g_cache.blk_size = atoi(argv[arg_iter++]);
    g_cache.size = atoi(argv[arg_iter++]);
    g_cache.set_assoc = atoi(argv[arg_iter++]);
    g_cache.repl_plcy = atoi(argv[arg_iter++]);
    g_cache.write_plcy = atoi(argv[arg_iter++]);
    trace_fpath = argv[arg_iter];

    /* Initialize a tagstore for the given cache. */
    cache_tagstore_init(&g_cache, &g_cache_ts);

    /* Try opening the trace file. */
    trace_fptr = fopen(trace_fpath, "r");
    if (!trace_fptr) {
        printf("Error: Unable to open trace file %s.\n", trace_fpath);
        goto error_exit;
    }

#if 0
    {
        uint32_t addr = 0x4002e850;
        cache_line_t tmp;

        cache_util_decode_mem_addr(g_cache.tagstore, addr, &tmp);
        dprint("addr 0x%x, tag 0x%x, index 0x%u, offset %u\n",
                addr, tmp.tag, tmp.index, tmp.offset);
        return 0;
    }
#endif

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

    if (trace_fptr) 
        fclose(trace_fptr);

    return 0;

error_exit:
    if (trace_fptr)
        fclose(trace_fptr);

    return -1;
}
