#include "data_structs/sds.h"
#include "gtest/gtest.h"
#include <cstdint>
#include <cstring>

TEST( SDS, SimpleConstructor ) {
  sds_8t s1( "simple" );
  EXPECT_EQ( s1.length(), strlen( "simple" ) );
  EXPECT_EQ( s1.get_alloc(), strlen( "simple" ) + 1 );
  EXPECT_EQ( s1.get_avail(), 0 );
}

TEST( SDS, Dumpilicate ) {
  sds_8t s( "simple" );
  EXPECT_EQ( s.length(), strlen( "simple" ) );
  EXPECT_EQ( s.get_alloc(), strlen( "simple" ) + 1 );
  EXPECT_EQ( s.get_avail(), 0 );

  sds_8t ds = s.duplicate();
  ASSERT_NE( s.data(), ds.data() );
  EXPECT_EQ( s.length(), ds.length() );
  EXPECT_EQ( s.get_alloc(), ds.get_alloc() );
  EXPECT_EQ( s.get_avail(), ds.get_avail() );
}

GTEST_API_ int main( int argc, char** argv ) {
  testing::InitGoogleTest( &argc, argv );
  return RUN_ALL_TESTS();
}
