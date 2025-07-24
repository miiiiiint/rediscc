#ifndef REDISCC_DATA_STRUCTURE_SKIP_LIST_H
#define REDISCC_DATA_STRUCTURE_SKIP_LIST_H

#include "data_structs/sds.h"
#include <cstddef>

// Skip list constants
#define ZSKIPLIST_MAXLEVEL 32    // Maximum level for skip list nodes
#define ZSKIPLIST_P        0.25  // Probability for level promotion

// Range specification for score-based queries
typedef struct zrangespec {
  double min, max;
  int    minex, maxex;  // min/max exclusive flags
} zrangespec;

// Range specification for lexicographic queries
typedef struct zlexrangespec {
  sds min, max;
  int minex, maxex;  // min/max exclusive flags
} zlexrangespec;

typedef struct zskiplistNode {
  sds                   ele;
  double                score;
  struct zskiplistNode* backward;
  struct zskiplistLevel {
    struct zskiplistNode* forward;
    unsigned long         span;
  } level[];
} zskiplistNode;

typedef struct zskiplist {
  struct zskiplistNode *header, *tail;
  unsigned long         length;
  int                   level;
} zskiplist;

// Core skip list operations
zskiplist*     zslCreate( void );
void           zslFree( zskiplist* zsl );
zskiplistNode* zslInsert( zskiplist* zsl, double score, sds ele );
int            zslDelete( zskiplist* zsl, double score, sds ele, zskiplistNode** node );
zskiplistNode* zslFind( zskiplist* zsl, double score, sds ele );

// Rank and range operations
unsigned long  zslGetRank( zskiplist* zsl, double score, sds ele );
zskiplistNode* zslGetElementByRank( zskiplist* zsl, unsigned long rank );
unsigned long  zslDeleteRangeByScore( zskiplist* zsl, zrangespec* range, zskiplistNode** deleted );
unsigned long  zslDeleteRangeByRank( zskiplist*      zsl,
                                     unsigned int    start,
                                     unsigned int    end,
                                     zskiplistNode** deleted );

// Range query operations
zskiplistNode* zslFirstInRange( zskiplist* zsl, zrangespec* range );
zskiplistNode* zslLastInRange( zskiplist* zsl, zrangespec* range );
int            zslIsInRange( zskiplist* zsl, zrangespec* range );

// Utility functions
int            zslValueGteMin( double value, zrangespec* spec );
int            zslValueLteMax( double value, zrangespec* spec );
zskiplistNode* zslCreateNode( int level, double score, sds ele );
void           zslFreeNode( zskiplistNode* node );
int            zslRandomLevel( void );

#endif  //! REDISCC_DATA_STRUCTURE_SKIP_LIST_H
