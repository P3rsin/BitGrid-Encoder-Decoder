cmake_minimum_required(VERSION 3.16)

project(BitGridEncoderDecoder
    VERSION 1.0
    LANGUAGES CXX
)

add_executable(bitgrid
    main.cpp
    BitGridCodec.cpp
    BitGridCodec.h 
)

target_compile_features(bitgrid PRIVATE cxx_std_17)

target_compile_options(bitgrid PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)