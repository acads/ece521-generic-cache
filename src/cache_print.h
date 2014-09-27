/* 
 * ECE 521 - Computer Design Techniques, Fall 2014
 * Project 1B - L1, L2 & victim cache implementation.
 *
 * This module contains all required util macros and util function 
 * declrations for the cache print functionalities..
 *
 * Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
 */

#ifndef CACHE_PRINT_H
#define CACHE_PRINT_H

void
ache_print_sim_config(cache_generic_t *cache);
void
cache_print_usage(const char *prog);
void
cache_print_sim_stats(cache_generic_t *cache);
void
cache_print_cache_data(cache_generic_t *cache);
void
cache_print_sim_config(cache_generic_t *cache);
void
cache_print_stats(cache_stats_t *pcache_stats, boolean detail);

#ifdef DBG_ON
void
cache_print_cache_dbg_data(cache_generic_t *pcache);
void
cache_print_tagstore(cache_generic_t *cache);
#endif /* DBG_ON */
void
cache_print_tags(cache_generic_t *cache, cache_line_t *line);

#endif /* CACHE_PRINT_H */

