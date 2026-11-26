#include "BitGridCodec.h"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstring>
#include <limits>

namespace BitGridCodec
{
namespace
{
static_assert(CHAR_BIT == 8 && std::numeric_limits<std::size_t>::digits >= 32,
              "BitGrid requires 8-bit bytes and at least 32-bit size_t");
static_assert('+' == 0x2b && '-' == 0x2d && '|' == 0x7c && '.' == 0x2e && '#' == 0x23 && '\n' == 0x0a && '\r' == 0x0d,
              "BitGrid requires ASCII grid characters");

constexpr std::uint32_t signature = 0x48414247; // HABG in ASCII.
constexpr unsigned signatureBits = 32;
constexpr unsigned lengthBits = 16;
constexpr unsigned checksumBits = 16;
constexpr unsigned byteBits = 8;
constexpr std::size_t headerBits = signatureBits + lengthBits + checksumBits;

constexpr std::size_t requiredBits(std::size_t payloadSize)
{
    return headerBits + byteBits * payloadSize;
}

// All callers have at most maxPayloadSize bytes.
constexpr std::size_t dimensionFor(std::size_t payloadSize)
{
    const auto bits = requiredBits(payloadSize);
    std::size_t side = 1;
    while (side * side < bits)
    {
        ++side;
    }
    return side;
}

constexpr std::size_t rowWidth(std::size_t side)
{
    return 2 * side + 2;
}

constexpr std::size_t gridSize(std::size_t side, std::size_t newlineSize)
{
    return (side + 2) * (rowWidth(side) + newlineSize);
}

constexpr auto maxDimension = dimensionFor(maxPayloadSize);
static_assert(maxPayloadSize == std::numeric_limits<std::uint16_t>::max());

std::size_t cellOffset(std::size_t bit, std::size_t side, std::size_t stride)
{
    return (bit / side + 1) * stride + 1 + 2 * (bit % side);
}

unsigned char byteValue(char byte)
{
    unsigned char value;
    std::memcpy(&value, &byte, 1);
    return value;
}

std::uint16_t calculateChecksum(std::string_view payload)
{
    std::uint32_t position = 0;
    std::uint16_t checksum = 0;
    for (char byte : payload)
    {
        // Conversion to uint16_t implements reduction modulo 65536.
        checksum = static_cast<std::uint16_t>(checksum + ++position * byteValue(byte));
    }
    return checksum;
}
} // namespace

std::size_t maxGridSize() noexcept
{
    return gridSize(maxDimension, 2);
}

CodecResult encode(std::string_view payload)
{
    if (payload.size() > maxPayloadSize)
    {
        return CodecError::InputTooLarge;
    }

    const auto side = dimensionFor(payload.size());
    const auto width = rowWidth(side);
    const auto stride = width + 1;
    std::string grid(gridSize(side, 1), '#');
    for (std::size_t row = 0; row < side + 2; ++row)
    {
        const auto start = row * stride;
        if (row == 0 || row == side + 1)
        {
            grid[start] = grid[start + width - 1] = '+';
            std::fill_n(grid.data() + start + 1, width - 2, '-');
        }
        else
        {
            grid[start] = grid[start + width - 1] = '|';
        }
        grid[start + width] = '\n';
    }

    std::size_t bit = 0;
    const auto writeBits = [&](std::uint32_t value, unsigned count) {
        while (count > 0)
        {
            if (((value >> --count) & 1u) == 0)
            {
                const auto offset = cellOffset(bit, side, stride);
                grid[offset] = grid[offset + 1] = '.';
            }
            ++bit;
        }
    };

    writeBits(signature, signatureBits);
    writeBits(static_cast<std::uint16_t>(payload.size()), lengthBits);
    writeBits(calculateChecksum(payload), checksumBits);
    for (char byte : payload)
    {
        writeBits(byteValue(byte), byteBits);
    }
    return grid;
}

CodecResult decode(std::string_view grid)
{
    if (grid.size() > maxGridSize())
    {
        return CodecError::InputTooLarge;
    }

    const auto firstLf = grid.find('\n');
    if (firstLf == std::string_view::npos || firstLf + 1 == grid.size())
    {
        return CodecError::MissingData;
    }

    const std::string_view newline = firstLf > 0 && grid[firstLf - 1] == '\r' ? "\r\n" : "\n";
    const auto width = firstLf + 1 - newline.size();
    if (width < 2 || width > rowWidth(maxDimension) || width % 2 != 0)
    {
        return CodecError::InvalidDimensions;
    }

    const auto side = (width - 2) / 2;
    const auto stride = width + newline.size();
    const auto fullSize = gridSize(side, newline.size());
    const bool terminated = grid.size() == fullSize;
    if (!terminated && grid.size() != fullSize - newline.size())
    {
        return CodecError::InvalidDimensions;
    }

    // Validate rows and cells before reading any header or allocating the payload.
    for (std::size_t row = 0; row < side + 2; ++row)
    {
        const auto start = row * stride;
        const auto line = grid.substr(start, width);
        const bool borderRow = row == 0 || row == side + 1;
        if ((row != side + 1 || terminated) && grid.substr(start + width, newline.size()) != newline)
        {
            return CodecError::InvalidDimensions;
        }
        if (borderRow)
        {
            if (line.front() != '+' || line.back() != '+' ||
                line.substr(1, width - 2).find_first_not_of('-') != std::string_view::npos)
            {
                return CodecError::MalformedBorder;
            }
        }
        else
        {
            if (line.front() != '|' || line.back() != '|')
            {
                return CodecError::MalformedBorder;
            }
            for (std::size_t col = 1; col < width - 1; col += 2)
            {
                if ((line[col] != '.' && line[col] != '#') || line[col] != line[col + 1])
                {
                    return CodecError::InvalidBlock;
                }
            }
        }
    }

    const auto capacity = side * side;
    if (capacity < headerBits)
    {
        return CodecError::IncompleteHeader;
    }

    std::size_t bit = 0;
    const auto readBits = [&](unsigned count) {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < count; ++i, ++bit)
        {
            value = (value << 1) | static_cast<std::uint32_t>(grid[cellOffset(bit, side, stride)] == '#');
        }
        return value;
    };

    if (readBits(signatureBits) != signature)
    {
        return CodecError::InvalidSignature;
    }
    const auto payloadSize = static_cast<std::size_t>(readBits(lengthBits));
    const auto checksum = readBits(checksumBits);
    const auto dataEnd = requiredBits(payloadSize);
    if (dataEnd > capacity)
    {
        return CodecError::MissingData;
    }
    // With sufficient capacity, only the next smaller square needs checking.
    if ((side - 1) * (side - 1) >= dataEnd)
    {
        return CodecError::NoncanonicalDimensions;
    }
    for (auto padding = dataEnd; padding < capacity; ++padding)
    {
        if (grid[cellOffset(padding, side, stride)] != '#')
        {
            return CodecError::InvalidPadding;
        }
    }

    std::string payload(payloadSize, '\0');
    // Write raw byte representations even on platforms with signed char.
    for (auto &byte : payload)
    {
        const auto value = static_cast<unsigned char>(readBits(byteBits));
        std::memcpy(&byte, &value, 1);
    }
    if (calculateChecksum(payload) != checksum)
    {
        return CodecError::ChecksumMismatch;
    }
    return payload;
}
} // namespace BitGridCodec
