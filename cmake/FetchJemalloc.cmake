include(ExternalProject)

function(fetch_jemalloc jemlloc_path)
  ExternalProject_Add(jemalloc
    PREFIX ${CMAKE_BINARY_DIR}/deps/jemalloc
    SOURCE_DIR ${jemlloc_path}
    CONFIGURE_COMMAND ${jemlloc_path}/autogen.sh &&
        ${jemlloc_path}/configure --prefix=${CMAKE_BINARY_DIR}/deps/jemalloc --with-jemalloc-prefix=je_
    BUILD_COMMAND make -j4
    INSTALL_COMMAND make install
    BUILD_IN_SOURCE 1
  )
endfunction()
