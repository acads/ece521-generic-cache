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

/* Util macros */


/* Util functions */
boolean
cache_validate_input(int nargs, char **args)
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
cache_print_usage(const char *prog)
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

#ifdef DBG_ON
void 
cache_print(cache_generic_t *pcache)
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

exit:
    return;
}


void
cache_print_stats(cache_stats_t *pstats, boolean detail)
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

#endif /* DBG_ON */
