#include "BitGridCodec.h"

#include <array>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <utility>

namespace
{
constexpr std::string_view emptyGrid = R"(+----------------+
|..##....##......|
|..##..........##|
|..##........##..|
|..##......######|
|................|
|................|
|................|
|................|
+----------------+
)";

// "Hi": header bytes 48 41 42 47 00 02 01 1A, then 48 69 and one padding bit.
constexpr std::string_view hiGrid = R"(+------------------+
|..##....##........|
|##..........##..##|
|........##....##..|
|....######........|
|..................|
|..##..............|
|..##......####..##|
|....##....##......|
|..####..##....####|
+------------------+
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

void expectError(const CodecResult &result, CodecError error, std::string_view name)
{
    const auto *actual = std::get_if<CodecError>(&result);
    expect(actual && *actual == error, name);
}

void expectBytes(const CodecResult &result, std::string_view bytes, std::string_view name)
{
    const auto *actual = std::get_if<std::string>(&result);
    expect(actual && *actual == bytes, name);
}

void expectRoundTrip(const std::string &input, std::string_view name)
{
    const auto encoded = BitGridCodec::encode(input);
    if (const auto *grid = std::get_if<std::string>(&encoded))
    {
        expectBytes(BitGridCodec::decode(*grid), input, name);
    }
    else
    {
        expect(false, std::string(name) + " (encode failed)");
    }
}

std::size_t cellOffset(std::size_t bit, std::size_t side)
{
    return (bit / side + 1) * (2 * side + 3) + 1 + 2 * (bit % side);
}

void setCell(std::string &grid, std::size_t bit, std::size_t side, char symbol)
{
    grid.replace(cellOffset(bit, side), 2, 2, symbol);
}

std::string filledGrid(std::size_t side, char symbol)
{
    const std::string border = "+" + std::string(2 * side, '-') + "+\n";
    const std::string row = "|" + std::string(2 * side, symbol) + "|\n";
    std::string grid = border;
    for (std::size_t i = 0; i < side; ++i)
    {
        grid += row;
    }
    return grid + border;
}

void testKnownAnswers()
{
    expectBytes(BitGridCodec::encode(""), emptyGrid, "encode empty known answer");
    expectBytes(BitGridCodec::decode(emptyGrid), "", "decode empty known answer");
    expectBytes(BitGridCodec::encode("Hi"), hiGrid, "encode Hi known answer");
    expectBytes(BitGridCodec::decode(hiGrid), "Hi", "decode Hi known answer");
}

void testRoundTrips()
{
    expectRoundTrip("A", "single byte");
    expectRoundTrip("Hello", "text");
    expectRoundTrip("Hello world", "text with spaces");
    expectRoundTrip("one\ntwo", "multiline text");
    expectRoundTrip("one\ntwo\n", "trailing newline");
    expectRoundTrip(std::string{"\0\r\n\xff", 4}, "binary with embedded NUL and CRLF");
    expectRoundTrip(u8"Grüße, 世界 🌍", "UTF-8 bytes");

    std::array<unsigned char, 256> bytes{};
    for (std::size_t i = 0; i < bytes.size(); ++i)
    {
        bytes[i] = static_cast<unsigned char>(i);
    }
    std::string allBytes(bytes.size(), '\0');
    std::memcpy(allBytes.data(), bytes.data(), bytes.size());
    expectRoundTrip(allBytes, "all 256 byte values");

    std::mt19937 random(0x42475244);
    for (int trial = 0; trial < 64; ++trial)
    {
        std::string payload(random() % 4096, '\0');
        for (auto &byte : payload)
        {
            const auto value = static_cast<unsigned char>(random() & 0xffu);
            std::memcpy(&byte, &value, 1);
        }
        expectRoundTrip(payload, "random binary trial " + std::to_string(trial));
    }
}

void testSizeLimits()
{
    constexpr std::pair<std::size_t, std::size_t> cases[]{
        {0, 8},   {1, 9},   {2, 9},       {3, 10},      {4, 10},      {5, 11},      {9, 12},
        {10, 12}, {11, 13}, {65514, 724}, {65515, 725}, {65534, 725}, {65535, 725},
    };

    for (const auto &[length, side] : cases)
    {
        std::string payload(length, '\0');
        for (std::size_t i = 0; i < length; ++i)
        {
            payload[i] = "bitgrid\r\n"[i % 9];
        }
        const auto encoded = BitGridCodec::encode(payload);
        const auto *grid = std::get_if<std::string>(&encoded);
        const auto name = std::to_string(length) + "-byte payload";
        expect(grid && grid->find('\n') == 2 * side + 2 && grid->size() == (side + 2) * (2 * side + 3),
               name + " uses the expected dimensions");
        if (grid)
        {
            expectBytes(BitGridCodec::decode(*grid), payload, name + " round trip");
        }
    }

    expectError(BitGridCodec::encode(std::string(65536, 'x')), CodecError::InputTooLarge, "payload size limit");
    expect(BitGridCodec::maxGridSize() == 1'057'058, "largest grid byte count");
    expectError(BitGridCodec::decode(std::string(2'000'000, '\n')), CodecError::InputTooLarge, "grid size limit");
    expectError(BitGridCodec::decode(std::string(1454, '-') + "\n++\n"), CodecError::InvalidDimensions,
                "grid width limit");
}

