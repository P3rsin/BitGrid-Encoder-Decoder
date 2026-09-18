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
    ChecksumMismatch
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
    std::string extractBinaryBits(const std::string &bitGrid);

    inline static const std::string FORMAT_SIGNATURE = "HABG";
    inline static const std::string ZERO_BIT_BLOCK = "▒▒";
    inline static const std::string ONE_BIT_BLOCK = "██";

    static constexpr std::size_t PAYLOAD_LENGTH_BITS = 16;
    static constexpr std::size_t MAX_PAYLOAD_SIZE = (std::size_t{1} << PAYLOAD_LENGTH_BITS) - 1; // bit shift
    static constexpr std::size_t CHECKSUM_BITS = 16;
    static constexpr std::size_t SIGNATURE_BITS = 32;
    static constexpr std::size_t HEADER_BITS = SIGNATURE_BITS + PAYLOAD_LENGTH_BITS + CHECKSUM_BITS;
};