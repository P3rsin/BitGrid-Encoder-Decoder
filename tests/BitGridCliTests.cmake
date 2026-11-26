cmake_minimum_required(VERSION 3.16)

file(MAKE_DIRECTORY "${TEST_DIR}/unreadable directory")
set(empty "${TEST_DIR}/empty")
set(output "${TEST_DIR}/stdout")
file(WRITE "${empty}" "")

function(run_bitgrid expected_status expected_diagnostic input output)
    execute_process(
        COMMAND "${BITGRID}" ${ARGN}
        WORKING_DIRECTORY "${TEST_DIR}"
        INPUT_FILE "${input}"
        OUTPUT_FILE "${output}"
        ERROR_VARIABLE diagnostic
        RESULT_VARIABLE status
        ENCODING NONE
        TIMEOUT 15
    )
    if(NOT "${status}" STREQUAL "${expected_status}")
        message(FATAL_ERROR "bitgrid ${ARGN}: expected exit ${expected_status}, got ${status}\n${diagnostic}")
    endif()
    if(NOT diagnostic MATCHES "${expected_diagnostic}")
        message(FATAL_ERROR "bitgrid ${ARGN}: unexpected stderr: '${diagnostic}'")
    endif()
    if(expected_status EQUAL 1 AND NOT output STREQUAL "/dev/full")
        file(SIZE "${output}" size)
        if(NOT size EQUAL 0)
            message(FATAL_ERROR "bitgrid ${ARGN}: unexpected stdout on error")
        endif()
    endif()
endfunction()

function(expect_file_bytes path expected_hex)
    file(READ "${path}" actual_hex HEX)
    if(NOT actual_hex STREQUAL expected_hex)
        message(FATAL_ERROR "${path}: expected bytes ${expected_hex}, got ${actual_hex}")
    endif()
endfunction()

run_bitgrid(0 "^$" "${empty}" "${output}" help)
file(READ "${output}" help)
if(NOT help MATCHES "Usage:" OR NOT help MATCHES "bitgrid decode" OR NOT help MATCHES "bitgrid recover")
    message(FATAL_ERROR "Help does not describe command usage")
endif()
run_bitgrid(0 "^$" "${empty}" "${output}" --help)
file(READ "${output}" option_help)
if(NOT help STREQUAL option_help)
    message(FATAL_ERROR "help and --help differ")
endif()
run_bitgrid(0 "^$" "${empty}" "${output}" --version)
file(READ "${output}" version)
if(NOT version STREQUAL "bitgrid ${BITGRID_VERSION}\n")
    message(FATAL_ERROR "Unexpected version output: '${version}'")
endif()

run_bitgrid(1 "no command" "${empty}" "${output}")
run_bitgrid(1 "unknown command" "${empty}" "${output}" invalid)
run_bitgrid(1 "usage|arguments" "${empty}" "${output}" help extra)
run_bitgrid(1 "usage|arguments" "${empty}" "${output}" --version extra)
run_bitgrid(1 "usage|input" "${empty}" "${output}" encode)
run_bitgrid(1 "file path" "${empty}" "${output}" encode -f)
run_bitgrid(1 "usage|arguments" "${empty}" "${output}" encode -f a b)
run_bitgrid(1 "usage|arguments" "${empty}" "${output}" encode - x)
run_bitgrid(1 "usage|arguments" "${empty}" "${output}" encode x y)
run_bitgrid(1 "usage|input" "${empty}" "${output}" decode)
run_bitgrid(1 "usage|arguments" "${empty}" "${output}" decode - x)
run_bitgrid(1 "no-such-file" "${empty}" "${output}" encode -f no-such-file)
run_bitgrid(1 "no-such-file" "${empty}" "${output}" decode no-such-file)
run_bitgrid(1 "could not (open|read) file" "${empty}" "${output}" encode -f "unreadable directory")
run_bitgrid(1 "could not (open|read) file" "${empty}" "${output}" decode "unreadable directory")
run_bitgrid(1 "missing encoded data" "${empty}" "${output}" decode -)

