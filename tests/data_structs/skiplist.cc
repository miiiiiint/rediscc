#include "data_structs/skiplist.h"
#include "allocator/xmalloc.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <string>
#include <vector>

class SkipListTest : public ::testing::Test {
protected:
  void SetUp() override {
    zsl = zslCreate();
    ASSERT_NE( zsl, nullptr );
  }

  void TearDown() override {
    if ( zsl ) {
      zslFree( zsl );
    }
  }

  zskiplist* zsl;
};

/**
 * Test basic skip list creation and destruction
 */
TEST_F( SkipListTest, BasicCreation ) {
  EXPECT_EQ( zsl->length, 0 );
  EXPECT_EQ( zsl->level, 1 );
  EXPECT_NE( zsl->header, nullptr );
  EXPECT_EQ( zsl->tail, nullptr );
}

/**
 * Test basic insert and find operations
 */
TEST_F( SkipListTest, BasicInsertFind ) {
  // Insert some elements
  auto ele1_opt = sdsnew( "element1" );
  auto ele2_opt = sdsnew( "element2" );
  auto ele3_opt = sdsnew( "element3" );

  ASSERT_TRUE( ele1_opt.has_value() );
  ASSERT_TRUE( ele2_opt.has_value() );
  ASSERT_TRUE( ele3_opt.has_value() );

  sds ele1 = ele1_opt.value();
  sds ele2 = ele2_opt.value();
  sds ele3 = ele3_opt.value();

  zskiplistNode* node1 = zslInsert( zsl, 1.0, ele1 );
  zskiplistNode* node2 = zslInsert( zsl, 2.0, ele2 );
  zskiplistNode* node3 = zslInsert( zsl, 3.0, ele3 );

  ASSERT_NE( node1, nullptr );
  ASSERT_NE( node2, nullptr );
  ASSERT_NE( node3, nullptr );

  EXPECT_EQ( zsl->length, 3 );

  // Test find operations
  zskiplistNode* found1 = zslFind( zsl, 1.0, ele1 );
  zskiplistNode* found2 = zslFind( zsl, 2.0, ele2 );
  zskiplistNode* found3 = zslFind( zsl, 3.0, ele3 );

  EXPECT_EQ( found1, node1 );
  EXPECT_EQ( found2, node2 );
  EXPECT_EQ( found3, node3 );

  // Test find non-existent element
  auto nonexistent_opt = sdsnew( "nonexistent" );
  ASSERT_TRUE( nonexistent_opt.has_value() );
  sds            nonexistent = nonexistent_opt.value();
  zskiplistNode* notfound    = zslFind( zsl, 4.0, nonexistent );
  EXPECT_EQ( notfound, nullptr );

  // Cleanup
  sdsfree( ele1 );
  sdsfree( ele2 );
  sdsfree( ele3 );
  sdsfree( nonexistent );
}

/**
 * Test element ordering by score
 */
TEST_F( SkipListTest, ScoreOrdering ) {
  // Insert elements in random order
  auto ele1_opt = sdsnew( "first" );
  auto ele2_opt = sdsnew( "second" );
  auto ele3_opt = sdsnew( "third" );

  ASSERT_TRUE( ele1_opt.has_value() );
  ASSERT_TRUE( ele2_opt.has_value() );
  ASSERT_TRUE( ele3_opt.has_value() );

  sds ele1 = ele1_opt.value();
  sds ele2 = ele2_opt.value();
  sds ele3 = ele3_opt.value();

  zslInsert( zsl, 3.0, ele3 );  // Insert highest score first
  zslInsert( zsl, 1.0, ele1 );  // Insert lowest score
  zslInsert( zsl, 2.0, ele2 );  // Insert middle score

  EXPECT_EQ( zsl->length, 3 );

  // Traverse forward and check ordering
  zskiplistNode* current = zsl->header->level[ 0 ].forward;
  ASSERT_NE( current, nullptr );
  EXPECT_EQ( current->score, 1.0 );
  EXPECT_STREQ( current->ele, "first" );

  current = current->level[ 0 ].forward;
  ASSERT_NE( current, nullptr );
  EXPECT_EQ( current->score, 2.0 );
  EXPECT_STREQ( current->ele, "second" );

  current = current->level[ 0 ].forward;
  ASSERT_NE( current, nullptr );
  EXPECT_EQ( current->score, 3.0 );
  EXPECT_STREQ( current->ele, "third" );

  current = current->level[ 0 ].forward;
  EXPECT_EQ( current, nullptr );

  // Cleanup
  sdsfree( ele1 );
  sdsfree( ele2 );
  sdsfree( ele3 );
}

/**
 * Test lexicographic ordering for same scores
 */
