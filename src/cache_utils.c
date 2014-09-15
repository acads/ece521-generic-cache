/* adhanas */

/*
 * Misc cache util function implementations. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include "cache.h"
#include "cache_utils.h"


/* Util functions */
/*************************************************************************** 
 * Name:    util_get_msb_mask
 *
 * Desc:    Computes and returns a 32-bit unsigned MSB mask.
 *
 * Params:
 *  num_msb_bits    # of MSB bits to be masked
 *
 * Returns: uint32_t
 *  32-bit uint with num_msb_bits masked
 **************************************************************************/
static inline unsigned
util_get_msb_mask(uint32_t num_msb_bits)
{
    return ((~0U) << (32 - num_msb_bits));
}


/*************************************************************************** 
 * Name:   util_get_lsb_mask 
 *
 * Desc:    Computes and returns a 32-bit unsigned LSB mask.
 *
 * Params:
 *  num_lsb_bits    # of LSB bits to be masked
 *
 * Returns: uint32_t
 *  32-bit uint with num_lsb_bits masked
 **************************************************************************/
static inline unsigned
util_get_lsb_mask(uint32_t num_lsb_bits)
{
    return ((~0U) >> (32 - num_lsb_bits));
}


/*************************************************************************** 
 * Name:    util_get_field_mask
 *
 * Desc:    Computes and returns a 32-bit arbitrary mask with bits from
 *          start_bit thru end_bit (both including) masked.
 *
 * Params:
 *  start_bit   start of mask
 *  end_bit     end of mask
 *
 * Returns: uint32_t
 *  32-bit uint with bits from start_bit thru end_bit masked
 **************************************************************************/
static inline unsigned
util_get_field_mask(uint32_t start_bit, uint32_t end_bit)
{
    return (util_get_lsb_mask(end_bit + 1) & (~util_get_lsb_mask(start_bit)));
}


/*************************************************************************** 
 * Name:    util_is_power_of_2
 *
 * Desc:    Checks whether a given number is a power of 2 or not
 *
 * Params:
 *  num     unsigned 32-bit int which is to be tested for power of 2
 *
 * Returns: boolean
 *  TRUE if num is a power of two
 *  FALSE otherwise or if num is 0
 **************************************************************************/
boolean
util_is_power_of_2(uint32_t num)
{
    if (0 == num)
        return FALSE;

    if (num & (num - 1))
        return FALSE;

    return TRUE;
}


/*************************************************************************** 
 * Name:    util_log_base_2
 *
 * Desc:    Computes the log_base_2 of the given number
 *
 * Params:
 *  num     32-bit uint whose log_base_2 is to be computed
 *
 * Returns: uint32_t
 *  Returns the log_base_2 of num
 *
 * Note:    This is only for numbers that are power of 2
 **************************************************************************/
uint32_t
util_log_base_2(uint32_t num)
{
    uint32_t result = 0;
    uint32_t tmp = 0;

    tmp = num;
    while (tmp >>= 1)
        result += 1;

    return result;
}


/*************************************************************************** 
 * Name:    util_get_curr_time
 *
 * Desc:    Returns the current system time in useconds
 *
 * Params:  None
 *
 * Returns: uint64_t
 *  Current system time in useconds
 **************************************************************************/
inline uint64_t
util_get_curr_time(void)
{
    struct timeval  curr_time;

    memset(&curr_time, 0, sizeof(curr_time));
    gettimeofday(&curr_time, NULL);

    return ((curr_time.tv_sec * 1000000) + curr_time.tv_usec);
}


/*************************************************************************** 
 * Name:    cache_util_is_block_dirty
 *
 * Desc:    Checks whether the given block is dirty or not
 *
 * Params:
 *  tagstore    ptr to the tagstore
 *  line        ptr to decoded addr as cache line
 *  block_id    ID of the block to check for dirty bit
 *
 * Returns: boolean
 *  TRUE if the block is dirty
 *  FALSE otherwise
 **************************************************************************/
inline boolean
cache_util_is_block_dirty(cache_tagstore_t *tagstore, cache_line_t *line, 
        int32_t block_id)
{
    return 
        (tagstore->tag_data[((line->index * tagstore->num_blocks_per_set) + \
                             block_id)].dirty);
}


/*************************************************************************** 
 * Name:    cache_util_validate_input
 *
 * Desc:    Validates the user entered cache configuration. Checks the 
 *          following:
 *          1. Total # of cache config arguments to be 6 
 *          2. Block size to be a power of 2.
 *          3. Valid replacement and write policies. 
 *          4. Given trace file is readable or not.
 *
 * Params:
 *  nargs   # of input arguments
 *  args    ptr to user entered arguments
 *
 * Returns: boolean
 *  TRUE if all arguments are good
 *  FALSE otherwise
 **************************************************************************/
