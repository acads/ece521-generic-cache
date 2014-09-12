/* adhanas */

/*
 * Misc cache utils constants, macros and util function declarations.
 */

#ifndef CACHE_UTILS_H_
#define CACHE_UTILS_H_

#include <assert.h>
#include "cache.h"

#ifdef DBG_ON
#define cache_assert(cond)      assert(cond)
#elif 
#define cache_assert(cond)
#endif /* DBG_ON */

/* Function declarations */
#ifdef DBG_ON
void
cache_print(cache_generic_t *pcache);
void
cache_print_stats(cache_stats_t *pcache_stats);
#endif /* DBG_ON */

#endif /* CACHE_UTILS_H_ */

