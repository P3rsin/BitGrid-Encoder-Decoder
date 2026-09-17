#pragma once
#include <cstddef>
#include <string>

class BitGridCodec
{
  public:
    std::string encode(const std::string &inputText);
    std::string decode(const std::string &bitGrid);

  private:
    std::string integerToBinaryBits(int value, std::size_t bitWidth);
    std::string characterToBinaryBits(char character);
    std::string textToBinaryBits(const std::string &text);

    int binaryBitsToInteger(const std::string &binaryBits);
    char binaryBitsToCharacter(const std::string &binaryBits);
    std::string binaryBitsToText(const std::string &binaryBits);

    int calculateChecksum(const std::string &text);
    std::string buildHeaderBits(const std::string &inputText);
    std::string buildBitGrid(const std::string &inputText);

    std::string extractBinaryBits(const std::string &bitGrid);

    inline static const std::string FORMAT_SIGNATURE = "HABG";
    inline static const std::string ZERO_BIT_BLOCK = "▒▒";
    inline static const std::string ONE_BIT_BLOCK = "██";

    static constexpr size_t BITS_PER_BYTE = 8;
    static constexpr size_t PAYLOAD_LENGTH_BITS = 16;
    static constexpr size_t CHECKSUM_BITS = 16;
    static constexpr size_t SIGNATURE_BITS = 32;
    static constexpr size_t HEADER_BITS = SIGNATURE_BITS + PAYLOAD_LENGTH_BITS + CHECKSUM_BITS;
};