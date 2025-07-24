#include "data_structs/sds.h"
#include "gtest/gtest.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

class SDSTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code if needed
  }

  void TearDown() override {
    // Cleanup code if needed
  }
};

/**
 * Test basic SDS creation and destruction
 */
TEST_F( SDSTest, BasicCreation ) {
  // Test sdsnew
  auto s1_opt = sdsnew( "hello" );
  ASSERT_TRUE( s1_opt.has_value() );
  sds s1 = *s1_opt;
  EXPECT_EQ( sdslen( s1 ), 5 );
  EXPECT_STREQ( s1, "hello" );
  sdsfree( s1 );

  // Test sdsempty
  sds s2 = sdsempty();
  ASSERT_NE( s2, nullptr );
  EXPECT_EQ( sdslen( s2 ), 0 );
  EXPECT_STREQ( s2, "" );
  sdsfree( s2 );

  // Test sdsnewlen
  auto s3_opt = sdsnewlen( "world", 3 );
  ASSERT_TRUE( s3_opt.has_value() );
  sds s3 = *s3_opt;
  EXPECT_EQ( sdslen( s3 ), 3 );
  EXPECT_EQ( strncmp( s3, "wor", 3 ), 0 );
  sdsfree( s3 );
}

/**
 * Test SDS concatenation
 */
TEST_F( SDSTest, Concatenation ) {
  auto s_opt = sdsnew( "hello" );
  ASSERT_TRUE( s_opt.has_value() );
  sds s = *s_opt;

  // Test sdscat with C string
  s = sdscat( s, " world" );
  ASSERT_NE( s, nullptr );
  EXPECT_EQ( sdslen( s ), 11 );
  EXPECT_STREQ( s, "hello world" );

  // Test sdscat with binary data
  s = sdscat( s, "!", 1 );
  ASSERT_NE( s, nullptr );
  EXPECT_EQ( sdslen( s ), 12 );
  EXPECT_STREQ( s, "hello world!" );

  // Test sdscat with another SDS
  auto s2_opt = sdsnew( " test" );
  ASSERT_TRUE( s2_opt.has_value() );
  sds s2 = *s2_opt;

  s = sdscat( s, s2 );
  ASSERT_NE( s, nullptr );
  EXPECT_EQ( sdslen( s ), 17 );
  EXPECT_STREQ( s, "hello world! test" );

  sdsfree( s );
  sdsfree( s2 );
}

/**
 * Test SDS copy operations
 */
TEST_F( SDSTest, Copy ) {
  auto s_opt = sdsnew( "original" );
  ASSERT_TRUE( s_opt.has_value() );
  sds s = *s_opt;

  // Test sdscpy with C string
  s = sdscpy( s, "new content" );
  ASSERT_NE( s, nullptr );
  EXPECT_EQ( sdslen( s ), 11 );
  EXPECT_STREQ( s, "new content" );

  // Test sdscpy with binary data
  s = sdscpy( s, "binary\0data", 11 );
  ASSERT_NE( s, nullptr );
  EXPECT_EQ( sdslen( s ), 11 );
  EXPECT_EQ( memcmp( s, "binary\0data", 11 ), 0 );

  // Test sdscpy with another SDS
  auto s2_opt = sdsnew( "another" );
  ASSERT_TRUE( s2_opt.has_value() );
  sds s2 = *s2_opt;

  s = sdscpy( s, s2 );
  ASSERT_NE( s, nullptr );
  EXPECT_EQ( sdslen( s ), 7 );
  EXPECT_STREQ( s, "another" );

  sdsfree( s );
  sdsfree( s2 );
}

/**
 * Test memory management and growth
 */
TEST_F( SDSTest, MemoryManagement ) {
  auto s_opt = sdsnew( "test" );
  ASSERT_TRUE( s_opt.has_value() );
  sds s = *s_opt;

  size_t initial_alloc = sdsalloc( s );
  EXPECT_GE( initial_alloc, 4 );  // Should have at least the string length

  // Test growth by concatenating
  for ( int i = 0; i < 100; i++ ) {
    s = sdscat( s, "x" );
    ASSERT_NE( s, nullptr );
  }

  EXPECT_EQ( sdslen( s ), 104 );    // 4 + 100
  EXPECT_GT( sdsalloc( s ), 104 );  // Should have allocated more than needed

  sdsfree( s );
}

/**
 * Test edge cases and error conditions
 */
TEST_F( SDSTest, EdgeCases ) {
  // Test with null pointers
  sdsfree( nullptr );  // Should not crash

  auto s_opt = sdsnew( "test" );
  ASSERT_TRUE( s_opt.has_value() );
  sds s = *s_opt;

  // Test concatenating null
  s = sdscat( s, static_cast< const char* >( nullptr ) );
  EXPECT_EQ( sdslen( s ), 4 );
  EXPECT_STREQ( s, "test" );

  // Test copying null
  s = sdscpy( s, static_cast< const char* >( nullptr ) );
  EXPECT_EQ( sdslen( s ), 0 );
  EXPECT_STREQ( s, "" );

  sdsfree( s );
}

/**
 * Test binary data handling
 */
TEST_F( SDSTest, BinaryData ) {
  const char binary_data[] = { 'a', '\0', 'b', '\0', 'c' };

  auto s_opt = sdsnewlen( binary_data, sizeof( binary_data ) );
  ASSERT_TRUE( s_opt.has_value() );
  sds s = *s_opt;

  EXPECT_EQ( sdslen( s ), 5 );
  EXPECT_EQ( memcmp( s, binary_data, 5 ), 0 );

  // Test concatenating binary data
  s = sdscat( s, binary_data, sizeof( binary_data ) );
  ASSERT_NE( s, nullptr );
  EXPECT_EQ( sdslen( s ), 10 );

  sdsfree( s );
}

/**
 * Test large strings
 */
TEST_F( SDSTest, LargeStrings ) {
  // Test string larger than 1MB to trigger different growth strategy
  const size_t large_size = 2 * 1024 * 1024;  // 2MB
  std::string  large_str( large_size, 'x' );

  auto s_opt = sdsnew( large_str.c_str() );
  ASSERT_TRUE( s_opt.has_value() );
  sds s = *s_opt;

  EXPECT_EQ( sdslen( s ), large_size );
  EXPECT_EQ( s[ 0 ], 'x' );
  EXPECT_EQ( s[ large_size - 1 ], 'x' );

  sdsfree( s );
}

GTEST_API_ int main( int argc, char** argv ) {
  testing::InitGoogleTest( &argc, argv );
  return RUN_ALL_TESTS();
}