void testStructure()
{
    expectError(BitGridCodec::decode(""), CodecError::MissingData, "empty input");
    expectError(BitGridCodec::decode("+--+\n"), CodecError::MissingData, "one row");
    expectError(BitGridCodec::decode(filledGrid(7, '.')), CodecError::IncompleteHeader, "incomplete header");

    std::string grid(hiGrid);
    grid.erase(grid.find('\n') + 2, 1);
    expectError(BitGridCodec::decode(grid), CodecError::InvalidDimensions, "inconsistent width");
    grid = hiGrid;
    grid.erase(grid.rfind('\n', grid.size() - 2) + 1);
    expectError(BitGridCodec::decode(grid), CodecError::InvalidDimensions, "missing bottom row");
    grid = hiGrid;
    grid.insert(1, 1, '-');
    expectError(BitGridCodec::decode(grid), CodecError::InvalidDimensions, "odd border width");

    grid = hiGrid;
    grid.front() = 'x';
    expectError(BitGridCodec::decode(grid), CodecError::MalformedBorder, "malformed corner");
    grid = hiGrid;
    grid[grid.size() - 3] = 'x';
    expectError(BitGridCodec::decode(grid), CodecError::MalformedBorder, "malformed horizontal border");
    grid = hiGrid;
    grid[grid.find('\n') + 1] = 'x';
    expectError(BitGridCodec::decode(grid), CodecError::MalformedBorder, "malformed side border");
    grid = hiGrid;
    grid[cellOffset(0, 9)] = '#';
    expectError(BitGridCodec::decode(grid), CodecError::InvalidBlock, "mismatched block halves");
}

void testMetadataAndPadding()
{
    std::string grid(hiGrid);
    setCell(grid, 0, 9, '#');
    expectError(BitGridCodec::decode(grid), CodecError::InvalidSignature, "altered signature");

    grid = hiGrid;
    setCell(grid, 47, 9, '#'); // Length 2 -> 3. The grid has room for only 2 bytes.
    expectError(BitGridCodec::decode(grid), CodecError::MissingData, "declared length exceeds capacity");
    grid = hiGrid;
    setCell(grid, 64, 9, '#');
    expectError(BitGridCodec::decode(grid), CodecError::ChecksumMismatch, "altered payload");
    grid = hiGrid;
    setCell(grid, 63, 9, '#');
    expectError(BitGridCodec::decode(grid), CodecError::ChecksumMismatch, "altered stored checksum");

    grid = hiGrid;
    setCell(grid, 80, 9, '.');
    expectError(BitGridCodec::decode(grid), CodecError::InvalidPadding, "zero padding is rejected");
    grid = filledGrid(10, '#');
    for (std::size_t bit = 0; bit < 80; ++bit)
    {
        setCell(grid, bit, 10, hiGrid[cellOffset(bit, 9)]);
    }
    expectError(BitGridCodec::decode(grid), CodecError::NoncanonicalDimensions, "larger square is rejected");

    const auto padded = BitGridCodec::encode("elevenbytes");
    if (const auto *paddedGrid = std::get_if<std::string>(&padded))
    {
        grid = *paddedGrid;
        setCell(grid, 152, 13, '.'); // This padding starts before the final interior row.
        expectError(BitGridCodec::decode(grid), CodecError::InvalidPadding, "padding before the final row");
    }
    else
    {
        expect(false, "encode multirow padding fixture");
    }

    // The legacy checksum cannot see a 0x80 change at the one-based position 512.
    std::string zeroPayload(512, '\0');
    const auto zeroEncoded = BitGridCodec::encode(zeroPayload);
    if (const auto *zeroGrid = std::get_if<std::string>(&zeroEncoded))
    {
        grid = *zeroGrid;
        setCell(grid, 64 + 511 * 8, 65, '#');
        const unsigned char highBit = 0x80;
        std::memcpy(&zeroPayload.back(), &highBit, 1);
        expectBytes(BitGridCodec::decode(grid), zeroPayload, "legacy checksum collision remains compatible");
    }
    else
    {
        expect(false, "encode checksum collision fixture");
    }
}

void testLineEndingsAndMutations()
{
    std::string grid(hiGrid);
    grid.pop_back();
    expectBytes(BitGridCodec::decode(grid), "Hi", "omitted final LF");

    std::string crlf;
    for (char ch : hiGrid)
    {
        if (ch == '\n')
        {
            crlf += '\r';
        }
        crlf += ch;
    }
    expectBytes(BitGridCodec::decode(crlf), "Hi", "CRLF grid");
    expectBytes(BitGridCodec::decode(crlf.substr(0, crlf.size() - 2)), "Hi", "omitted final CRLF");
    expectError(BitGridCodec::decode(std::string(hiGrid) + '\n'), CodecError::InvalidDimensions, "extra LF");
    expectError(BitGridCodec::decode(crlf + "\r\n"), CodecError::InvalidDimensions, "extra CRLF");
    expectError(BitGridCodec::decode(crlf.substr(0, crlf.size() - 1)), CodecError::InvalidDimensions, "bare CR");
    crlf.erase(crlf.find('\r', crlf.find('\n') + 1), 1);
    expectError(BitGridCodec::decode(crlf), CodecError::InvalidDimensions, "mixed LF and CRLF");

    for (std::size_t offset = 0; offset < hiGrid.size(); ++offset)
    {
        grid = hiGrid;
        grid[offset] = 'x';
        expect(std::holds_alternative<CodecError>(BitGridCodec::decode(grid)),
               "invalid character at byte " + std::to_string(offset));
    }
    for (std::size_t length = 0; length + 1 < hiGrid.size(); ++length)
    {
        expect(std::holds_alternative<CodecError>(BitGridCodec::decode(hiGrid.substr(0, length))),
               "truncated prefix of " + std::to_string(length) + " bytes");
    }
}
} // namespace

int main()
{
    testKnownAnswers();
    testRoundTrips();
    testSizeLimits();
    testStructure();
    testMetadataAndPadding();
    testLineEndingsAndMutations();
    if (failures != 0)
    {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    return 0;
}
