#include "BitGridCodec.h"
#include "BitGridLayout.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

namespace BitGridCodec
{
namespace
{
using Byte = std::uint8_t;
static_assert('?' == 0x3f, "BitGrid requires ASCII erasure markers");

constexpr std::array<Byte, 4> signature{0x42, 0x47, 0x52, 0x31}; // BGR1
constexpr std::size_t headerSize = signature.size() + 2;
constexpr std::size_t crcSize = 4;
constexpr std::size_t recordOverhead = headerSize + crcSize;

constexpr std::size_t dataRows = 8;
constexpr std::size_t bandRows = 12;
constexpr std::size_t maxColumns = 32; // One byte per column.
constexpr std::size_t bitsPerByte = 8;
constexpr std::size_t charsPerByte = 2 * bitsPerByte;
constexpr std::size_t minColumns = (recordOverhead + dataRows - 1) / dataRows;

struct Shape
{
    std::size_t columns;
    std::size_t bands;
};

constexpr Shape shapeFor(std::size_t recordSize)
{
    const auto words = (recordSize + dataRows - 1) / dataRows;
    const auto columns = std::min(words, maxColumns);
    return {columns, (words + columns - 1) / columns};
}

constexpr auto maxBands = shapeFor(maxPayloadSize + recordOverhead).bands;

using Samples = std::array<Byte, dataRows>;
using Codeword = std::array<std::optional<Byte>, bandRows>;
constexpr Samples dataPoints{1, 2, 3, 4, 5, 6, 7, 8};

// Carryless polynomial multiplication modulo x^8 + x^4 + x^3 + x^2 + 1.
constexpr Byte multiply(Byte left, Byte right)
{
    unsigned a = left;
    unsigned b = right;
    unsigned product = 0;
    while (b != 0)
    {
        if ((b & 1u) != 0)
        {
            product ^= a;
        }
        b >>= 1;
        a <<= 1;
        if ((a & 0x100u) != 0)
        {
            a ^= 0x11du;
        }
    }
    return static_cast<Byte>(product);
}

constexpr auto fieldReciprocals()
{
    // Points 1 through 12 can differ only by a nonzero value in 1 through 15.
    std::array<Byte, 16> reciprocals{};
    for (unsigned value = 1; value < reciprocals.size(); ++value)
    {
        Byte reciprocal = 1;
        auto power = static_cast<Byte>(value);
        for (unsigned exponent = 254; exponent != 0; exponent >>= 1)
        {
            if ((exponent & 1u) != 0)
            {
                reciprocal = multiply(reciprocal, power);
            }
            power = multiply(power, power);
        }
        reciprocals[value] = reciprocal;
    }
    return reciprocals;
}

constexpr auto reciprocals = fieldReciprocals();

Samples newtonCoefficients(const Samples &points, Samples values)
{
    // Descend so both values in each difference are still from the previous order.
    for (std::size_t order = 1; order < values.size(); ++order)
    {
        for (std::size_t i = values.size() - 1; i >= order; --i)
        {
            const auto difference = static_cast<unsigned>(points[i] ^ points[i - order]);
            const auto numerator = static_cast<Byte>(values[i] ^ values[i - 1]);
            values[i] = multiply(numerator, reciprocals[difference]);
        }
    }
    return values;
}

Byte evaluate(const Samples &points, const Samples &coefficients, Byte point)
{
    auto value = coefficients.back();
    for (std::size_t i = coefficients.size() - 1; i > 0; --i)
    {
        const auto offset = static_cast<Byte>(point ^ points[i - 1]);
        value = static_cast<Byte>(coefficients[i - 1] ^ multiply(value, offset));
    }
    return value;
}

std::optional<CodecError> completeCodeword(Codeword &word)
{
    Samples points{};
    Samples values{};
    std::size_t count = 0;
    for (std::size_t row = 0; row < word.size() && count < points.size(); ++row)
    {
        if (word[row])
        {
            points[count] = static_cast<Byte>(row + 1);
            values[count] = *word[row];
            ++count;
        }
    }
    if (count < points.size())
    {
        return CodecError::TooManyErasures;
    }

    const auto coefficients = newtonCoefficients(points, values);
    std::size_t sample = 0;
    for (std::size_t row = 0; row < word.size(); ++row)
    {
        const auto point = static_cast<Byte>(row + 1);
        if (sample < points.size() && points[sample] == point)
        {
            ++sample;
            continue;
        }

        const auto expected = evaluate(points, coefficients, point);
        if (word[row] && *word[row] != expected)
        {
            return CodecError::InconsistentSymbols;
        }
        word[row] = expected;
    }
    return std::nullopt;
}

std::uint32_t crc32c(const Byte *bytes, std::size_t count)
{
    std::uint32_t crc = 0xffffffffu;
    for (std::size_t i = 0; i < count; ++i)
    {
        crc ^= bytes[i];
        for (unsigned bit = 0; bit < 8; ++bit)
        {
            const auto polynomial = (crc & 1u) != 0 ? 0x82f63b78u : 0u;
            crc = (crc >> 1) ^ polynomial;
        }
    }
    return crc ^ 0xffffffffu;
}

void writeBigEndian(Byte *bytes, std::size_t count, std::uint32_t value)
{
    while (count > 0)
    {
        bytes[--count] = static_cast<Byte>(value);
        value >>= 8;
    }
}

std::uint32_t readBigEndian(const Byte *bytes, std::size_t count)
{
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < count; ++i)
    {
        value = (value << 8) | bytes[i];
    }
    return value;
}

void writeSymbol(char *cells, Byte value)
{
    for (std::size_t bit = 0; bit < bitsPerByte; ++bit)
    {
        const char cell = (value & (0x80u >> bit)) != 0 ? '#' : '.';
        cells[2 * bit] = cells[2 * bit + 1] = cell;
    }
}

// A valid erasure anywhere in a symbol discards the whole symbol.
bool readSymbol(std::string_view cells, std::optional<Byte> &symbol)
{
    bool erased = false;
    unsigned value = 0;
    for (std::size_t offset = 0; offset < charsPerByte; offset += 2)
    {
        const auto cell = cells[offset];
        if (cell != cells[offset + 1] || (cell != '#' && cell != '.' && cell != '?'))
        {
            return false;
        }
        erased = erased || cell == '?';
        value = (value << 1) | (cell == '#' ? 1u : 0u);
    }
    if (!erased)
    {
        symbol = static_cast<Byte>(value);
    }
    return true;
}

CodecResult unpack(const std::vector<Byte> &data, Shape shape)
{
    // Even the smallest valid geometry has room for the fixed header and CRC.
    if (!std::equal(signature.begin(), signature.end(), data.begin()))
    {
        return CodecError::InvalidSignature;
    }
    const auto size = static_cast<std::size_t>(readBigEndian(data.data() + signature.size(), 2));
    if (size + recordOverhead > data.size())
    {
        return CodecError::MissingData;
    }
    const auto canonical = shapeFor(size + recordOverhead);
    if (shape.columns != canonical.columns || shape.bands != canonical.bands)
    {
        return CodecError::NoncanonicalDimensions;
    }
    const auto *padding = data.data() + size + recordOverhead;
    if (std::any_of(padding, data.data() + data.size(), [](Byte byte) { return byte != 0; }))
    {
        return CodecError::InvalidPadding;
    }
    const auto crcOffset = headerSize + size;
    if (readBigEndian(data.data() + crcOffset, crcSize) != crc32c(data.data(), crcOffset))
    {
        return CodecError::ChecksumMismatch;
    }
    return std::string(reinterpret_cast<const char *>(data.data() + headerSize), size);
}
} // namespace