TEST_F( SkipListTest, LexicographicOrdering ) {
  // Insert elements with same score but different names
  auto ele1_opt = sdsnew( "apple" );
  auto ele2_opt = sdsnew( "banana" );
  auto ele3_opt = sdsnew( "cherry" );

  ASSERT_TRUE( ele1_opt.has_value() );
  ASSERT_TRUE( ele2_opt.has_value() );
  ASSERT_TRUE( ele3_opt.has_value() );

  sds ele1 = ele1_opt.value();
  sds ele2 = ele2_opt.value();
  sds ele3 = ele3_opt.value();

  // Insert in reverse lexicographic order
  zslInsert( zsl, 1.0, ele3 );  // cherry
  zslInsert( zsl, 1.0, ele1 );  // apple
  zslInsert( zsl, 1.0, ele2 );  // banana

  EXPECT_EQ( zsl->length, 3 );

  // Check lexicographic ordering
  zskiplistNode* current = zsl->header->level[ 0 ].forward;
  ASSERT_NE( current, nullptr );
  EXPECT_STREQ( current->ele, "apple" );

  current = current->level[ 0 ].forward;
  ASSERT_NE( current, nullptr );
  EXPECT_STREQ( current->ele, "banana" );

  current = current->level[ 0 ].forward;
  ASSERT_NE( current, nullptr );
  EXPECT_STREQ( current->ele, "cherry" );

  // Cleanup
  sdsfree( ele1 );
  sdsfree( ele2 );
  sdsfree( ele3 );
}

/**
 * Test delete operations
 */
TEST_F( SkipListTest, Delete ) {
  // Insert some elements
  auto ele1_opt = sdsnew( "element1" );
  auto ele2_opt = sdsnew( "element2" );
  auto ele3_opt = sdsnew( "element3" );

  ASSERT_TRUE( ele1_opt.has_value() );
  ASSERT_TRUE( ele2_opt.has_value() );
  ASSERT_TRUE( ele3_opt.has_value() );

  sds ele1 = ele1_opt.value();
  sds ele2 = ele2_opt.value();
  sds ele3 = ele3_opt.value();

  zslInsert( zsl, 1.0, ele1 );
  zslInsert( zsl, 2.0, ele2 );
  zslInsert( zsl, 3.0, ele3 );

  EXPECT_EQ( zsl->length, 3 );

  // Delete middle element
  int deleted = zslDelete( zsl, 2.0, ele2, nullptr );
  EXPECT_EQ( deleted, 1 );
  EXPECT_EQ( zsl->length, 2 );

  // Verify element is gone
  zskiplistNode* found = zslFind( zsl, 2.0, ele2 );
  EXPECT_EQ( found, nullptr );

  // Verify other elements still exist
  found = zslFind( zsl, 1.0, ele1 );
  EXPECT_NE( found, nullptr );
  found = zslFind( zsl, 3.0, ele3 );
  EXPECT_NE( found, nullptr );

  // Try to delete non-existent element
  auto nonexistent_opt = sdsnew( "nonexistent" );
  ASSERT_TRUE( nonexistent_opt.has_value() );
  sds nonexistent = nonexistent_opt.value();
  deleted         = zslDelete( zsl, 4.0, nonexistent, nullptr );
  EXPECT_EQ( deleted, 0 );
  EXPECT_EQ( zsl->length, 2 );

  // Cleanup
  sdsfree( ele1 );
  sdsfree( ele2 );
  sdsfree( ele3 );
  sdsfree( nonexistent );
}

/**
 * Test rank operations
 */
TEST_F( SkipListTest, Rank ) {
  // Insert elements
  auto ele1_opt = sdsnew( "first" );
  auto ele2_opt = sdsnew( "second" );
  auto ele3_opt = sdsnew( "third" );

  ASSERT_TRUE( ele1_opt.has_value() );
  ASSERT_TRUE( ele2_opt.has_value() );
  ASSERT_TRUE( ele3_opt.has_value() );

  sds ele1 = ele1_opt.value();
  sds ele2 = ele2_opt.value();
  sds ele3 = ele3_opt.value();

  zslInsert( zsl, 1.0, ele1 );
  zslInsert( zsl, 2.0, ele2 );
  zslInsert( zsl, 3.0, ele3 );

  // Test rank retrieval
  unsigned long rank1 = zslGetRank( zsl, 1.0, ele1 );
  unsigned long rank2 = zslGetRank( zsl, 2.0, ele2 );
  unsigned long rank3 = zslGetRank( zsl, 3.0, ele3 );

  EXPECT_EQ( rank1, 1 );
  EXPECT_EQ( rank2, 2 );
  EXPECT_EQ( rank3, 3 );

  // Test element by rank
  zskiplistNode* node1 = zslGetElementByRank( zsl, 1 );
  zskiplistNode* node2 = zslGetElementByRank( zsl, 2 );
  zskiplistNode* node3 = zslGetElementByRank( zsl, 3 );

  ASSERT_NE( node1, nullptr );
  ASSERT_NE( node2, nullptr );
  ASSERT_NE( node3, nullptr );

  EXPECT_EQ( node1->score, 1.0 );
  EXPECT_EQ( node2->score, 2.0 );
  EXPECT_EQ( node3->score, 3.0 );

  // Test out of bounds
  zskiplistNode* outofbounds = zslGetElementByRank( zsl, 4 );
  EXPECT_EQ( outofbounds, nullptr );

  // Cleanup
  sdsfree( ele1 );
  sdsfree( ele2 );
  sdsfree( ele3 );
}

