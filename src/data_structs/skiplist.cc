#include "data_structs/skiplist.h"
#include "allocator/xmalloc.h"
#include <cstdlib>
#include <cstring>
#include <ctime>

// Initialize random seed for level generation
static int zsl_random_seed_initialized = 0;

/**
 * @brief Generate a random level for a new skip list node
 * @return Random level between 1 and ZSKIPLIST_MAXLEVEL
 */
int zslRandomLevel( void ) {
  if ( !zsl_random_seed_initialized ) {
    srand( (unsigned int)time( nullptr ) );
    zsl_random_seed_initialized = 1;
  }

  int level = 1;
  while ( ( rand() & 0xFFFF ) < ( ZSKIPLIST_P * 0xFFFF ) && level < ZSKIPLIST_MAXLEVEL ) {
    level++;
  }
  return level;
}

/**
 * @brief Create a new skip list node
 * @param level Number of levels for this node
 * @param score Score value
 * @param ele Element (SDS string)
 * @return New skip list node or nullptr on failure
 */
zskiplistNode* zslCreateNode( int level, double score, sds ele ) {
  zskiplistNode* zn = (zskiplistNode*)xmalloc( sizeof( *zn ) + level * sizeof( zn->level[ 0 ] ) );
  if ( zn == nullptr ) return nullptr;

  zn->score    = score;
  zn->ele      = ele;
  zn->backward = nullptr;

  // Initialize all levels
  for ( int i = 0; i < level; i++ ) {
    zn->level[ i ].forward = nullptr;
    zn->level[ i ].span    = 0;
  }

  return zn;
}

/**
 * @brief Free a skip list node
 * @param node Node to free
 */
void zslFreeNode( zskiplistNode* node ) {
  if ( node ) {
    sdsfree( node->ele );
    xfree( node );
  }
}

/**
 * @brief Create a new skip list
 * @return New skip list or nullptr on failure
 */
zskiplist* zslCreate( void ) {
  zskiplist* zsl = (zskiplist*)xmalloc( sizeof( *zsl ) );
  if ( zsl == nullptr ) return nullptr;

  zsl->level  = 1;
  zsl->length = 0;

  // Create header node with maximum level
  zsl->header = zslCreateNode( ZSKIPLIST_MAXLEVEL, 0, nullptr );
  if ( zsl->header == nullptr ) {
    xfree( zsl );
    return nullptr;
  }

  // Initialize header node levels
  for ( int i = 0; i < ZSKIPLIST_MAXLEVEL; i++ ) {
    zsl->header->level[ i ].forward = nullptr;
    zsl->header->level[ i ].span    = 0;
  }
  zsl->header->backward = nullptr;
  zsl->tail             = nullptr;

  return zsl;
}

/**
 * @brief Free a skip list and all its nodes
 * @param zsl Skip list to free
 */
void zslFree( zskiplist* zsl ) {
  if ( zsl == nullptr ) return;

  zskiplistNode* node = zsl->header->level[ 0 ].forward;
  zskiplistNode* next;

  // Free all nodes except header
  while ( node ) {
    next = node->level[ 0 ].forward;
    zslFreeNode( node );
    node = next;
  }

  // Free header node (it has no element to free)
  xfree( zsl->header );
  xfree( zsl );
}

/**
 * @brief Compare two elements for ordering
 * @param score1 First score
 * @param ele1 First element
 * @param score2 Second score
 * @param ele2 Second element
 * @return < 0 if first < second, 0 if equal, > 0 if first > second
 */
static int zslCompareElements( double score1, sds ele1, double score2, sds ele2 ) {
  if ( score1 != score2 ) {
    return score1 < score2 ? -1 : 1;
  }
  // If scores are equal, compare elements lexicographically
  if ( ele1 == nullptr && ele2 == nullptr ) return 0;
  if ( ele1 == nullptr ) return -1;
  if ( ele2 == nullptr ) return 1;
  return sdscmp( ele1, ele2 );
}

/**
 * @brief Insert an element into the skip list
 * @param zsl Skip list
 * @param score Score value
 * @param ele Element (SDS string) - will be duplicated
 * @return Pointer to inserted node or nullptr on failure
 */
