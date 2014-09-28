/* 
 * ECE 521 - Computer Design Techniques, Fall 2014
 * Project 1A - Generic Cache Implementation
 *
 * This module contains all required util function declrations for the 
 * cache implementation.
 *
 * Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
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

#ifndef DBG_ON
    /*
     * An ugly hack to prevent the same-time for different references due to
     * a usecond timer bug in non-QA code. The perfomance is still reasonable;
     * i.e., under 7s for an input trace of 100k references.
     *
     * QA code uses a lot of dprints, thus the time interval between two
     * consecutinve references of a same block is substantially higher than
     * in non-QA code.
     */
    usleep(1);
#endif /* !DBG_ON */
    gettimeofday(&curr_time, NULL);

    return ((curr_time.tv_sec * 1000000) + curr_time.tv_usec);
}


/*************************************************************************** 
 * Name:    util_get_block_ref_count
 *
 * Desc:    Returns the ref. count of the set where the block is present.
 *
 * Params:  
 *  tagstore    ptr to the tagstore containing the block
 *  line        cache line containing the index 
 *
 * Returns: uint32_t
 *  Current ref count of the set containing the block.
 **************************************************************************/
inline uint32_t
util_get_block_ref_count(cache_tagstore_t *tagstore, cache_line_t *line)
{
    return (tagstore->set_ref_count[line->index]);
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
 * Name:    cache_util_is_l2_present
 *
 * Desc:    Checks whether L2 cache is configured or not
 *
 * Params:  None
 *
 * Returns: boolean
 *  TRUE if L2 cache is configure and present
 *  FALSE otherwise
 **************************************************************************/
inline boolean
cache_util_is_l2_present(void)
{
    return (g_l2_present ? TRUE : FALSE);
}

    
/*************************************************************************** 
 * Name:    cache_util_is_victim_present 
 *
 * Desc:    Checks whether victim cache is configured or not
 *
 * Params:  None
 *
 * Returns: boolean
 *  TRUE if victim cache is configure and present
 *  FALSE otherwise
 **************************************************************************/
inline boolean
cache_util_is_victim_present(void)
{
    return (g_victim_present ? TRUE : FALSE);
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
 * Name:    util_compare_uint64
 *
 * Desc:    Non-increasing type qsort comparator for uint64_t
 *
 * Params:
 * a        ptr to data A
 * b        ptr to data B
 *
 * Returns: int
 * > 0, if b is greater than a
 * < 0, if a is greater than b
 * 0, if a and b are equal
 **************************************************************************/
int
util_compare_uint64(const void *a, const void *b)
{
    const uint64_t *loc_a = (const uint64_t *) a;
    const uint64_t *loc_b = (const uint64_t *) b;

    /* Sorts in non-inreasing order. */
    return (*loc_b - *loc_a);
}


/*************************************************************************** 
 * Name:    cache_util_validate_input
 *
 * Desc:    Validates the user entered cache configuration. Checks the 
 *          following:
 *          1. Total # of cache config arguments to be 7
 *          2. Block size to be a power of 2.
 *          3. Given trace file is readable or not.
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
    int         blk_size = 0;

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

    /* Check if the trace-file is present and is readable. */
    if (access(args[nargs - 1], (F_OK | R_OK))) {
        dprint_err("bad trace file %s\n", args[nargs - 1]);
        return FALSE;
    }

    return TRUE;
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

    if (!line) {
        dprint_err("null line\n");
        cache_assert(0);
        goto exit;
    }

    if (!tagstore) {
        dprint_err("null tagstore\n");
        cache_assert(0);
        goto exit;
    }

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


/***************************************************************************
 * Name:    cache_util_encode_mem_addr
 *
 * Desc:    Encodes the memory addr for the given line and cache.
 *
 * Params:
 *  tagstore    ptr to the tagstore of the cache for which addr is decoded
 *  line        cache_line containing the tag & index
 *  mref        ptr to the mem_ref_t struture to store the encoded addr
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_util_encode_mem_addr(cache_tagstore_t *tagstore, cache_line_t *line,
                mem_ref_t *mref)
{
    uint8_t         num_index_bits = 0;
    uint8_t         num_offset_bits = 0;
    cache_generic_t *cache = NULL;

    if ((!tagstore) || (!line) || (!mref)) {
        cache_assert(0);
        goto exit;
    }

    cache = (cache_generic_t *) tagstore->cache;
    num_index_bits = tagstore->num_index_bits;
    num_offset_bits = tagstore->num_offset_bits;

    mref->ref_addr = ((line->tag << (num_index_bits + num_offset_bits)) |
            (line->index << num_offset_bits));

    dprint_info("%s, addr_encode tag 0x%x, index %u, addr 0x%x\n",
            CACHE_GET_NAME(cache), line->tag, line->index, mref->ref_addr);

exit:
    return;
}


/***************************************************************************
 * Name:    cache_util_get_lru_block_id
 *
 * Desc:    Gets the LRU block ID for a given cache line.
 *
 * Params:
 *  tagstore    ptr to the tagstore of the cache for which addr is decoded
 *  line        cache line whose LRU block ID is reuqired
 *
 * Returns: int8_t
 *  LRU block ID if everything goes well
 *  CACHE_RV_ERR on failure
 **************************************************************************/
int8_t
cache_util_get_lru_block_id(cache_tagstore_t *tagstore, cache_line_t *line)
{
    uint8_t             block_id = 0;
    uint8_t             min_block_id = 0;
    uint64_t            min_block_age = 0;
    uint32_t            num_blocks = 0;
    cache_tag_data_t    *tag_data = NULL;

    if ((!tagstore) || (!line)) {
        cache_assert(0);
        goto error_exit;
    }

    num_blocks = tagstore->num_blocks_per_set;
    tag_data = &tagstore->tag_data[line->index * num_blocks];
    
    for (block_id = 0, min_block_age = tag_data[block_id].age; 
            block_id < num_blocks; ++block_id) {
        if ((tag_data[block_id].valid) && 
                (tag_data[block_id].age < min_block_age)) {
            min_block_id = block_id;
            min_block_age = tag_data[block_id].age;
        }
    }

    return min_block_id;

error_exit:
    return CACHE_RV_ERR;
}


