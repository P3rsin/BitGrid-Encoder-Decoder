#pragma once
#include <cstddef>
#include <string>

class BitGridCodec
{
  public:
    std::string encode(const std::string &inputTxt);
    std::string decode(const std::string &bitGrid);

  private:
    std::string intToBinaryString(int num, std::size_t width);
    std::string charToBinaryString(char character);
    std::string textToBinaryString(const std::string &text);

    int binaryStringToInt(const std::string &binary);
    char binaryStringToChar(const std::string &binary);
    std::string binaryStringToText(const std::string &binary);

    int calculateChecksum(const std::string &text);
    std::string buildHeader();
    std::string constructBitGrid();

    std::string bitGridToBinaryStr(const std::string &bitGrid);

    std::string inputStr;
    std::string bitGrid;
    const std::string bitGridSignature = "HABG";
};