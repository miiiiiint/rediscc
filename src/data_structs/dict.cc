#include "data_structs/dict.h"
#include "allocator/xmalloc.h"
#include <cassert>
#include <cstring>
#include <ctime>

// Hash function constants
#define DICT_HASH_FUNCTION_SEED 5381

// Global resize enable flag
static int          dict_can_resize         = 1;
static unsigned int dict_force_resize_ratio = 5;

// Private function declarations
static int           _dictExpandIfNeeded( dict* ht );
static unsigned long _dictNextPower( unsigned long size );
static long _dictKeyIndex( dict* ht, const void* key, uint64_t hash, dictEntry** existing );
static int  _dictInit( dict* ht, dictType* type );
static int  dictTypeExpandAllowed( dict* d );
static void _dictRehashStep( dict* d );
static void _dictClear( dict* d, int htidx, void( callback )( dict* ) );

/**
 * @brief Hash function seed for dictionary
 */
static uint64_t dict_hash_function_seed = DICT_HASH_FUNCTION_SEED;

/**
 * @brief MurmurHash2, by Austin Appleby
 *
 * This is a high-quality, fast hash function suitable for hash tables.
 * MurmurHash2 is chosen because:
 * 1. Excellent distribution properties - minimizes hash collisions
 * 2. Fast performance on modern CPUs
 * 3. Good avalanche effect - small input changes cause large output changes
 * 4. Widely tested and used in production systems (Redis, Cassandra, etc.)
 *
 * Note - This code makes a few assumptions about how your machine behaves:
 * 1. We can read a 4-byte value from any address without crashing
 * 2. sizeof(int) == 4
 */
uint64_t dictGenHashFunction( const void* key, size_t len ) {
  const uint64_t m    = 0xc6a4a7935bd1e995ULL;
  const int      r    = 47;
  uint64_t       seed = dict_hash_function_seed;
  uint64_t       h    = seed ^ ( len * m );

  const uint64_t* data = (const uint64_t*)key;
  const uint64_t* end  = data + ( len / 8 );

  while ( data != end ) {
    uint64_t k  = *data++;
    k          *= m;
    k          ^= k >> r;
    k          *= m;
    h          ^= k;
    h          *= m;
  }

  const unsigned char* data2 = (const unsigned char*)data;
  switch ( len & 7 ) {
  case 7: h ^= (uint64_t)( data2[ 6 ] ) << 48; [[fallthrough]];
  case 6: h ^= (uint64_t)( data2[ 5 ] ) << 40; [[fallthrough]];
  case 5: h ^= (uint64_t)( data2[ 4 ] ) << 32; [[fallthrough]];
  case 4: h ^= (uint64_t)( data2[ 3 ] ) << 24; [[fallthrough]];
  case 3: h ^= (uint64_t)( data2[ 2 ] ) << 16; [[fallthrough]];
  case 2: h ^= (uint64_t)( data2[ 1 ] ) << 8; [[fallthrough]];
  case 1: h ^= (uint64_t)( data2[ 0 ] ); h *= m;
  }

  h ^= h >> r;
  h *= m;
  h ^= h >> r;
  return h;
}

/**
 * @brief Reset a hash table already initialized with ht_init()
 */
static void _dictReset( dict* d, int htidx ) {
  d->ht_table[ htidx ]    = nullptr;
  d->ht_used[ htidx ]     = 0;
  d->ht_size_exp[ htidx ] = -1;
}

/**
 * @brief Create a new hash table
 */
dict* dictCreate( dictType* type ) {
  dict* d = (dict*)xmalloc( sizeof( *d ) );
  if ( d == nullptr ) return nullptr;

  _dictInit( d, type );
  return d;
}

/**
 * @brief Initialize the hash table
 */
static int _dictInit( dict* d, dictType* type ) {
  _dictReset( d, 0 );
  _dictReset( d, 1 );
  d->type            = type;
  d->rehashidx       = -1;
  d->pauserehash     = 0;
  d->useStoredKeyApi = 0;
  d->pauseAutoResize = 0;
  return DICT_OK;
}

/**
 * @brief Resize the table to the minimal size that contains all the elements
 */
int dictResize( dict* d ) {
  unsigned long minimal = d->ht_used[ 0 ];
  if ( minimal < 4 ) minimal = 4;
  return dictExpand( d, minimal );
}

/**
 * @brief Expand or create the hash table
 */
