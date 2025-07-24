#include "data_structs/dict.h"
#include "allocator/xmalloc.h"
#include "gtest/gtest.h"
#include <cstring>
#include <string>

// Test dictionary type for string keys and values
static uint64_t stringHashFunction( const void* key ) {
  return dictGenHashFunction( key, strlen( (const char*)key ) );
}

static int stringKeyCompare( dict* d, const void* key1, const void* key2 ) {
  (void)d;  // Suppress unused parameter warning
  return strcmp( (const char*)key1, (const char*)key2 );
}

static void* stringKeyDup( dict* d, const void* key ) {
  (void)d;  // Suppress unused parameter warning
  size_t len = strlen( (const char*)key ) + 1;
  char*  dup = (char*)xmalloc( len );
  if ( dup ) memcpy( dup, key, len );
  return dup;
}

static void* stringValDup( dict* d, const void* val ) {
  (void)d;  // Suppress unused parameter warning
  size_t len = strlen( (const char*)val ) + 1;
  char*  dup = (char*)xmalloc( len );
  if ( dup ) memcpy( dup, val, len );
  return dup;
}

static void stringKeyDestructor( dict* d, void* key ) {
  (void)d;  // Suppress unused parameter warning
  xfree( key );
}

static void stringValDestructor( dict* d, void* val ) {
  (void)d;  // Suppress unused parameter warning
  xfree( val );
}

static dictType stringDictType = {
  stringHashFunction,   // hashFunction
  stringKeyDup,         // keyDup
  stringValDup,         // valDup
  stringKeyCompare,     // keyCompare
  stringKeyDestructor,  // keyDestructor
  stringValDestructor,  // valDestructor
  nullptr,              // resizeAllowed
  nullptr,              // rehashingStarted
  nullptr,              // rehashingCompleted
  nullptr,              // dictMetadataBytes
  nullptr,              // userdata
  0,                    // no_value
  0,                    // keys_are_odd
  nullptr,              // storedKeyCompare
  nullptr               // onDictRelease
};

class DictTest : public ::testing::Test {
protected:
  void SetUp() override {
    d = dictCreate( &stringDictType );
    ASSERT_NE( d, nullptr );
  }

  void TearDown() override {
    if ( d ) {
      dictRelease( d );
    }
  }

  dict* d;
};

/**
 * Test basic dictionary creation and destruction
 */
TEST_F( DictTest, BasicCreation ) {
  EXPECT_TRUE( dictIsEmpty( d ) );
  EXPECT_EQ( dictSize( d ), 0 );
}

/**
 * Test basic add and find operations
 */
TEST_F( DictTest, BasicAddFind ) {
  // Add some key-value pairs
  EXPECT_EQ( dictAdd( d, (void*)"key1", (void*)"value1" ), DICT_OK );
  EXPECT_EQ( dictAdd( d, (void*)"key2", (void*)"value2" ), DICT_OK );
  EXPECT_EQ( dictAdd( d, (void*)"key3", (void*)"value3" ), DICT_OK );

  EXPECT_FALSE( dictIsEmpty( d ) );
  EXPECT_EQ( dictSize( d ), 3 );

  // Find the values
  dictEntry* entry = dictFind( d, "key1" );
  ASSERT_NE( entry, nullptr );
  EXPECT_STREQ( (char*)dictGetVal( entry ), "value1" );

  entry = dictFind( d, "key2" );
  ASSERT_NE( entry, nullptr );
  EXPECT_STREQ( (char*)dictGetVal( entry ), "value2" );

  entry = dictFind( d, "key3" );
  ASSERT_NE( entry, nullptr );
  EXPECT_STREQ( (char*)dictGetVal( entry ), "value3" );

  // Test non-existent key
  entry = dictFind( d, "nonexistent" );
  EXPECT_EQ( entry, nullptr );
}

/**
 * Test dictFetchValue function
 */
