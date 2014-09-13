/* adhanas */

/* 
 * Generic cache data structures and function declarations.
 */

#ifndef CACHE_H_
#define CACHE_H_

/* Constants */
#ifndef TRUE
#define TRUE    1
#endif /* TRUE */
#ifndef FALSE
#define FALSE   0
#endif /* FALSE */

#define CACHE_LEVEL_1           1
#define CACHE_LEVEL_2           2
#define CACHE_LEVEL_3           3
#define CACHE_NAME_LEN          24

#define CACHE_REPL_PLCY_LRU     0
#define CACHE_REPL_PLCY_LFU     1
#define CACHE_WRITE_PLCY_WBWA   0
#define CACHE_WRITE_PLCY_WTNA   1

#define MEM_REF_TYPE_READ       0
#define MEM_REF_TYPE_WRITE      1

/* Standard typedefs */
typedef unsigned char uchar;
typedef unsigned char boolean;

/* Memory reference: address and refernce type */
typedef struct mem_ref__ {
    uint8_t     ref_type;
    uint32_t    ref_addr;
} mem_ref_t;

/* Cahce line: addr = <tag, index, blk_offset> */
typedef struct cache_line__ {
    uint32_t    tag;
    uint32_t    index;
    uint32_t    offset;
} cache_line_t;

/* Cache tag store data structure */
typedef struct cache_tagstore__ {
    void            *cache;                 /* ptr ot parent cache      */
    uint32_t        num_sets;               /* # of sets in cache       */
    uint32_t        num_blocks;             /* # of blocks in cache     */
    uint32_t        num_blocks_per_set;     /* # of blocks per set      */
    uint8_t         num_tag_bits;           /* # of bits for tags       */
    uint8_t         num_index_bits;         /* # of bits for index      */
    uint8_t         num_offset_bits;        /* # of bits for blk offset */
    uint32_t        *index;                 /* ptr to tag indices       */
    uint32_t        *tags;                  /* ptr to tag array         */
    uint32_t        *valid;                 /* ptr to valid bits array  */
    uint32_t        *dirty;                 /* ptr to dirty bits array  */
} cache_tagstore_t;

/* Cache statistics data structure */
typedef struct cache_stats__ {
    uint32_t        num_reads;              /* # of reads               */
    uint32_t        num_writes;             /* # of writes              */
    uint32_t        num_read_hits;          /* # of read hits           */ 
    uint32_t        num_write_hits;         /* # of write hits          */ 
    uint32_t        num_read_misses;        /* # of read misses         */
    uint32_t        num_write_misses;       /* # of write misses        */
    uint32_t        num_blk_mem_traffic;    /* # of blks transferred    */
    void            *cache;                 /* ptr to parent cache      */
} cache_stats_t;

/* Generic cache data structure */
typedef struct cache_generic__ {
    char                name[CACHE_NAME_LEN];   /* name - L1, L2..          */
    uint8_t             level;                  /* 1, 2, 3 ..               */
    uint8_t             set_assoc;              /* level of associativity   */
    uint32_t            blk_size;               /* cache block size         */
    uint32_t            size;                   /* total cache size         */
    uint8_t             repl_plcy;              /* replacement policy       */
    uint8_t             write_plcy;             /* write policy             */
    cache_stats_t       stats;                  /* cache statistics         */
    cache_tagstore_t    *tagstore;              /* associated tagstore      */
    struct cache_generic__ *next_cache;         /* next higher level cache  */
    struct cache_generic__ *prev_cache;         /* prev lower level cache   */
} cache_generic_t;


/* Function declarations */
void
cache_init(cache_generic_t *pcache, const char *name, uint8_t level);
void
cache_cleanup(cache_generic_t *pcache);
void
cache_tagstore_init(cache_generic_t *cache, cache_tagstore_t *tagstore);
void
cache_tagstore_cleanup(cache_generic_t *cache, cache_tagstore_t *tagstore);

#endif /* CACHE_H_ */
