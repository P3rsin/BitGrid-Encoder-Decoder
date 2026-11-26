#include "BitGridCodec.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace
{
// Reference encoder uses a Vandermonde system solved by Gauss-Jordan elimination.
// Data rows: 4247 5231 0002 4869 EE05 5B24 0000 0000. Parity: 3C9E 7E70 3433 91AC.
constexpr std::string_view knownGrid = R"(+--------------------------------+
|..##........##....##......######|
|..##..##....##......####......##|
|............................##..|
|..##....##........####..##....##|
|######..######............##..##|
|..##..####..####....##....##....|
|................................|
|................................|
|....########....##....########..|
|..############....######........|
|....####..##........####....####|
|##....##......####..##..####....|
+--------------------------------+
)";

int failures = 0;

void expect(bool condition, std::string_view name)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << name << '\n';
        ++failures;
    }
}

bool matches(const CodecResult &result, std::string_view bytes)
{
    const auto *actual = std::get_if<std::string>(&result);
    return actual && *actual == bytes;
}

bool matches(const CodecResult &result, CodecError error)
{
    const auto *actual = std::get_if<CodecError>(&result);
    return actual && *actual == error;
}

std::size_t rowStart(std::size_t row, std::size_t columns)
{
    return (row + 1) * (16 * columns + 3) + 1;
}

void maskRow(std::string &grid, std::size_t row, std::size_t columns)
{
    std::fill_n(grid.data() + rowStart(row, columns), 16 * columns, '?');
}

void flipCell(std::string &grid, std::size_t offset)
{
    grid[offset + 1] = grid[offset] = grid[offset] == '.' ? '#' : '.';
}

void testKnownAnswerAndErasures()
{
    expect(matches(BitGridCodec::protect("Hi"), knownGrid), "independent BGR1 encode vector");
    expect(matches(BitGridCodec::recover(knownGrid), "Hi"), "independent BGR1 decode vector");

    for (unsigned mask = 0; mask < (1u << 12); ++mask)
    {
        std::string damaged(knownGrid);
        unsigned erased = 0;
        for (unsigned row = 0; row < 12; ++row)
        {
            if ((mask & (1u << row)) != 0)
            {
                maskRow(damaged, row, 2);
                ++erased;
            }
        }
        const auto result = BitGridCodec::recover(damaged);
        expect(erased <= 4 ? matches(result, "Hi") : matches(result, CodecError::TooManyErasures),
               "whole-row mask " + std::to_string(mask));
    }

    // A marked bit erases its byte. Independent column masks can exceed four rows in total.
    std::string partial(knownGrid);
    for (std::size_t row = 0; row < 4; ++row)
    {
        partial.replace(rowStart(row, 2), 2, "??");
        partial.replace(rowStart(row + 4, 2) + 16, 2, "??");
    }
    expect(matches(BitGridCodec::recover(partial), "Hi"), "different erasure masks by column");
    partial.replace(rowStart(4, 2), 2, "??");
    expect(matches(BitGridCodec::recover(partial), CodecError::TooManyErasures), "fifth byte in just one column");
}

void testRejectionAndIntegrity()
{
    std::string damaged(knownGrid);
    flipCell(damaged, rowStart(3, 2));
    expect(matches(BitGridCodec::recover(damaged), CodecError::InconsistentSymbols), "unmarked bit substitution");

    for (std::size_t row = 8; row < 12; ++row)
    {
        maskRow(damaged, row, 2);
    }
    expect(matches(BitGridCodec::recover(damaged), CodecError::ChecksumMismatch),
           "CRC detects changed payload when none of the parity symbols survive");

    std::string missingParity(knownGrid);
    for (std::size_t row = 8; row < 12; ++row)
    {
        maskRow(missingParity, row, 2);
    }
    damaged = missingParity;
    flipCell(damaged, rowStart(1, 2) + 16);
    expect(matches(BitGridCodec::recover(damaged), CodecError::InvalidSignature), "unknown recovery version");
    damaged = missingParity;
    flipCell(damaged, rowStart(2, 2));
    expect(matches(BitGridCodec::recover(damaged), CodecError::MissingData), "length exceeds data capacity");
    damaged = missingParity;
    flipCell(damaged, rowStart(7, 2));
    expect(matches(BitGridCodec::recover(damaged), CodecError::InvalidPadding), "nonzero record padding");

    const auto wider = BitGridCodec::protect("1234567");
    if (const auto *grid = std::get_if<std::string>(&wider))
    {
        damaged = *grid;
        for (std::size_t row = 8; row < 12; ++row)
        {
            maskRow(damaged, row, 3);
        }
        flipCell(damaged, rowStart(1, 3) + 2 * 16 + 14); // Declared length changes from 7 to 6.
        expect(matches(BitGridCodec::recover(damaged), CodecError::NoncanonicalDimensions), "unneeded third column");
    }
    else
    {
        expect(false, "encode a three-column grid");
    }

    damaged = knownGrid;
    damaged[rowStart(0, 2)] = '?';
    expect(matches(BitGridCodec::recover(damaged), CodecError::InvalidBlock), "partial erasure marker");
    damaged = knownGrid;
    damaged.front() = '?';
    expect(matches(BitGridCodec::recover(damaged), CodecError::MalformedBorder), "erasures cannot replace framing");
    expect(matches(BitGridCodec::recover(""), CodecError::MissingData), "missing input");
    expect(!std::holds_alternative<std::string>(BitGridCodec::decode(knownGrid)), "legacy decoder rejects BGR1");
    const auto legacy = BitGridCodec::encode("Hi");
    const auto *legacyGrid = std::get_if<std::string>(&legacy);
    expect(legacyGrid && std::holds_alternative<CodecError>(BitGridCodec::recover(*legacyGrid)),
           "recovery decoder rejects legacy");
}

