/* Hash Tables Implementation.
 *
 * This file implements in-memory hash tables with insert/del/replace/find/
 * get-random-element operations. Hash tables will auto-resize if needed
 * tables of power of two in size are used, collisions are handled by
 * chaining. See the source code for more information... :)
 *
 * Copyright (c) 2006-Present, Redis Ltd.
 * All rights reserved.
 *
 * Licensed under your choice of the Redis Source Available License 2.0
 * (RSALv2) or the Server Side Public License v1 (SSPLv1).
 */

#ifndef REDISCC_DATA_STRUCTURE_DICT_H
#define REDISCC_DATA_STRUCTURE_DICT_H

#include <cstddef>
#include <cstdint>

typedef struct dictEntry dictEntry; /* opaque */
typedef struct dict      dict;

typedef struct dictType {
  uint64_t ( *hashFunction )( const void* key );
  void* ( *keyDup )( dict* d, const void* key );
  void* ( *valDup )( dict* d, const void* obj );
  int ( *keyCompare )( dict* d, const void* key1, const void* key2 );
  void ( *keyDestructor )( dict* d, void* key );
  void ( *valDestructor )( dict* d, void* obj );
  int ( *resizeAllowed )( size_t moreMem, double usedRatio );
  void ( *rehashingStarted )( dict* d );
  void ( *rehashingCompleted )( dict* d );
  size_t ( *dictMetadataBytes )( dict* d );
  void*        userdata;
  unsigned int no_value     : 1;
  unsigned int keys_are_odd : 1;
  int ( *storedKeyCompare )( dict* d, const void* key1, const void* key2 );
  void ( *onDictRelease )( dict* d );
} dictType;

#define DICTHT_SIZE( exp )      ( ( exp ) == -1 ? 0 : (unsigned long)1 << ( exp ) )
#define DICTHT_SIZE_MASK( exp ) ( ( exp ) == -1 ? 0 : ( DICTHT_SIZE( exp ) ) - 1 )

struct dict {
  dictType*     type;
  dictEntry**   ht_table[ 2 ];
  unsigned long ht_used[ 2 ];
  long          rehashidx;
  unsigned      pauserehash     : 15;
  unsigned      useStoredKeyApi : 1;
  signed char   ht_size_exp[ 2 ];
  int16_t       pauseAutoResize;
  void*         metadata[];
};

typedef struct dictIterator {
  dict*              d;
  long               index;
  int                table, safe;
  dictEntry *        entry, *nextEntry;
  unsigned long long fingerprint;
} dictIterator;

typedef struct dictStats {
  int            htidx;
  unsigned long  buckets;
  unsigned long  maxChainLen;
  unsigned long  totalChainLen;
  unsigned long  htSize;
  unsigned long  htUsed;
  unsigned long* clvector;
} dictStats;

typedef void( dictScanFunction )( void* privdata, const dictEntry* de );
typedef void*(dictDefragAllocFunction)( void* ptr );
typedef struct {
  dictDefragAllocFunction* defragAlloc; /* Used for entries etc. */
  dictDefragAllocFunction* defragKey;   /* Defrag-realloc keys (optional) */
  dictDefragAllocFunction* defragVal;   /* Defrag-realloc values (optional) */
} dictDefragFunctions;

#endif  //! REDISCC_DATA_STRUCTURE_DICT_H
