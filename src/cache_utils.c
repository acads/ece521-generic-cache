/* adhanas */

/*
 * Misc cache util function implementations. 
 */

/* Util macros */
#define CACHE_GET_REPL_PLCY_STR(VAL)                \
    do {                                            \
        if (VAL == CACHE_REPL_PLCY_LRU)             \
            "Least Recently Used"                   \
        else if (VAL == CACHE_REPL_PLCY_LFU)        \
            "Least Frequently Used"                 \
        else                                        \
            "Unknown"                               \
    } while (0)

#define CACHE_GET_WRITE_PLCY_STR(VAL)               \
    do {                                            \
        if (VAL == CACHE_WRITE_PLCY_WBWA)           \
            "Write Back Write Allocate"             \
        else if (VAL == CACHE_WRITE_PLCY_WTNA)      \
            "Write Through Not Allocate"            \
        else                                        \
            "Unknown"                               \
    } while (0)


/* Util functions */
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
    printf("Total Size: %u\n" pcache->size);
    printf("Replacement Policy: %s\n" 
            CACHE_GET_REPL_PLCY_STR((pcache->repl_plcy)));
    printf("Write Policy: %s\n",
            CACHE_GET_WRITE_PLCY_STR(pcache->write_plcy));
exit:
    return;
}


void
cache_print_stats(cache_stats-t *pstats, boolean detail)
{
    char    *spacing = "    ";

    if (NULL == pstats) {
        cache_assert(0);
        goto exit:
    }

    if (NULL == pstats->cache) {
        cache_assert(0);
        goto exit:
    }

    if (TRUE == detail) {
        printf("\n");
        spacing = "";
        printf("Cache Statistics\n");
        printf("----- ----------\n");
        printf("Name: %s\n", pstats->cache->name;);
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
