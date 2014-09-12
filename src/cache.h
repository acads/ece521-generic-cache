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
#define CACHE_NAME_LEN          16
#define CACHE_REPL_PLCY_LRU     0
#define CACHE_REPL_PLCY_LFU     1
#define CACHE_WRITE_PLCY_WBWA   0
#define CACHE_WRITE_PLCY_WTNA   1

/* Standard typedefs */
typedef unsigned char uchar;
typedef unsigned char boolean;

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
    char            name[CACHE_NAME_LEN];   /* name - L1, L2..          */
    uint8_t         level;                  /* 1, 2, 3 ..               */
    uint32_t        blk_size;               /* cache block size         */
    uint32_t        size;                   /* total cache size         */
    uint8_t         repl_plcy;              /* replacement policy       */
    uint8_t         write_plcy;             /* write policy             */
    cache_stats_t   stats;                  /* cache statistics         */
    struct cache_generic__ *next_cache;     /* next higher level cache  */
    struct cache_generic__ *prev_cache;     /* prev lower level cache   */
} cache_generic_t;


/* Function declarations */
void
cache_init(cache_generic_t *pcache, const char *name, uint8_t level);
void
cache_cleanup(cache_generic_t *pcache);

#endif /* CACHE_H_ */
