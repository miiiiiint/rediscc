cmake_minimum_required(VERSION 3.5.0)
project(rediscc VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/static)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/shared)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(ENABLE_SHARED_LIBRARY "enable shared library" OFF)
option(ENABLE_TESTING_BUILD "enable testing build" ON)
option(ENABLE_BENCHMARK_BUILD "enable benchmark build" OFF)

include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/data_structs
)

set(REDISCC_DATA_STRUCTS_ROOT ${CMAKE_SOURCE_DIR}/include/data_structs)
set(REDISCC_DATA_STRUCTS_SRC ${CMAKE_SOURCE_DIR}/src/data_structs)

add_library(rediscc STATIC
  ${REDISCC_DATA_STRUCTS_SRC}/sds.cc
)
target_link_directories(rediscc PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/data_structs
)

if(ENABLE_TESTING_BUILD)
  enable_testing()
  add_subdirectory(deps/googletest)
  include_directories(
    deps/googletest/googletest/include
    deps/googletest/googlemock/include
  )

  add_subdirectory(tests)
endif(ENABLE_TESTING_BUILD)

