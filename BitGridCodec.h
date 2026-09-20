#pragma once
#include <cstddef>
#include <string>
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
    InvalidBlock
};

using CodecResult = std::variant<std::string, CodecError>;

namespace BitGridCodec
{
CodecResult encode(const std::string &inputText);
CodecResult decode(const std::string &bitGrid);
} // namespace BitGridCodec