zskiplistNode* zslInsert( zskiplist* zsl, double score, sds ele ) {
  zskiplistNode* update[ ZSKIPLIST_MAXLEVEL ];
  unsigned int   rank[ ZSKIPLIST_MAXLEVEL ];
  zskiplistNode* x;
  int            i, level;

  if ( zsl == nullptr ) return nullptr;

  x = zsl->header;
  // Search for insertion point and record update pointers
  for ( i = zsl->level - 1; i >= 0; i-- ) {
    // Store rank that is crossed to reach the insert position
    rank[ i ] = i == ( zsl->level - 1 ) ? 0 : rank[ i + 1 ];

    while ( x->level[ i ].forward
            && zslCompareElements(
                   x->level[ i ].forward->score, x->level[ i ].forward->ele, score, ele )
                   < 0 ) {
      rank[ i ] += x->level[ i ].span;
      x          = x->level[ i ].forward;
    }
    update[ i ] = x;
  }

  // Generate random level for new node
  level = zslRandomLevel();

  // If new level is higher than current max level, initialize new levels
  if ( level > zsl->level ) {
    for ( i = zsl->level; i < level; i++ ) {
      rank[ i ]                    = 0;
      update[ i ]                  = zsl->header;
      update[ i ]->level[ i ].span = zsl->length;
    }
    zsl->level = level;
  }

  // Create new node with duplicated element
  sds ele_copy = sdsdup( ele );
  if ( ele_copy == nullptr ) return nullptr;

  x = zslCreateNode( level, score, ele_copy );
  if ( x == nullptr ) {
    sdsfree( ele_copy );
    return nullptr;
  }

  // Insert node and update forward pointers and spans
  for ( i = 0; i < level; i++ ) {
    x->level[ i ].forward           = update[ i ]->level[ i ].forward;
    update[ i ]->level[ i ].forward = x;

    // Update span covered by update[i] as x is inserted here
    x->level[ i ].span           = update[ i ]->level[ i ].span - ( rank[ 0 ] - rank[ i ] );
    update[ i ]->level[ i ].span = ( rank[ 0 ] - rank[ i ] ) + 1;
  }

  // Increment span for untouched levels
  for ( i = level; i < zsl->level; i++ ) {
    update[ i ]->level[ i ].span++;
  }

  // Set backward pointer
  x->backward = ( update[ 0 ] == zsl->header ) ? nullptr : update[ 0 ];
  if ( x->level[ 0 ].forward ) {
    x->level[ 0 ].forward->backward = x;
  } else {
    zsl->tail = x;
  }

  zsl->length++;
  return x;
}

/**
 * @brief Find an element in the skip list
 * @param zsl Skip list
 * @param score Score to search for
 * @param ele Element to search for
 * @return Pointer to found node or nullptr if not found
 */
zskiplistNode* zslFind( zskiplist* zsl, double score, sds ele ) {
  zskiplistNode* x;
  int            i;

  if ( zsl == nullptr ) return nullptr;

  x = zsl->header;
  for ( i = zsl->level - 1; i >= 0; i-- ) {
    while ( x->level[ i ].forward
            && zslCompareElements(
                   x->level[ i ].forward->score, x->level[ i ].forward->ele, score, ele )
                   < 0 ) {
      x = x->level[ i ].forward;
    }
  }

  // Move to the actual node
  x = x->level[ 0 ].forward;

  // Check if we found the exact element
  if ( x && zslCompareElements( x->score, x->ele, score, ele ) == 0 ) {
    return x;
  }

  return nullptr;
}

/**
 * @brief Delete an element from the skip list
 * @param zsl Skip list
 * @param score Score of element to delete
 * @param ele Element to delete
 * @param node Output parameter for deleted node (can be nullptr)
 * @return 1 if element was deleted, 0 if not found
 */
int zslDelete( zskiplist* zsl, double score, sds ele, zskiplistNode** node ) {
  zskiplistNode* update[ ZSKIPLIST_MAXLEVEL ];
  zskiplistNode* x;
  int            i;

  if ( zsl == nullptr ) return 0;

  x = zsl->header;
  // Search for the element to delete
  for ( i = zsl->level - 1; i >= 0; i-- ) {
    while ( x->level[ i ].forward
            && zslCompareElements(
                   x->level[ i ].forward->score, x->level[ i ].forward->ele, score, ele )
                   < 0 ) {
      x = x->level[ i ].forward;
    }
    update[ i ] = x;
  }

  // Move to the actual node
  x = x->level[ 0 ].forward;

  // Check if we found the element to delete
  if ( x && zslCompareElements( x->score, x->ele, score, ele ) == 0 ) {
    // Remove node from all levels
    for ( i = 0; i < zsl->level; i++ ) {
      if ( update[ i ]->level[ i ].forward == x ) {
        update[ i ]->level[ i ].span    += x->level[ i ].span - 1;
        update[ i ]->level[ i ].forward  = x->level[ i ].forward;
      } else {
        update[ i ]->level[ i ].span -= 1;
      }
    }

    // Update backward pointer
    if ( x->level[ 0 ].forward ) {
      x->level[ 0 ].forward->backward = x->backward;
    } else {
      zsl->tail = x->backward;
    }

    // Remove empty levels
    while ( zsl->level > 1 && zsl->header->level[ zsl->level - 1 ].forward == nullptr ) {
      zsl->level--;
    }

    zsl->length--;

    if ( node ) {
      *node = x;
    } else {
      zslFreeNode( x );
    }

    return 1;
  }

  return 0;
}

