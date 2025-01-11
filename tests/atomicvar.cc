#include <atomic>
#include <atomicvar.h>
#include <cstdint>
#include <gtest/gtest.h>

TEST( ATOMIC, increase ) {
  std::atomic< int32_t > var = 0;
  atomic_inc( var, 5 );
  EXPECT_EQ( var, 5 );
}

TEST( ATOMIC, multi_increase ) {
  std::atomic< int > value { 0 };
  constexpr int      num_threads = 10;
  constexpr int      increment   = 100;

  auto worker = [ & ]() {
    for ( int i = 0; i < increment; ++i ) {
      atomic_inc( value, 1 );
    }
  };

  std::vector< std::thread > threads;
  for ( int i = 0; i < num_threads; ++i ) {
    threads.emplace_back( worker );
  }

  for ( auto& thread : threads ) {
    thread.join();
  }

  EXPECT_EQ( value.load(), num_threads * increment );
}

TEST( ATOMIC, get_inc ) {
  std::atomic< int32_t > var = 10;
  int32_t                old_value;
  atomic_get_inc( var, old_value, 5 );
  EXPECT_EQ( old_value, 10 );
  EXPECT_EQ( var, 15 );
}

TEST( ATOMIC, inc_get ) {
  std::atomic< int32_t > var = 10;
  int32_t                new_value;
  atomic_inc_get( var, new_value, 5 );
  EXPECT_EQ( new_value, 15 );
  EXPECT_EQ( var, 15 );
}

TEST( ATOMIC, decrease ) {
  std::atomic< int32_t > var = 10;
  atomic_dec( var, 3 );
  EXPECT_EQ( var, 7 );
}

TEST( ATOMIC, get ) {
  std::atomic< int32_t > var = 20;
  int32_t                dstvar;
  atomic_get( var, dstvar );
  EXPECT_EQ( dstvar, 20 );
}

TEST( ATOMIC, set ) {
  std::atomic< int32_t > var = 0;
  atomic_set( var, 50 );
  EXPECT_EQ( var, 50 );
}

TEST( ATOMIC, get_with_sync ) {
  std::atomic< int32_t > var = 30;
  int32_t                dstvar;
  atomic_get_with_sync( var, dstvar );
  EXPECT_EQ( dstvar, 30 );
}

TEST( ATOMIC, set_with_sync ) {
  std::atomic< int32_t > var = 0;
  atomic_set_with_sync( var, 100 );
  EXPECT_EQ( var, 100 );
}

TEST( ATOMIC, flag_get_set ) {
  std::atomic< int32_t > var = 0;
  int32_t                old_value;
  atomic_flag_get_set( var, old_value );
  EXPECT_EQ( old_value, 0 );  // Initial value
  EXPECT_EQ( var, 1 );        // After setting flag
}

TEST( ATOMIC, multi_thread_get_inc ) {
  std::atomic< int > var { 0 };
  constexpr int      num_threads = 10;
  constexpr int      increment   = 100;

  auto worker = [ & ]() {
    for ( int i = 0; i < increment; ++i ) {
      int old_value;
      atomic_get_inc( var, old_value, 1 );
    }
  };

  std::vector< std::thread > threads;
  for ( int i = 0; i < num_threads; ++i ) {
    threads.emplace_back( worker );
  }

  for ( auto& thread : threads ) {
    thread.join();
  }

  EXPECT_EQ( var.load(), num_threads * increment );
}

TEST( ATOMIC, multi_thread_inc_get ) {
  std::atomic< int > var { 0 };
  constexpr int      num_threads = 10;
  constexpr int      increment   = 100;

  auto worker = [ & ]() {
    for ( int i = 0; i < increment; ++i ) {
      int new_value;
      atomic_inc_get( var, new_value, 1 );
    }
  };

  std::vector< std::thread > threads;
  for ( int i = 0; i < num_threads; ++i ) {
    threads.emplace_back( worker );
  }

  for ( auto& thread : threads ) {
    thread.join();
  }

  EXPECT_EQ( var.load(), num_threads * increment );
}

GTEST_API_ int main( int argc, char* argv[] ) {
  ::testing::InitGoogleTest( &argc, argv );
  return RUN_ALL_TESTS();
}
