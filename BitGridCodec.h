#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <variant>

enum class CodecError
{
    InputTooLarge,
    IncompleteHeader,
    InvalidSignature,
    MissingData,
    ChecksumMismatch,
    MalformedBorder,
    InvalidDimensions,
    InvalidBlock,
    NoncanonicalDimensions,
    InvalidPadding,
    TooManyErasures,
    InconsistentSymbols
};

using CodecResult = std::variant<std::string, CodecError>;

namespace BitGridCodec
{
inline constexpr std::size_t maxPayloadSize = 65'535;

// The largest accepted grid in bytes, including CRLF on every row.
[[nodiscard]] std::size_t maxGridSize() noexcept;

[[nodiscard]] CodecResult encode(std::string_view payload);
[[nodiscard]] CodecResult decode(std::string_view grid);

// BGR1 uses a separate wire format. Each ?? cell marks its byte as erased.
[[nodiscard]] std::size_t maxProtectedGridSize() noexcept;
[[nodiscard]] CodecResult protect(std::string_view payload);
[[nodiscard]] CodecResult recover(std::string_view grid);
} // namespace BitGridCodec