std::size_t maxProtectedGridSize() noexcept
{
    return detail::gridSize(maxColumns * bitsPerByte, maxBands * bandRows, 2);
}

CodecResult protect(std::string_view payload)
{
    if (payload.size() > maxPayloadSize)
    {
        return CodecError::InputTooLarge;
    }
    const auto shape = shapeFor(payload.size() + recordOverhead);
    const auto bandCapacity = dataRows * shape.columns;
    std::vector<Byte> data(shape.bands * bandCapacity);
    std::copy(signature.begin(), signature.end(), data.begin());
    writeBigEndian(data.data() + signature.size(), 2, static_cast<std::uint32_t>(payload.size()));
    if (!payload.empty())
    {
        std::memcpy(data.data() + headerSize, payload.data(), payload.size());
    }
    const auto crcOffset = headerSize + payload.size();
    writeBigEndian(data.data() + crcOffset, crcSize, crc32c(data.data(), crcOffset));

    const auto width = shape.columns * bitsPerByte;
    const auto stride = detail::rowWidth(width) + 1;
    auto grid = detail::makeGrid(width, shape.bands * bandRows);
    for (std::size_t band = 0; band < shape.bands; ++band)
    {
        for (std::size_t col = 0; col < shape.columns; ++col)
        {
            Samples samples{};
            for (std::size_t row = 0; row < dataRows; ++row)
            {
                samples[row] = data[band * bandCapacity + row * shape.columns + col];
            }
            const auto coefficients = newtonCoefficients(dataPoints, samples);
            for (std::size_t row = 0; row < bandRows; ++row)
            {
                const auto value =
                    row < dataRows ? samples[row] : evaluate(dataPoints, coefficients, static_cast<Byte>(row + 1));
                writeSymbol(grid.data() + (band * bandRows + row + 1) * stride + 1 + col * charsPerByte, value);
            }
        }
    }
    return grid;
}