int dictExpand( dict* d, unsigned long size ) {
  if ( dictIsRehashing( d ) || d->ht_used[ 0 ] > size ) return DICT_ERR;

  // Calculate the real size
  unsigned long realsize = _dictNextPower( size );
  if ( realsize == DICTHT_SIZE( d->ht_size_exp[ 0 ] ) ) return DICT_ERR;

  // Allocate the new hash table and initialize all pointers to NULL
  dictEntry** new_ht_table = (dictEntry**)xcalloc( realsize * sizeof( dictEntry* ) );
  if ( new_ht_table == nullptr ) return DICT_ERR;

  // Calculate the new size exponent
  signed char new_ht_size_exp = 0;
  while ( ( 1UL << new_ht_size_exp ) < realsize )
    new_ht_size_exp++;

  // Is this the first initialization? If so it's not really a rehashing
  if ( d->ht_table[ 0 ] == nullptr ) {
    d->ht_size_exp[ 0 ] = new_ht_size_exp;
    d->ht_used[ 0 ]     = 0;
    d->ht_table[ 0 ]    = new_ht_table;
    return DICT_OK;
  }

  // Prepare a second hash table for incremental rehashing
  d->ht_size_exp[ 1 ] = new_ht_size_exp;
  d->ht_used[ 1 ]     = 0;
  d->ht_table[ 1 ]    = new_ht_table;
  d->rehashidx        = 0;
  return DICT_OK;
}

/**
 * @brief Check if dictionary is currently rehashing
 */
int dictIsRehashing( dict* d ) { return d->rehashidx != -1; }

/**
 * @brief Return the next power of 2 >= size
 */
static unsigned long _dictNextPower( unsigned long size ) {
  unsigned long i = 4;  // Start with 4, the minimum hash table size
  if ( size >= ULONG_MAX / 2 ) return ULONG_MAX;
  while ( 1 ) {
    if ( i >= size ) return i;
    i *= 2;
  }
}

/**
 * @brief Returns the index of a free slot that can be populated with a hash entry
 * for the given 'key'. If the key already exists, -1 is returned and the optional
 * output parameter may be filled.
 */
static long _dictKeyIndex( dict* d, const void* key, uint64_t hash, dictEntry** existing ) {
  unsigned long idx, table;
  dictEntry*    he;
  if ( existing ) *existing = nullptr;

  // Expand the hash table if needed
  if ( _dictExpandIfNeeded( d ) == DICT_ERR ) return -1;

  for ( table = 0; table <= 1; table++ ) {
    idx = hash & DICTHT_SIZE_MASK( d->ht_size_exp[ table ] );
    // Search if this slot does not already contain the given key
    he  = d->ht_table[ table ][ idx ];
    while ( he ) {
      if ( key == he->key
           || ( d->type->keyCompare && d->type->keyCompare( d, key, he->key ) == 0 ) ) {
        if ( existing ) *existing = he;
        return -1;
      }
      he = he->next;
    }
    if ( !dictIsRehashing( d ) ) break;
  }
  return idx;
}

/**
 * @brief Expand the hash table if needed
 */
static int _dictExpandIfNeeded( dict* d ) {
  // Incremental rehashing already in progress. Return.
  if ( dictIsRehashing( d ) ) return DICT_OK;

  // If the hash table is empty expand it to the initial size.
  if ( DICTHT_SIZE( d->ht_size_exp[ 0 ] ) == 0 ) return dictExpand( d, 4 );

  // If we reached the 1:1 ratio, and we are allowed to resize the hash
  // table (global setting) or we should avoid it but the ratio between
  // elements/buckets is over the "safe" threshold, we resize doubling
  // the number of buckets.
  if ( d->ht_used[ 0 ] >= DICTHT_SIZE( d->ht_size_exp[ 0 ] )
       && ( dict_can_resize
            || d->ht_used[ 0 ] / DICTHT_SIZE( d->ht_size_exp[ 0 ] ) > dict_force_resize_ratio )
       && dictTypeExpandAllowed( d ) ) {
    return dictExpand( d, d->ht_used[ 0 ] * 2 );
  }
  return DICT_OK;
}

/**
 * @brief Check if expand is allowed
 */
static int dictTypeExpandAllowed( dict* d ) {
  if ( d->type && d->type->resizeAllowed ) {
    return d->type->resizeAllowed( DICTHT_SIZE( d->ht_size_exp[ 1 ] ) * sizeof( dictEntry* ),
                                   (double)d->ht_used[ 0 ] / DICTHT_SIZE( d->ht_size_exp[ 0 ] ) );
  }
  return 1;  // Default: allow expansion
}

