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

cache_generic_t     g_l1_cache;     /* generic cache, l1 as of now */

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
    pcache->next_cache = NULL;
    pcache->prev_cache = NULL;

exit:
    return;
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


int
main(int argc, char **argv)
{

    if (FALSE == cache_validate_input(argc, argv)) {
        printf("Error: Invalid input(s). See usage for help.\n");
        cache_print_usage(argv[0]);
        goto exit_error;
    }

    return 0;

exit_error:
    return -1;
}
