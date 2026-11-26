cmake_minimum_required(VERSION 3.16)

project(BitGridEncoderDecoder VERSION 1.0 LANGUAGES CXX)

include(CTest)
include(GNUInstallDirs)

option(BITGRID_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bitgrid_codec STATIC BitGridCodec.cpp BitGridCodec.h)
target_include_directories(bitgrid_codec PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(bitgrid_codec PUBLIC cxx_std_17)

add_executable(bitgrid main.cpp)
target_link_libraries(bitgrid PRIVATE bitgrid_codec)
target_compile_definitions(bitgrid PRIVATE BITGRID_VERSION="${PROJECT_VERSION}")
install(TARGETS bitgrid RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

set(bitgrid_targets bitgrid_codec bitgrid)
set(bitgrid_executables bitgrid)

if(BUILD_TESTING)
    add_executable(bitgrid_tests tests/BitGridCodecTests.cpp)
    target_link_libraries(bitgrid_tests PRIVATE bitgrid_codec)
    list(APPEND bitgrid_targets bitgrid_tests)
    list(APPEND bitgrid_executables bitgrid_tests)

    set(has_dev_full OFF)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND EXISTS "/dev/full")
        set(has_dev_full ON)
    endif()

    add_test(NAME BitGridCodecTests COMMAND bitgrid_tests)
    add_test(NAME BitGridCliTests
        COMMAND "${CMAKE_COMMAND}"
            "-DBITGRID=$<TARGET_FILE:bitgrid>"
            "-DBITGRID_VERSION=${PROJECT_VERSION}"
            "-DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/cli-tests/$<CONFIG>"
            "-DHAS_DEV_FULL=${has_dev_full}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/BitGridCliTests.cmake"
    )
    set_tests_properties(BitGridCodecTests BitGridCliTests PROPERTIES TIMEOUT 60)
endif()

foreach(target IN LISTS bitgrid_targets)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
    endif()
endforeach()

if(BITGRID_ENABLE_SANITIZERS)
    if(MSVC OR NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        message(FATAL_ERROR "BITGRID_ENABLE_SANITIZERS requires GCC or Clang with GNU-style options")
    endif()
    foreach(target IN LISTS bitgrid_targets)
        target_compile_options(${target} PRIVATE
            -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer
        )
    endforeach()
    foreach(target IN LISTS bitgrid_executables)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endforeach()
endif()
