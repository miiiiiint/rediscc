# Logger example executable
add_executable(logger_example
    logger_example.cc
)

target_link_libraries(logger_example PRIVATE
    rediscc
    spdlog::spdlog
)

target_include_directories(logger_example PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Copy config files to build directory
configure_file(
    ${CMAKE_SOURCE_DIR}/config/logger.json
    ${CMAKE_BINARY_DIR}/config/logger.json
    COPYONLY
)

configure_file(
    ${CMAKE_SOURCE_DIR}/config/logger_production.json
    ${CMAKE_BINARY_DIR}/config/logger_production.json
    COPYONLY
)
