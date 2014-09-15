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
#define IS_MEM_REF_READ(REF)    (MEM_REF_TYPE_READ == REF->ref_type)
#define IS_MEM_REF_WRITE(REF)   (MEM_REF_TYPE_WRITE == REF->ref_type)

#define CACHE_GET_REPLACEMENT_POLICY(CACHE)     (CACHE->repl_plcy)

#define dprint(str, ...)     printf(str, ##__VA_ARGS__)
#ifdef DBG_ON
#define dprint_info(str, ...)               \
    printf("cache_info %s %u>> " str, __func__, __LINE__, ##__VA_ARGS__)
#define dprint_warn(str, ...)               \
    printf("cache_warn: %s %s %u# " str,     \
            __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define dprint_err(str, ...)                \
    printf("cache_err: %s %s %u# " str,     \
            __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define dprint_info(str, ...)
#define dprint_warn(str, ...)
#define dprint_err(str, ...)
#endif /* DBG_ON */

#ifdef DBG_ON
#define cache_assert(cond)      assert(cond)
#else
#define cache_assert(cond)
#endif /* DBG_ON */


/* Function declarations */
inline boolean
cache_util_is_block_dirty(cache_tagstore_t *tagstore, cache_line_t *line, 
        int32_t block_id);
boolean
cache_util_validate_input(int nargs, char **args);
void
cache_util_print_usage(const char *prog);
void
cache_util_decode_mem_addr(cache_tagstore_t *tagstore, uint32_t addr, 
        cache_line_t *line);

#ifdef DBG_ON
void
cache_util_print(cache_generic_t *pcache);
void
cache_util_print_stats(cache_stats_t *pcache_stats, boolean detail);
void
cache_util_print_tagstore(cache_generic_t *cache);
#endif /* DBG_ON */

boolean
util_is_power_of_2(uint32_t num);
uint32_t
util_log_base_2(uint32_t num);
inline uint64_t
util_get_curr_time(void);

#endif /* CACHE_UTILS_H_ */

