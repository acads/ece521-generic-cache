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
    uint32_t    num_sets = 0;
    uint32_t    iter = 0;

    if ((NULL == cache) || (NULL == tagstore)) {
        cache_assert(0);
        goto exit;
    }

    /* 
     * # of tags that could be accomodated is same as the # of sets.
     * # of sets = (cache_size / (set_assoc * block_size))
     */
    tagstore->num_sets = num_sets = 
        (cache->size / (cache->set_assoc * cache->blk_size));
    dprint_info("# of sets is %u\n", tagstore->num_sets);

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
    tagstore->num_blocks = num_sets * cache->set_assoc;


    dprint_info("num_sets %u, tag_bits %u, index_bits %u, "     \
            "blk_offset_bits %u\n",
            num_sets, tag_bits, index_bits, blk_offset_bits);

    /* Allocate memory to store tags, valid and dirty bits. */
    tagstore->tags = 
        calloc(1, (cache->set_assoc * num_sets * sizeof (uint32_t)));
    tagstore->index = 
        calloc(1, (num_sets * sizeof(uint32_t)));
    tagstore->valid = calloc(1, (num_sets * sizeof(uint8_t)));
    tagstore->dirty = calloc(1, (num_sets * sizeof(uint8_t)));

    if ((!tagstore->tags) || (!tagstore->index) || 
            (!tagstore->valid) || (!tagstore->dirty)) {
        printf("Error: Unable to allocate memory for cache tagstore.\n");
        cache_assert(0);
        goto fatal_exit;
    }

    /* Initialize indices. */
    for (iter = 0; iter < num_sets; ++iter)
        tagstore->index[iter] = iter;


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

    free(tagstore->tags);
    free(tagstore->valid);
    free(tagstore->dirty);

exit:
    return;
}


boolean
cache_handle_memory_ref_request(cache_generic_t *cache, mem_ref_t *mem_ref)
{
    cache_line_t cache_line;

    if ((!cache) || (!mem_ref)) {
        cache_assert(0);
        goto error_exit;
    }

    memset(&cache_line, 0, sizeof cache_line);

    cache_util_decode_mem_addr(cache->tagstore, mem_ref->ref_addr, 
            &cache_line);

    /* lol */

    return TRUE;

error_exit:
    return FALSE;
}


int
main(int argc, char **argv)
{
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

#ifdef DBG_ON
    cache_util_print(&g_cache);
#endif /* DBG_ON */

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
    while (fscanf(trace_fptr, "%c %u", 
                &(mem_ref.ref_type), &(mem_ref.ref_addr)) != EOF) {
        dprint_info("ref_type %c, ref_addr 0x%x\n", 
                mem_ref.ref_type, mem_ref.ref_addr);
        if (!cache_handle_memory_ref_request(&g_cache, &mem_ref)) {
            dprint_err("Error: Unable to handle memory reference request for "\
                    "type %c, addr 0x%x.\n", 
                    mem_ref.ref_type, mem_ref.ref_addr);
            goto error_exit;
        }
    }

    if (trace_fptr) 
        fclose(trace_fptr);

    return 0;

error_exit:
    if (trace_fptr)
        fclose(trace_fptr);

    return -1;
}
