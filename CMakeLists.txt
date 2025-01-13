cmake_minimum_required(VERSION 3.5.0)
project(rediscc VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(CMAKE_CXX_COMPILER_VERSION 16)
set(CMAKE_CXX_COMPILER_ID Clang)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -fsanitize=address -O2 -g")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(ENABLE_SHARED_LIBRARY "enable shared library" OFF)
option(ENABLE_SPDLOG_BUILD  "enable spdlog build" ON)
option(ENABLE_JEMALLOC_BUILD "enable jemalloc build" ON)
option(ENABLE_TESTING_BUILD "enable testing build" ON)
option(ENABLE_BENCHMARK_BUILD "enable benchmark build" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(ENABLE_JEMALLOC_BUILD)
  include(cmake/FetchJemalloc.cmake)
  fetch_jemalloc(${CMAKE_SOURCE_DIR}/deps/jemalloc)
  
  add_compile_definitions(USE_JEMALLOC)
  add_compile_definitions(USE_LIBC=0)

  find_package(jemalloc REQUIRED)
  include_directories(${JEMALLOC_INCLUDE_DIR})
endif()

if(ENABLE_SPDLOG_BUILD)
  add_subdirectory(deps/spdlog)
  include_directories(deps/spdlog/include)

  add_compile_definitions(USE_SPDLOG)
endif()


include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/allocator
  ${CMAKE_SOURCE_DIR}/include/data_structs
)

file(GLOB_RECURSE REDISCC_HEADERS ${CMAKE_SOURCE_DIR}/include/*.h)
file(GLOB_RECURSE REDISCC_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cc)

add_library(rediscc STATIC
  ${REDISCC_HEADERS}
  ${REDISCC_SOURCES}
)

if(ENABLE_JEMALLOC_BUILD)
  target_link_libraries(rediscc PRIVATE ${JEMALLOC_LIBRARY})
endif()

if(ENABLE_SPDLOG_BUILD)
  target_link_libraries(rediscc PRIVATE spdlog::spdlog)
endif()

target_link_directories(rediscc PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/include/allocator
  ${CMAKE_SOURCE_DIR}/include/data_structs
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

