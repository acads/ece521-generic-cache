/* adhanas */

/* 
 * Generic cache implementation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
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


cache_rv
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


void
cache_evict_and_add_tag(cache_generic_t *cache, mem_ref_t *mem_ref, 
        cache_line_t *line)
{
    uint32_t            block_id = 0;
    uint8_t             read_flag = FALSE;
    uint32_t            *tags = NULL;
    cache_tag_data_t    *tag_data = NULL;
    cache_tagstore_t    *tagstore = NULL;

    tagstore = cache->tagstore;
    tags = &tagstore->tags[line->index];
    tag_data = &tagstore->tag_data[line->index];
    read_flag = (IS_MEM_REF_READ(mem_ref) ? TRUE : FALSE);

    if (read_flag)
        cache->stats.num_reads += 1;
    else
        cache->stats.num_writes += 1;


    if (FALSE == cache_is_any_block_valid(tagstore, line)) {
        /* All blocks are invalid, place the tag in the 1st block. */
        block_id = 0;
        tags[block_id] = line->tag;
        tag_data[block_id].valid = 1;
        tag_data[block_id].age += 1;
        if (read_flag) {
            cache->stats.num_read_misses += 1;
        } else {
            cache->stats.num_write_misses += 1;
            tag_data[block_id].dirty = 1;
        }
        dprint_info("tag 0x%x added to index %u, block %u\n", 
                line->tag, line->index, block_id);
    } else if (CACHE_RV_ERR != 
            (block_id = cache_does_tag_match(tagstore, line))) {
        /* 
         * Tag is already present. Just update the tag_data and write thru 
         * if required.
         */
        tag_data[block_id].valid = 1;
        tag_data[block_id].age += 1;
        if (!read_flag) {
            cache->stats.num_writes += 1;
            tag_data[block_id].dirty = 1;
        }
        dprint_info("tag 0x%x already present in index %u, block %u\n",
                line->tag, line->index, block_id);
    }
#if 0
    } else if () {
    } else if () {
    } else {
    }
#endif

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

    /* 
     * Read the trace file, fetch the address and process the memory access
     * request for every request in the trace file. 
     */
    while (fscanf(trace_fptr, "%c %x%c", 
                &(mem_ref.ref_type), &(mem_ref.ref_addr), &newline) != EOF) {
        dprint_info("ref_type %c, ref_addr 0x%x\n", 
                mem_ref.ref_type, mem_ref.ref_addr);
        dprint("%c %x\n", mem_ref.ref_type, mem_ref.ref_addr);
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
