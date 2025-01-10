find_path(JEMALLOC_INCLUDE_DIR jemalloc/jemalloc.h
  PATHS ${CMAKE_BINARY_DIR}/deps/jemalloc/include
  PATH_SUFFIXES jemalloc
)

find_library(JEMALLOC_LIBRARY jemalloc
  PATHS ${CMAKE_BINARY_DIR}/deps/jemalloc/lib
)

if (JEMALLOC_INCLUDE_DIR AND JEMALLOC_LIBRARY)
  set(JEMALLOC_FOUND TRUE)
else()
  set(JEMALLOC_FOUND FALSE)
endif()

if(JEMALLOC_FOUND)
  message(STATUS "jemalloc found at ${JEMALLOC_INCLUDE_DIR}")
  message(STATUS "jemalloc libs found at ${JEMALLOC_LIBRARY}")
  set(JEMALLOC_INCLUDE_DIR ${JEMALLOC_INCLUDE_DIR} CACHE PATH "jemalloc include directory")
  set(JEMALLOC_LIBRARY ${JEMALLOC_LIBRARY} CACHE FILEPATH "jemalloc library")
endif()