boolean
cache_util_validate_input(int nargs, char **args)
{
    int blk_size = 0;
    int plcy = 0;

    if (CACHE_INPUT_NUM_ARGS != (nargs - 1)) {
        dprint_err("bad number of args %u\n", nargs);
        return FALSE;
    }

    /* Block size should be a power of 2. */
    blk_size = atoi(args[1]);
    if (blk_size <= 0) {
        dprint_err("block size not power of 2 %u\n", blk_size);
        return FALSE;
    }
    if (blk_size & (blk_size - 1)) {
        dprint_err("block size not power of 2 %u\n", blk_size);
        return FALSE;
    }

    /* Only 0 and 1 are valid inputs for replacement/write policies. */
    plcy = atoi(args[4]);
    if ((CACHE_REPL_PLCY_LRU != plcy) && (CACHE_REPL_PLCY_LFU != plcy)) {
        dprint_err("bad replacement policy %u\n", plcy);
        return FALSE;
    }

    plcy = atoi(args[5]);
    if ((CACHE_WRITE_PLCY_WBWA != plcy) && (CACHE_WRITE_PLCY_WTNA != plcy)) {
        dprint_err("bad write policy %u\n", plcy);
        return FALSE;
    }

    /* Check if the trace-file is present and is readable. */
    if (access(args[nargs - 1], (F_OK | R_OK))) {
        dprint_err("bad trace file %s\n", args[nargs - 1]);
        return FALSE;
    }

    return TRUE;
}


/*************************************************************************** 
 * Name:    cache_util_print_usage
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
cache_util_print_usage(const char *prog)
{
    printf("Usage: %s <block-size> <cache-size> <set-assoc> "   \
            "<replacement-policy> <write-policy> <trace-file>\n", prog);
    printf("    block-size         : size of each cache block in bytes; "   \
            "must be a power of 2.\n");
    printf("    cache-size         : size of the entire cahce in bytes.\n");
    printf("    set-assoc          : set associativity of the cache.\n");
    printf("    replacement-policy : 0 LRU, 1 LFU.\n");
    printf("    write-policy       : 0 WBWA, 1 WTNA.\n");
    printf("    trace-file         : CPU memory access file with full "     \
            "path.\n");

    return;
}


/*************************************************************************** 
 * Name:    cache_util_decode_mem_addr
 *
 * Desc:    Decodes the incoming memory address reference into cache 
 *          understandable format in a cache line.
 *          i.e., <addr> = <tag, index, block_offset>
 *
 * Params:
 *  tagstore    ptr to the tagstore of the cache for which addr is decoded
 *  addr        incoming 32-bit memory address
 *  line        ptr to store the decoded addr
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_util_decode_mem_addr(cache_tagstore_t *tagstore, uint32_t addr, 
        cache_line_t *line)
{
    uint32_t tag_mask = 0;
    uint32_t index_mask = 0;
    uint32_t offset_mask = 0;

    if ((!line) || (!tagstore)) {
        cache_assert(0);
        goto exit;
    }

    tag_mask = util_get_msb_mask(tagstore->num_tag_bits);
    offset_mask = util_get_lsb_mask(tagstore->num_offset_bits);
    index_mask = 
        util_get_field_mask(tagstore->num_offset_bits, 
                (tagstore->num_offset_bits + 
                    tagstore->num_index_bits) - 1); 

    line->tag = ((addr & tag_mask) >> (32 - tagstore->num_tag_bits));
    line->index = ((addr & index_mask) >> tagstore->num_offset_bits);
    line->offset = (addr & offset_mask);

    dprint_info("addr 0x%x, tag 0x%x, index %u, offset %u\n", 
            addr, line->tag, line->index, line->offset);

exit:
    return;
}


#ifdef DBG_ON
/*************************************************************************** 
 * Name:    cache_util_print
 *
 * Desc:    Prints the cache details, configuration and statistics.
 *
 * Params:
 *  pcache  ptr to the cache whose details are to be printed
 *
 * Returns: Nothing
 **************************************************************************/
void 
cache_util_print(cache_generic_t *pcache)
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
    printf("Replacement Policy : %s\n", pcache->repl_plcy ? "LRU" : "LFU");
    printf("Write Policy       : %s\n", pcache->write_plcy ? "WBWA" : "WTNA");
    printf("Statistics\n");
    dprint("---------\n");
    cache_util_print_stats((cache_stats_t *) &(pcache->stats), FALSE);

exit:
    return;
}


/*************************************************************************** 
 * Name:    cache_util_print_stats
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
cache_util_print_stats(cache_stats_t *pstats, boolean detail)
{
    char    *spacing = "    ";

    if (NULL == pstats) {
        cache_assert(0);
        goto exit;
    }

    if (NULL == pstats->cache) {
        cache_assert(0);
        goto exit;
    }

    if (TRUE == detail) {
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
 * Name:    cache_util_print_tagstore
 *
 * Desc:    Pretty prints the cache tagstore details
 *
 * Params:
 *  pcache  ptr to cache whose details are to be printed
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_util_print_tagstore(cache_generic_t *cache)
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
#endif /* DBG_ON */


void
cache_util_print_debug_data(cache_generic_t *cache, cache_line_t *line)
{
    uint32_t            *tags = NULL;
    uint32_t            num_blocks = 0;
    uint32_t            block_id = 0;
    cache_tagstore_t    *tagstore = NULL;

    tagstore = cache->tagstore;
    tags = &tagstore->tags[line->index];
    num_blocks = tagstore->num_blocks_per_set;

    dprint("debug_set %u: ", line->index);
    for (block_id = 0; block_id < num_blocks; ++block_id)
        dprint("%8x", tags[block_id]);
    dprint("\n");

    return;
}