TEST_F( DictTest, FetchValue ) {
  dictAdd( d, (void*)"test_key", (void*)"test_value" );

  void* value = dictFetchValue( d, "test_key" );
  ASSERT_NE( value, nullptr );
  EXPECT_STREQ( (char*)value, "test_value" );

  value = dictFetchValue( d, "nonexistent" );
  EXPECT_EQ( value, nullptr );
}

/**
 * Test dictionary replace operation
 */
TEST_F( DictTest, Replace ) {
  // Add initial value
  EXPECT_EQ( dictAdd( d, (void*)"key", (void*)"original_value" ), DICT_OK );
  EXPECT_EQ( dictSize( d ), 1 );

  // Replace with new value
  EXPECT_EQ( dictReplace( d, (void*)"key", (void*)"new_value" ), 0 );  // 0 means replaced existing
  EXPECT_EQ( dictSize( d ), 1 );  // Size should remain the same

  // Verify new value
  void* value = dictFetchValue( d, "key" );
  ASSERT_NE( value, nullptr );
  EXPECT_STREQ( (char*)value, "new_value" );

  // Replace non-existent key (should add new)
  EXPECT_EQ( dictReplace( d, (void*)"new_key", (void*)"new_value2" ), 1 );  // 1 means added new
  EXPECT_EQ( dictSize( d ), 2 );
}

/**
 * Test dictionary delete operation
 */
TEST_F( DictTest, Delete ) {
  // Add some entries
  dictAdd( d, (void*)"key1", (void*)"value1" );
  dictAdd( d, (void*)"key2", (void*)"value2" );
  dictAdd( d, (void*)"key3", (void*)"value3" );
  EXPECT_EQ( dictSize( d ), 3 );

  // Delete existing key
  EXPECT_EQ( dictDelete( d, "key2" ), DICT_OK );
  EXPECT_EQ( dictSize( d ), 2 );

  // Verify key is gone
  EXPECT_EQ( dictFind( d, "key2" ), nullptr );

  // Other keys should still exist
  EXPECT_NE( dictFind( d, "key1" ), nullptr );
  EXPECT_NE( dictFind( d, "key3" ), nullptr );

  // Delete non-existent key
  EXPECT_EQ( dictDelete( d, "nonexistent" ), DICT_ERR );
  EXPECT_EQ( dictSize( d ), 2 );
}

/**
 * Test duplicate key addition
 */
TEST_F( DictTest, DuplicateKeys ) {
  // Add a key
  EXPECT_EQ( dictAdd( d, (void*)"key", (void*)"value1" ), DICT_OK );
  EXPECT_EQ( dictSize( d ), 1 );

  // Try to add the same key again
  EXPECT_EQ( dictAdd( d, (void*)"key", (void*)"value2" ), DICT_ERR );
  EXPECT_EQ( dictSize( d ), 1 );  // Size should remain the same

  // Original value should be preserved
  void* value = dictFetchValue( d, "key" );
  EXPECT_STREQ( (char*)value, "value1" );
}

/**
 * Test dictionary with many entries to trigger rehashing
 */
TEST_F( DictTest, ManyEntries ) {
  const int num_entries = 1000;

  // Add many entries
  for ( int i = 0; i < num_entries; i++ ) {
    std::string key = "key" + std::to_string( i );
    std::string val = "value" + std::to_string( i );
    EXPECT_EQ( dictAdd( d, (void*)key.c_str(), (void*)val.c_str() ), DICT_OK );
  }

  EXPECT_EQ( dictSize( d ), num_entries );

  // Verify all entries can be found
  for ( int i = 0; i < num_entries; i++ ) {
    std::string key   = "key" + std::to_string( i );
    dictEntry*  entry = dictFind( d, key.c_str() );
    EXPECT_NE( entry, nullptr ) << "Failed to find key: " << key;
  }
}

GTEST_API_ int main( int argc, char** argv ) {
  testing::InitGoogleTest( &argc, argv );
  return RUN_ALL_TESTS();
}