CodecResult recover(std::string_view grid)
{
    if (grid.size() > maxProtectedGridSize())
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
    if (width < detail::rowWidth(minColumns * bitsPerByte) || width > detail::rowWidth(maxColumns * bitsPerByte) ||
        (width - 2) % charsPerByte != 0)
    {
        return CodecError::InvalidDimensions;
    }
    const auto stride = width + newline.size();
    const auto remainder = grid.size() % stride;
    if (remainder != 0 && remainder != width)
    {
        return CodecError::InvalidDimensions;
    }
    const auto rows = grid.size() / stride + (remainder != 0 ? 1u : 0u);
    if (rows < bandRows + 2 || (rows - 2) % bandRows != 0)
    {
        return CodecError::InvalidDimensions;
    }
    const Shape shape{(width - 2) / charsPerByte, (rows - 2) / bandRows};
    if (shape.bands > maxBands || (shape.bands > 1 && shape.columns != maxColumns))
    {
        return CodecError::InvalidDimensions;
    }

    for (std::size_t row = 0; row < rows; ++row)
    {
        const auto start = row * stride;
        const auto line = grid.substr(start, width);
        if ((row + 1 < rows || remainder == 0) && grid.substr(start + width, newline.size()) != newline)
        {
            return CodecError::InvalidDimensions;
        }
        const bool border = row == 0 || row + 1 == rows;
        if (border ? !detail::horizontalBorder(line) : !detail::verticalBorder(line))
        {
            return CodecError::MalformedBorder;
        }
    }

    const auto bandCapacity = shape.columns * dataRows;
    std::vector<Byte> data(shape.bands * bandCapacity);
    for (std::size_t band = 0; band < shape.bands; ++band)
    {
        for (std::size_t col = 0; col < shape.columns; ++col)
        {
            Codeword word{};
            for (std::size_t row = 0; row < bandRows; ++row)
            {
                const auto start = (band * bandRows + row + 1) * stride + 1 + col * charsPerByte;
                if (!readSymbol(grid.substr(start, charsPerByte), word[row]))
                {
                    return CodecError::InvalidBlock;
                }
            }
            if (const auto error = completeCodeword(word))
            {
                return *error;
            }
            for (std::size_t row = 0; row < dataRows; ++row)
            {
                data[band * bandCapacity + row * shape.columns + col] = *word[row];
            }
        }
    }
    return unpack(data, shape);
}
} // namespace BitGridCodec