set(grid "${TEST_DIR}/sample grid.bgrid")
run_bitgrid(0 "^$" "${empty}" "${grid}" encode "Hello world")
run_bitgrid(0 "^$" "${empty}" "${output}" decode "${grid}")
expect_file_bytes("${output}" "48656c6c6f20776f726c64")

run_bitgrid(0 "^$" "${empty}" "${grid}" encode -- -f)
run_bitgrid(0 "^$" "${grid}" "${output}" decode -)
expect_file_bytes("${output}" "2d66")
run_bitgrid(0 "^$" "${empty}" "${TEST_DIR}/-" encode -- -)
run_bitgrid(0 "^$" "${empty}" "${output}" decode -- -)
expect_file_bytes("${output}" "2d")
run_bitgrid(0 "^$" "${empty}" "${grid}" encode --)
run_bitgrid(0 "^$" "${grid}" "${output}" decode -)
expect_file_bytes("${output}" "2d2d")

run_bitgrid(0 "^$" "${empty}" "${grid}" encode -f "${empty}")
run_bitgrid(0 "^$" "${grid}" "${output}" decode -)
expect_file_bytes("${output}" "")
run_bitgrid(0 "^$" "${empty}" "${grid}" encode -)
run_bitgrid(0 "^$" "${grid}" "${output}" decode -)
expect_file_bytes("${output}" "")

# Independently constructed grid for 00 0D 0A 1A 80 FF, with checksum 091A.
# Decode to a file to create the binary input without depending on shell tools.
set(binary_fixture "${TEST_DIR}/binary fixture.bgrid")
set(binary "${TEST_DIR}/binary payload")
set(binary_grid [=[+----------------------+
|..##....##........##..|
|........##..##........|
|##....##......######..|
|......................|
|..####..........##....|
|##......####..##......|
|....................##|
|##..##........##..##..|
|......####..##..##....|
|..........############|
|######################|
+----------------------+
]=])
file(WRITE "${binary_fixture}" "${binary_grid}")
run_bitgrid(0 "^$" "${empty}" "${binary}" decode "${binary_fixture}")
expect_file_bytes("${binary}" "000d0a1a80ff")
set(malformed "${TEST_DIR}/malformed.bgrid")
string(REGEX REPLACE "^\\+" "x" malformed_grid "${binary_grid}")
file(WRITE "${malformed}" "${malformed_grid}")
run_bitgrid(1 "malformed BitGrid border" "${empty}" "${output}" decode "${malformed}")
# The first two bits in the final interior row are payload bits.
string(REPLACE "|######################|" "|####..################|" malformed_grid "${binary_grid}")
file(WRITE "${malformed}" "${malformed_grid}")
run_bitgrid(1 "grid padding" "${empty}" "${output}" decode "${malformed}")
run_bitgrid(0 "^$" "${empty}" "${grid}" encode -f "${binary}")
file(READ "${grid}" encoded_grid)
file(SIZE "${grid}" encoded_size)
string(LENGTH "${binary_grid}" expected_size)
if(NOT encoded_grid STREQUAL binary_grid OR NOT encoded_size EQUAL expected_size)
    message(FATAL_ERROR "Encoding the binary fixture did not reproduce its grid")
endif()

execute_process(
    COMMAND "${BITGRID}" encode -
    COMMAND "${BITGRID}" decode -
    INPUT_FILE "${binary}"
    OUTPUT_FILE "${output}"
    ERROR_VARIABLE diagnostic
    RESULTS_VARIABLE statuses
    ENCODING NONE
    TIMEOUT 15
)
if(NOT statuses STREQUAL "0;0" OR NOT diagnostic STREQUAL "")
    message(FATAL_ERROR "Binary pipe failed (${statuses}): ${diagnostic}")
endif()
expect_file_bytes("${output}" "000d0a1a80ff")