/**
 * @brief Add an element to the target hash table
 */
int dictAdd( dict* d, void* key, void* val ) {
  dictEntry* entry = dictAddRaw( d, key, nullptr );
  if ( !entry ) return DICT_ERR;
  dictSetVal( d, entry, val );
  return DICT_OK;
}

/**
 * @brief Low level add or find
 */
dictEntry* dictAddRaw( dict* d, void* key, dictEntry** existing ) {
  long       index;
  dictEntry* entry;
  int        htidx;

  if ( dictIsRehashing( d ) ) _dictRehashStep( d );

  // Get the index of the new element, or -1 if the element already exists.
  if ( ( index = _dictKeyIndex( d, key, dictHashKey( d, key ), existing ) ) == -1 ) return nullptr;

  // Allocate the memory and store the new entry.
  // Insert the element in top, with the assumption that in a database
  // system it is more likely that recently added entries are accessed
  // more frequently.
  htidx = dictIsRehashing( d ) ? 1 : 0;
  entry = (dictEntry*)xmalloc( sizeof( *entry ) );
  if ( entry == nullptr ) return nullptr;

  entry->next                   = d->ht_table[ htidx ][ index ];
  d->ht_table[ htidx ][ index ] = entry;
  d->ht_used[ htidx ]++;

  // Set the hash entry fields.
  dictSetKey( d, entry, key );
  return entry;
}

/**
 * @brief Add or replace an element
 */
int dictReplace( dict* d, void* key, void* val ) {
  dictEntry *entry, *existing;

  // Try to add the element. If the key does not exist dictAdd will succeed.
  entry = dictAddRaw( d, key, &existing );
  if ( entry ) {
    dictSetVal( d, entry, val );
    return 1;
  }

  // Set the new value and free the old one. Note that it is important
  // to do that in this order, as the value may just be exactly the same
  // as the previous one. In this context, think to reference counting,
  // you want to increment (set), and then decrement (free), and not the
  // reverse.
  void* oldval = dictGetVal( existing );
  dictSetVal( d, existing, val );
  if ( d->type->valDestructor ) d->type->valDestructor( d, oldval );
  return 0;
}

/**
 * @brief Search and remove an element
 */
static dictEntry* dictGenericDelete( dict* d, const void* key, int nofree ) {
  uint64_t   h, idx;
  dictEntry *he, *prevHe;
  int        table;

  if ( dictSize( d ) == 0 ) return nullptr;  // dict is empty
  if ( dictIsRehashing( d ) ) _dictRehashStep( d );

  h = dictHashKey( d, key );

  for ( table = 0; table <= 1; table++ ) {
    idx    = h & DICTHT_SIZE_MASK( d->ht_size_exp[ table ] );
    he     = d->ht_table[ table ][ idx ];
    prevHe = nullptr;
    while ( he ) {
      if ( key == he->key
           || ( d->type->keyCompare && d->type->keyCompare( d, key, he->key ) == 0 ) ) {
        // Unlink the element from the list
        if ( prevHe )
          prevHe->next = he->next;
        else
          d->ht_table[ table ][ idx ] = he->next;
        if ( !nofree ) {
          dictFreeKey( d, he );
          dictFreeVal( d, he );
          xfree( he );
        }
        d->ht_used[ table ]--;
        return he;
      }
      prevHe = he;
      he     = he->next;
    }
    if ( !dictIsRehashing( d ) ) break;
  }
  return nullptr;  // not found
}

/**
 * @brief Remove an element, returning DICT_OK on success or DICT_ERR if the element was not found.
 */
int dictDelete( dict* d, const void* key ) {
  return dictGenericDelete( d, key, 0 ) ? DICT_OK : DICT_ERR;
}

/**
 * @brief Search for an element
 */
dictEntry* dictFind( dict* d, const void* key ) {
  dictEntry* he;
  uint64_t   h, idx, table;

  if ( dictSize( d ) == 0 ) return nullptr;  // dict is empty
  if ( dictIsRehashing( d ) ) _dictRehashStep( d );

  h = dictHashKey( d, key );
  for ( table = 0; table <= 1; table++ ) {
    idx = h & DICTHT_SIZE_MASK( d->ht_size_exp[ table ] );
    he  = d->ht_table[ table ][ idx ];
    while ( he ) {
      if ( key == he->key
           || ( d->type->keyCompare && d->type->keyCompare( d, key, he->key ) == 0 ) )
        return he;
      he = he->next;
    }
    if ( !dictIsRehashing( d ) ) return nullptr;
  }
  return nullptr;
}

