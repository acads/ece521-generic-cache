/* adhanas */

/*
 * Misc cache util function implementations. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "cache.h"
#include "cache_utils.h"


/* Util functions */
static inline unsigned
util_get_msb_mask(uint32_t num_msb_bits)
{
    return ((~0U) << (32 - num_msb_bits));
}


static inline unsigned
util_get_lsb_mask(uint32_t num_lsb_bits)
{
    return ((~0U) >> (32 - num_lsb_bits));
}


static inline unsigned
util_get_field_mask(uint32_t start_bit, uint32_t end_bit)
{
    return (util_get_lsb_mask(end_bit + 1) & (~util_get_lsb_mask(start_bit)));
}


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
    index_mask = util_get_field_mask(tagstore->num_tag_bits, 
            (tagstore->num_tag_bits + tagstore->num_index_bits - 1));

    line->tag = ((addr & tag_mask) >> (32 - tagstore->num_tag_bits));
    line->index = ((addr & index_mask) >> tagstore->num_offset_bits);
    line->offset = (addr & offset_mask);

    dprint_info("addr 0x%x, tag 0x%x, index %u, offset %u\n", 
            addr, line->tag, line->index, line->offset);

exit:
    return;
}


#ifdef DBG_ON
void 
cache_util_print(cache_generic_t *pcache)
{
    if (NULL == pcache) {
        cache_assert(0);
        goto exit;
    }

    printf("\n");
    printf("Cache Details\n");
    printf("----- -------\n");
    printf("Name: %s\n", pcache->name);
    printf("Level: %u\n", pcache->level);
    printf("Block Size: %u\n", pcache->blk_size);
    printf("Total Size: %u\n", pcache->size);
    printf("Replacement Policy: %s\n", pcache->repl_plcy ? "LRU" : "LFU");
    printf("Write Policy: %s\n", pcache->write_plcy ? "WBWA" : "WTNA");
    printf("Statistics:\n");
    cache_util_print_stats((cache_stats_t *) &(pcache->stats), FALSE);

exit:
    return;
}


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
        printf("----- ----------\n");
        printf("Name: %s\n", cache->name);
    }

    printf("%sNum Reads: %u\n", spacing, pstats->num_reads);
    printf("%sNum Read Misses: %u\n", spacing, pstats->num_read_misses);
    printf("%sNum Writes: %u\n", spacing, pstats->num_writes);
    printf("%sNum Write Misses: %u\n", spacing, pstats->num_write_misses);
    printf("%sMemory Traffic: %u blocks\n", spacing, 
            pstats->num_blk_mem_traffic);

    if (TRUE == detail) 
       printf("\n");

exit:
    return;
}


void
cache_util_print_tagstore(cache_generic_t *cache)
{
    if (!cache) 
        cache_assert(0);

    dprint("Cache Tag Store Statistics\n");
    dprint("--------------------------\n");
    dprint("# of sets: %u\n", cache->tagstore->num_sets);
    dprint("# of blocks: %u\n", cache->tagstore->num_blocks);
    dprint("# of blocks/set: %u\n", cache->tagstore->num_blocks_per_set);
    dprint("# of tag index block bits: %u %u %U\n", 
            cache->tagstore->num_tag_bits, cache->tagstore->num_index_bits, 
            cache->tagstore->num_offset_bits);

    return;

}
#endif /* DBG_ON */


boolean
util_is_power_of_2(uint32_t num)
{
    if (0 == num)
        return FALSE;

    if (num & (num - 1))
        return FALSE;

    return TRUE;
}


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