set(protected "${TEST_DIR}/protected grid.bgr")
set(damaged "${TEST_DIR}/damaged grid.bgr")
run_bitgrid(0 "^$" "${empty}" "${protected}" protect -f "${binary}")
run_bitgrid(0 "^$" "${empty}" "${output}" recover "${protected}")
expect_file_bytes("${output}" "000d0a1a80ff")
run_bitgrid(1 "invalid grid dimensions" "${empty}" "${output}" decode "${protected}")
run_bitgrid(1 "invalid grid dimensions" "${empty}" "${output}" recover "${grid}")

# The six-byte payload fits one 12-row band with two byte columns.
file(STRINGS "${protected}" protected_rows)
function(write_damaged_grid count)
    string(REPEAT "?" 32 erased_cells)
    set(content "")
    foreach(index RANGE 0 13)
        if(index GREATER 0 AND index LESS_EQUAL count)
            set(row "|${erased_cells}|")
        else()
            list(GET protected_rows "${index}" row)
        endif()
        string(APPEND content "${row}\n")
    endforeach()
    file(WRITE "${damaged}" "${content}")
endfunction()
write_damaged_grid(4)
run_bitgrid(0 "^$" "${damaged}" "${output}" recover -)
expect_file_bytes("${output}" "000d0a1a80ff")
write_damaged_grid(5)
run_bitgrid(1 "more than four erased bytes" "${empty}" "${output}" recover "${damaged}")

run_bitgrid(0 "^$" "${empty}" "${protected}" protect -- -)
run_bitgrid(0 "^$" "${protected}" "${output}" recover -)
expect_file_bytes("${output}" "2d")

set(large "${TEST_DIR}/large payload")
string(REPEAT "x" 65535 max_payload)
file(WRITE "${large}" "${max_payload}")
run_bitgrid(0 "^$" "${large}" "${grid}" encode -)
set(crlf_grid "${TEST_DIR}/maximum CRLF grid.bgrid")
configure_file("${grid}" "${crlf_grid}" @ONLY NEWLINE_STYLE CRLF)
file(SIZE "${crlf_grid}" crlf_size)
if(NOT crlf_size EQUAL 1057058)
    message(FATAL_ERROR "Unexpected maximum CRLF grid size: ${crlf_size}")
endif()
foreach(max_grid IN ITEMS "${grid}" "${crlf_grid}")
    run_bitgrid(0 "^$" "${max_grid}" "${output}" decode -)
    file(READ "${output}" round_trip)
    if(NOT round_trip STREQUAL max_payload)
        message(FATAL_ERROR "Maximum payload was altered")
    endif()
endforeach()
run_bitgrid(0 "^$" "${large}" "${protected}" protect -)
run_bitgrid(0 "^$" "${protected}" "${output}" recover -)
file(READ "${output}" round_trip)
if(NOT round_trip STREQUAL max_payload)
    message(FATAL_ERROR "Maximum protected payload was altered")
endif()
if(HAS_DEV_FULL)
    run_bitgrid(1 "could not write to standard output" "${large}" "/dev/full" encode -)
endif()
file(APPEND "${large}" "x")
run_bitgrid(1 "exceeds.*65535" "${empty}" "${output}" encode -f "${large}")
run_bitgrid(1 "exceeds.*65535" "${large}" "${output}" encode -)
run_bitgrid(1 "exceeds.*65535" "${large}" "${output}" protect -)
string(REPEAT "x" 1057059 oversized_grid)
file(WRITE "${grid}" "${oversized_grid}")
run_bitgrid(1 "exceeds.*1057058" "${grid}" "${output}" decode -)
run_bitgrid(1 "exceeds.*1057058" "${empty}" "${output}" decode "${grid}")
string(REPEAT "?" 1592377 oversized_protected)
file(WRITE "${protected}" "${oversized_protected}")
run_bitgrid(1 "exceeds.*1592376" "${protected}" "${output}" recover -)

if(HAS_DEV_FULL)
    # A small output can succeed in fwrite and fail on the final flush.
    run_bitgrid(1 "could not write to standard output" "${empty}" "/dev/full" encode "x")
    run_bitgrid(1 "could not write to standard output" "${empty}" "/dev/full" decode "${binary_fixture}")
endif()