/**
 * @brief Get the rank (1-based position) of an element
 * @param zsl Skip list
 * @param score Score to search for
 * @param ele Element to search for
 * @return Rank of element (1-based), or 0 if not found
 */
unsigned long zslGetRank( zskiplist* zsl, double score, sds ele ) {
  zskiplistNode* x;
  unsigned long  rank = 0;
  int            i;

  if ( zsl == nullptr ) return 0;

  x = zsl->header;
  for ( i = zsl->level - 1; i >= 0; i-- ) {
    while ( x->level[ i ].forward
            && zslCompareElements(
                   x->level[ i ].forward->score, x->level[ i ].forward->ele, score, ele )
                   <= 0 ) {
      rank += x->level[ i ].span;
      x     = x->level[ i ].forward;
    }

    // Check if we found the exact element
    if ( x->ele && zslCompareElements( x->score, x->ele, score, ele ) == 0 ) {
      return rank;
    }
  }

  return 0;
}

/**
 * @brief Get element by rank (1-based position)
 * @param zsl Skip list
 * @param rank Rank to search for (1-based)
 * @return Pointer to node at given rank, or nullptr if rank is out of bounds
 */
zskiplistNode* zslGetElementByRank( zskiplist* zsl, unsigned long rank ) {
  zskiplistNode* x;
  unsigned long  traversed = 0;
  int            i;

  if ( zsl == nullptr || rank == 0 || rank > zsl->length ) return nullptr;

  x = zsl->header;
  for ( i = zsl->level - 1; i >= 0; i-- ) {
    while ( x->level[ i ].forward && ( traversed + x->level[ i ].span ) <= rank ) {
      traversed += x->level[ i ].span;
      x          = x->level[ i ].forward;
    }

    if ( traversed == rank ) {
      return x;
    }
  }

  return nullptr;
}

/**
 * @brief Check if a value is greater than or equal to range minimum
 * @param value Value to check
 * @param spec Range specification
 * @return 1 if value >= min, 0 otherwise
 */
int zslValueGteMin( double value, zrangespec* spec ) {
  return spec->minex ? ( value > spec->min ) : ( value >= spec->min );
}

/**
 * @brief Check if a value is less than or equal to range maximum
 * @param value Value to check
 * @param spec Range specification
 * @return 1 if value <= max, 0 otherwise
 */
int zslValueLteMax( double value, zrangespec* spec ) {
  return spec->maxex ? ( value < spec->max ) : ( value <= spec->max );
}

/**
 * @brief Check if skip list contains any element in the given range
 * @param zsl Skip list
 * @param range Range specification
 * @return 1 if range overlaps with skip list, 0 otherwise
 */
int zslIsInRange( zskiplist* zsl, zrangespec* range ) {
  zskiplistNode* x;

  if ( zsl == nullptr || range == nullptr ) return 0;

  // Check if range is valid
  if ( range->min > range->max
       || ( range->min == range->max && ( range->minex || range->maxex ) ) ) {
    return 0;
  }

  // Check if tail is in range
  x = zsl->tail;
  if ( x == nullptr || !zslValueGteMin( x->score, range ) ) {
    return 0;
  }

  // Check if head is in range
  x = zsl->header->level[ 0 ].forward;
  if ( x == nullptr || !zslValueLteMax( x->score, range ) ) {
    return 0;
  }

  return 1;
}

/**
 * @brief Find first element in range
 * @param zsl Skip list
 * @param range Range specification
 * @return First element in range, or nullptr if none found
 */
zskiplistNode* zslFirstInRange( zskiplist* zsl, zrangespec* range ) {
  zskiplistNode* x;
  int            i;

  if ( zsl == nullptr || !zslIsInRange( zsl, range ) ) return nullptr;

  x = zsl->header;
  for ( i = zsl->level - 1; i >= 0; i-- ) {
    while ( x->level[ i ].forward && !zslValueGteMin( x->level[ i ].forward->score, range ) ) {
      x = x->level[ i ].forward;
    }
  }

  // Move to the actual node
  x = x->level[ 0 ].forward;

  // Check if node is within range
  if ( x && !zslValueLteMax( x->score, range ) ) {
    return nullptr;
  }

  return x;
}

/**
 * @brief Find last element in range
 * @param zsl Skip list
 * @param range Range specification
 * @return Last element in range, or nullptr if none found
 */
zskiplistNode* zslLastInRange( zskiplist* zsl, zrangespec* range ) {
  zskiplistNode* x;
  int            i;

  if ( zsl == nullptr || !zslIsInRange( zsl, range ) ) return nullptr;

  x = zsl->header;
  for ( i = zsl->level - 1; i >= 0; i-- ) {
    while ( x->level[ i ].forward && zslValueLteMax( x->level[ i ].forward->score, range ) ) {
      x = x->level[ i ].forward;
    }
  }

  // Check if node is within range
  if ( x && !zslValueGteMin( x->score, range ) ) {
    return nullptr;
  }

  return x;
}
