cmake_minimum_required(VERSION 3.16)

project(BitGridEncoderDecoder
    VERSION 1.0
    LANGUAGES CXX
)

# library
add_library(bitgrid_codec
    BitGridCodec.cpp
    BitGridCodec.h
)

target_include_directories(bitgrid_codec PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(bitgrid_codec PUBLIC cxx_std_17)

target_compile_options(bitgrid_codec PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# cli
add_executable(bitgrid
    main.cpp
)

target_link_libraries(bitgrid PRIVATE bitgrid_codec)

# testing
enable_testing()

add_executable(bitgrid_tests
    tests/BitGridCodecTests.cpp
)

target_link_libraries(bitgrid_tests PRIVATE bitgrid_codec)

add_test(
    NAME BitGridCodecTests
    COMMAND bitgrid_tests
)