void testFraming()
{
    expect(matches(BitGridCodec::recover(knownGrid.substr(0, knownGrid.size() - 1)), "Hi"), "optional final LF");
    std::string crlf;
    for (char ch : knownGrid)
    {
        if (ch == '\n')
        {
            crlf += '\r';
        }
        crlf += ch;
    }
    expect(matches(BitGridCodec::recover(crlf), "Hi"), "CRLF grid");
    expect(matches(BitGridCodec::recover(std::string_view(crlf).substr(0, crlf.size() - 2)), "Hi"),
           "optional final CRLF");
    expect(matches(BitGridCodec::recover(crlf + "\r\n"), CodecError::InvalidDimensions), "extra CRLF");
    crlf.erase(crlf.find('\r', crlf.find('\n') + 1), 1);
    expect(matches(BitGridCodec::recover(crlf), CodecError::InvalidDimensions), "mixed row separators");

    for (std::size_t offset = 0; offset < knownGrid.size(); ++offset)
    {
        std::string damaged(knownGrid);
        damaged[offset] = 'x';
        expect(std::holds_alternative<CodecError>(BitGridCodec::recover(damaged)),
               "reject invalid character at offset " + std::to_string(offset));
    }
    for (std::size_t size = 0; size + 1 < knownGrid.size(); ++size)
    {
        expect(std::holds_alternative<CodecError>(BitGridCodec::recover(knownGrid.substr(0, size))),
               "reject prefix with size " + std::to_string(size));
    }
}

void testSizeLimitsAndBands()
{
    expect(BitGridCodec::maxProtectedGridSize() == 1'592'376, "maximum CRLF grid size");
    expect(matches(BitGridCodec::protect(std::string(65'536, 'x')), CodecError::InputTooLarge), "protect size limit");
    expect(matches(BitGridCodec::recover(std::string(1'592'377, '?')), CodecError::InputTooLarge),
           "recover size limit");
    struct Case
    {
        std::size_t payloadSize;
        std::size_t columns;
        std::size_t bands;
    };
    constexpr Case cases[]{
        {0, 2, 1},    {6, 2, 1},    {7, 3, 1},         {246, 32, 1},      {247, 32, 2},
        {502, 32, 2}, {503, 32, 3}, {65'526, 32, 256}, {65'527, 32, 257}, {65'535, 32, 257},
    };
    for (const auto &[payloadSize, columns, bands] : cases)
    {
        std::string payload(payloadSize, '\0');
        for (std::size_t i = 0; i < payload.size(); ++i)
        {
            const auto byte = static_cast<unsigned char>(i % 256);
            std::memcpy(&payload[i], &byte, 1);
        }
        const auto encoded = BitGridCodec::protect(payload);
        if (const auto *grid = std::get_if<std::string>(&encoded))
        {
            const auto expectedSize = (12 * bands + 2) * (16 * columns + 3);
            expect(grid->size() == expectedSize, "canonical LF grid size " + std::to_string(payloadSize));
            if (grid->size() != expectedSize)
            {
                continue;
            }
            std::string damaged(*grid);
            for (std::size_t band = 0; band < bands; ++band)
            {
                for (std::size_t row = 0; row < 4; ++row)
                {
                    maskRow(damaged, 12 * band + (band + row) % 12, columns);
                }
            }
            if (payloadSize == BitGridCodec::maxPayloadSize)
            {
                std::string crlf;
                for (char ch : damaged)
                {
                    if (ch == '\n')
                    {
                        crlf += '\r';
                    }
                    crlf += ch;
                }
                damaged = std::move(crlf);
                expect(damaged.size() == BitGridCodec::maxProtectedGridSize(), "maximum CRLF size matches bound");
            }
            expect(matches(BitGridCodec::recover(damaged), payload),
                   "four erasures per band " + std::to_string(payloadSize));
        }
        else
        {
            expect(false, "protect boundary payload of " + std::to_string(payloadSize) + " bytes");
        }
    }
}
} // namespace

int main()
{
    testKnownAnswerAndErasures();
    testRejectionAndIntegrity();
    testFraming();
    testSizeLimitsAndBands();
    return failures == 0 ? 0 : 1;
}
