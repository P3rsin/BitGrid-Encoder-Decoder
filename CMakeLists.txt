cmake_minimum_required(VERSION 3.16)

project(BitGridEncoderDecoder VERSION 1.1.0 LANGUAGES CXX)

include(CTest)
include(GNUInstallDirs)

option(BITGRID_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(BITGRID_ENABLE_FUZZING "Build Clang libFuzzer targets for the two grid parsers" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(bitgrid_codec_sources BitGridCodec.cpp BitGridCodec.h BitGridLayout.h BitGridRecovery.cpp)
add_library(bitgrid_codec STATIC ${bitgrid_codec_sources})
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
    add_executable(bitgrid_recovery_tests tests/BitGridRecoveryTests.cpp)
    target_link_libraries(bitgrid_recovery_tests PRIVATE bitgrid_codec)
    list(APPEND bitgrid_targets bitgrid_tests bitgrid_recovery_tests)
    list(APPEND bitgrid_executables bitgrid_tests bitgrid_recovery_tests)

    set(has_dev_full OFF)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND EXISTS "/dev/full")
        set(has_dev_full ON)
    endif()

    add_test(NAME BitGridCodecTests COMMAND bitgrid_tests)
    add_test(NAME BitGridRecoveryTests COMMAND bitgrid_recovery_tests)
    add_test(NAME BitGridCliTests
        COMMAND "${CMAKE_COMMAND}"
            "-DBITGRID=$<TARGET_FILE:bitgrid>"
            "-DBITGRID_VERSION=${PROJECT_VERSION}"
            "-DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/cli-tests/$<CONFIG>"
            "-DHAS_DEV_FULL=${has_dev_full}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/BitGridCliTests.cmake"
    )
    set_tests_properties(BitGridCodecTests BitGridRecoveryTests BitGridCliTests PROPERTIES TIMEOUT 60)
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

if(BITGRID_ENABLE_FUZZING)
    if(MSVC OR NOT CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|AppleClang)$")
        message(FATAL_ERROR "BITGRID_ENABLE_FUZZING requires Clang with GNU-style options and libFuzzer")
    endif()

    # Instrument a separate copy so the normal CLI and tests need no fuzzing runtime.
    set(bitgrid_fuzz_flags -fsanitize=fuzzer-no-link,address,undefined
        -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
    add_library(bitgrid_fuzz_codec STATIC ${bitgrid_codec_sources})
    target_include_directories(bitgrid_fuzz_codec PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(bitgrid_fuzz_codec PRIVATE ${bitgrid_fuzz_flags})

    foreach(parser IN ITEMS decode recover)
        set(target bitgrid_fuzz_${parser})
        add_executable(${target} tests/fuzz/GridParserFuzz.cpp)
        target_link_libraries(${target} PRIVATE bitgrid_fuzz_codec)
        target_compile_options(${target} PRIVATE ${bitgrid_fuzz_flags})
        target_link_options(${target} PRIVATE -fsanitize=fuzzer,address,undefined)
    endforeach()
    target_compile_definitions(bitgrid_fuzz_recover PRIVATE BITGRID_FUZZ_RECOVER)
endif()