/**
 * Test range query operations
 */
TEST_F( SkipListTest, RangeQuery ) {
  // Insert elements with various scores
  auto ele1_opt = sdsnew( "one" );
  auto ele2_opt = sdsnew( "two" );
  auto ele3_opt = sdsnew( "three" );
  auto ele4_opt = sdsnew( "four" );
  auto ele5_opt = sdsnew( "five" );

  ASSERT_TRUE( ele1_opt.has_value() );
  ASSERT_TRUE( ele2_opt.has_value() );
  ASSERT_TRUE( ele3_opt.has_value() );
  ASSERT_TRUE( ele4_opt.has_value() );
  ASSERT_TRUE( ele5_opt.has_value() );

  sds ele1 = ele1_opt.value();
  sds ele2 = ele2_opt.value();
  sds ele3 = ele3_opt.value();
  sds ele4 = ele4_opt.value();
  sds ele5 = ele5_opt.value();

  zslInsert( zsl, 1.0, ele1 );
  zslInsert( zsl, 2.0, ele2 );
  zslInsert( zsl, 3.0, ele3 );
  zslInsert( zsl, 4.0, ele4 );
  zslInsert( zsl, 5.0, ele5 );

  EXPECT_EQ( zsl->length, 5 );

  // Test range specification
  zrangespec range;
  range.min   = 2.0;
  range.max   = 4.0;
  range.minex = 0;  // inclusive
  range.maxex = 0;  // inclusive

  // Test if range is valid
  EXPECT_EQ( zslIsInRange( zsl, &range ), 1 );

  // Test first in range
  zskiplistNode* first = zslFirstInRange( zsl, &range );
  ASSERT_NE( first, nullptr );
  EXPECT_EQ( first->score, 2.0 );
  EXPECT_STREQ( first->ele, "two" );

  // Test last in range
  zskiplistNode* last = zslLastInRange( zsl, &range );
  ASSERT_NE( last, nullptr );
  EXPECT_EQ( last->score, 4.0 );
  EXPECT_STREQ( last->ele, "four" );

  // Test exclusive range
  range.minex = 1;  // exclusive min
  range.maxex = 1;  // exclusive max

  first = zslFirstInRange( zsl, &range );
  ASSERT_NE( first, nullptr );
  EXPECT_EQ( first->score, 3.0 );
  EXPECT_STREQ( first->ele, "three" );

  last = zslLastInRange( zsl, &range );
  ASSERT_NE( last, nullptr );
  EXPECT_EQ( last->score, 3.0 );
  EXPECT_STREQ( last->ele, "three" );

  // Cleanup
  sdsfree( ele1 );
  sdsfree( ele2 );
  sdsfree( ele3 );
  sdsfree( ele4 );
  sdsfree( ele5 );
}

/**
 * Test large dataset performance
 */
TEST_F( SkipListTest, LargeDataset ) {
  const int          num_elements = 1000;
  std::vector< sds > elements;

  // Insert many elements
  for ( int i = 0; i < num_elements; i++ ) {
    std::string str     = "element" + std::to_string( i );
    auto        ele_opt = sdsnew( str.c_str() );
    ASSERT_TRUE( ele_opt.has_value() );
    sds ele = ele_opt.value();
    elements.push_back( ele );

    zskiplistNode* node = zslInsert( zsl, (double)i, ele );
    ASSERT_NE( node, nullptr );
  }

  EXPECT_EQ( zsl->length, num_elements );

  // Test random access by rank
  for ( int i = 0; i < 10; i++ ) {
    int            rank = rand() % num_elements + 1;  // 1-based
    zskiplistNode* node = zslGetElementByRank( zsl, rank );
    ASSERT_NE( node, nullptr );
    EXPECT_EQ( node->score, rank - 1 );  // 0-based score
  }

  // Test range query on large dataset
  zrangespec range;
  range.min   = 100.0;
  range.max   = 200.0;
  range.minex = 0;
  range.maxex = 0;

  zskiplistNode* first = zslFirstInRange( zsl, &range );
  zskiplistNode* last  = zslLastInRange( zsl, &range );

  ASSERT_NE( first, nullptr );
  ASSERT_NE( last, nullptr );
  EXPECT_EQ( first->score, 100.0 );
  EXPECT_EQ( last->score, 200.0 );

  // Cleanup
  for ( sds ele : elements ) {
    sdsfree( ele );
  }
}

GTEST_API_ int main( int argc, char** argv ) {
  testing::InitGoogleTest( &argc, argv );
  return RUN_ALL_TESTS();
}