/**
 * @brief Get value for a key
 */
void* dictFetchValue( dict* d, const void* key ) {
  dictEntry* he = dictFind( d, key );
  return he ? dictGetVal( he ) : nullptr;
}

/**
 * @brief Performs one step of incremental rehashing
 */
static void _dictRehashStep( dict* d ) {
  if ( d->pauserehash == 0 ) dictRehash( d, 1 );
}

/**
 * @brief Performs N steps of incremental rehashing
 */
int dictRehash( dict* d, int n ) {
  int empty_visits = n * 10;  // Max number of empty buckets to visit
  if ( !dictIsRehashing( d ) ) return 0;

  while ( n-- && d->ht_used[ 0 ] != 0 ) {
    dictEntry *de, *nextde;

    // Note that rehashidx can't overflow as we are sure there are more
    // elements because ht[0].used != 0
    assert( DICTHT_SIZE( d->ht_size_exp[ 0 ] ) > (unsigned long)d->rehashidx );
    while ( d->ht_table[ 0 ][ d->rehashidx ] == nullptr ) {
      d->rehashidx++;
      if ( --empty_visits == 0 ) return 1;
    }
    de = d->ht_table[ 0 ][ d->rehashidx ];
    // Move all the keys in this bucket from the old to the new hash HT
    while ( de ) {
      uint64_t h;

      nextde                = de->next;
      // Get the index in the new hash table
      h                     = dictHashKey( d, de->key ) & DICTHT_SIZE_MASK( d->ht_size_exp[ 1 ] );
      de->next              = d->ht_table[ 1 ][ h ];
      d->ht_table[ 1 ][ h ] = de;
      d->ht_used[ 0 ]--;
      d->ht_used[ 1 ]++;
      de = nextde;
    }
    d->ht_table[ 0 ][ d->rehashidx ] = nullptr;
    d->rehashidx++;
  }

  // Check if we already rehashed the whole table...
  if ( d->ht_used[ 0 ] == 0 ) {
    xfree( d->ht_table[ 0 ] );
    d->ht_table[ 0 ]    = d->ht_table[ 1 ];
    d->ht_used[ 0 ]     = d->ht_used[ 1 ];
    d->ht_size_exp[ 0 ] = d->ht_size_exp[ 1 ];
    _dictReset( d, 1 );
    d->rehashidx = -1;
    return 0;
  }

  // More to rehash...
  return 1;
}

/**
 * @brief Get dictionary size
 */
size_t dictSize( dict* d ) { return d->ht_used[ 0 ] + d->ht_used[ 1 ]; }

/**
 * @brief Check if dictionary is empty
 */
int dictIsEmpty( dict* d ) { return dictSize( d ) == 0; }

/**
 * @brief Empty a dictionary
 */
void dictEmpty( dict* d ) {
  _dictClear( d, 0, nullptr );
  _dictClear( d, 1, nullptr );
  d->rehashidx   = -1;
  d->pauserehash = 0;
}

/**
 * @brief Clear & Release the hash table
 */
static void _dictClear( dict* d, int htidx, void( callback )( dict* ) ) {
  unsigned long i;

  // Free all the elements
  for ( i = 0; i < DICTHT_SIZE( d->ht_size_exp[ htidx ] ) && d->ht_used[ htidx ] > 0; i++ ) {
    dictEntry *he, *nextHe;

    if ( callback && ( i & 65535 ) == 0 ) callback( d );

    if ( ( he = d->ht_table[ htidx ][ i ] ) == nullptr ) continue;
    while ( he ) {
      nextHe = he->next;
      dictFreeKey( d, he );
      dictFreeVal( d, he );
      xfree( he );
      d->ht_used[ htidx ]--;
      he = nextHe;
    }
  }
  // Free the table and the allocated cache structure
  xfree( d->ht_table[ htidx ] );
  // Re-initialize the table
  _dictReset( d, htidx );
}

/**
 * @brief Clear & Release the whole dictionary
 */
void dictRelease( dict* d ) {
  _dictClear( d, 0, nullptr );
  _dictClear( d, 1, nullptr );
  xfree( d );
}

// Global resize control functions
void dictEnableResize( void ) { dict_can_resize = 1; }

void dictDisableResize( void ) { dict_can_resize = 0; }
