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

class BitGridCodec
{
  public:
    CodecResult encode(const std::string &inputText);
    CodecResult decode(const std::string &bitGrid);

  private:
    int calculateChecksum(const std::string &text);
    std::string buildHeaderBits(const std::string &inputText);
    std::string buildBitGrid(const std::string &inputText);
    CodecResult extractBinaryBits(const std::string &bitGrid);

    inline static const std::string FORMAT_SIGNATURE = "HABG";

    inline static const char BORDER_CORNER = '+';
    inline static const char BORDER_HORIZONTAL = '-';
    inline static const char BORDER_VERTICAL = '|';
    inline static const char ZERO_BIT_CHAR = '.';
    inline static const char ONE_BIT_CHAR = '#';
    static constexpr std::size_t BIT_BLOCK_LENGTH = 2;

    inline static const std::string ZERO_BIT_BLOCK = std::string(BIT_BLOCK_LENGTH, ZERO_BIT_CHAR);
    inline static const std::string ONE_BIT_BLOCK = std::string(BIT_BLOCK_LENGTH, ONE_BIT_CHAR);

    static constexpr std::size_t PAYLOAD_LENGTH_BITS = 16;
    static constexpr std::size_t CHECKSUM_BITS = 16;
    static constexpr std::size_t SIGNATURE_BITS = 32;
    
    static constexpr std::size_t MAX_PAYLOAD_SIZE = (std::size_t{1} << PAYLOAD_LENGTH_BITS) - 1;
    static constexpr std::size_t CHECKSUM_MODULUS = (std::size_t{1} << CHECKSUM_BITS);
    static constexpr std::size_t HEADER_BITS = SIGNATURE_BITS + PAYLOAD_LENGTH_BITS + CHECKSUM_BITS;
};