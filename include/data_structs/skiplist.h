#ifndef REDISCC_DATA_STRUCTURE_SKIP_LIST_H
#define REDISCC_DATA_STRUCTURE_SKIP_LIST_H

#include "sds.h"

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

// zskiplist*     zslCreate( void );
// void           zslFree( zskiplist* zsl );
// zskiplistNode* zslInsert( zskiplist* zsl, double score, sds ele );
// unsigned char* zzlInsert( unsigned char* zl, sds ele, double score );
// int            zslDelete( zskiplist* zsl, double score, sds ele, zskiplistNode** node );
// zskiplistNode* zslNthInRange( zskiplist* zsl, zrangespec* range, long n );
// double         zzlGetScore( unsigned char* sptr );
// void           zzlNext( unsigned char* zl, unsigned char** eptr, unsigned char** sptr );
// void           zzlPrev( unsigned char* zl, unsigned char** eptr, unsigned char** sptr );
// unsigned char* zzlFirstInRange( unsigned char* zl, zrangespec* range );
// unsigned char* zzlLastInRange( unsigned char* zl, zrangespec* range );
// unsigned long  zsetLength( const robj* zobj );
// void           zsetConvert( robj* zobj, int encoding );
// void           zsetConvertToListpackIfNeeded( robj* zobj, size_t maxelelen, size_t totelelen );
// int            zsetScore( robj* zobj, sds member, double* score );
// unsigned long  zslGetRank( zskiplist* zsl, double score, sds o );
// int   zsetAdd( robj* zobj, double score, sds ele, int in_flags, int* out_flags, double* newscore
// ); long  zsetRank( robj* zobj, sds ele, int reverse, double* score ); int   zsetDel( robj* zobj,
// sds ele ); robj* zsetDup( robj* o ); void  genericZpopCommand( client* c,
//                           robj**  keyv,
//                           int     keyc,
//                           int     where,
//                           int     emitkey,
//                           long    count,
//                           int     use_nested_array,
//                           int     reply_nil_when_empty,
//                           int*    deleted );
// sds   lpGetObject( unsigned char* sptr );
// int   zslValueGteMin( double value, zrangespec* spec );
// int   zslValueLteMax( double value, zrangespec* spec );
// void  zslFreeLexRange( zlexrangespec* spec );
// int   zslParseLexRange( robj* min, robj* max, zlexrangespec* spec );
// unsigned char* zzlFirstInLexRange( unsigned char* zl, zlexrangespec* range );
// unsigned char* zzlLastInLexRange( unsigned char* zl, zlexrangespec* range );
// zskiplistNode* zslNthInLexRange( zskiplist* zsl, zlexrangespec* range, long n );
// int            zzlLexValueGteMin( unsigned char* p, zlexrangespec* spec );
// int            zzlLexValueLteMax( unsigned char* p, zlexrangespec* spec );
// int            zslLexValueGteMin( sds value, zlexrangespec* spec );
// int            zslLexValueLteMax( sds value, zlexrangespec* spec );

#endif  //! REDISCC_DATA_STRUCTURE_SKIP_LIST_H
