/* adhanas */

/*
 * Misc cache utils constants, macros and util function declarations.
 */

#ifndef CACHE_UTILS_H_
#define CACHE_UTILS_H_

#include <assert.h>
#include "cache.h"

/* Constants */
#define CACHE_INPUT_NUM_ARGS    6

/* Util macros */
#ifdef DBG_ON
#define dprint_info(str, ...)               \
    printf("cache_info %s %s %u: " str,     \
            __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define dprint_warn(str, ...)               \
    printf("cache_warn %s %s %u: " str,     \
            __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define dprint_err(str, ...)                \
    printf("cache_err %s %s %u: " str,     \
            __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define dprint_info(str, ...)
#define dprint_warn(str, ...)
#define dprint_wrr(str, ...)
#endif /* DBG_ON */

#ifdef DBG_ON
#define cache_assert(cond)      assert(cond)
#else
#define cache_assert(cond)
#endif /* DBG_ON */

/* Function declarations */
boolean
cache_validate_input(int nargs, char **args);
void
cache_print_usage(const char *prog);
#ifdef DBG_ON
void
cache_print(cache_generic_t *pcache);
void
cache_print_stats(cache_stats_t *pcache_stats, boolean detail);
#endif /* DBG_ON */

#endif /* CACHE_UTILS_H_ */

