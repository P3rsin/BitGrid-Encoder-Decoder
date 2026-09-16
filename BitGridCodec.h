#pragma once
#include <cstddef>
#include <string>

class BitGridCodec
{
  public:
    void encode(const std::string &inputStr);
    void decode();

    std::string getBitGrid();

    int calculateChecksum(const std::string &text);
    std::string buildHeader();
    std::string constructBitGrid();

    std::string bitGridToBinaryStr(const std::string &bitGrid);
    std::string decodeBitGrid(const std::string &bitGrid);
    //void saveBitGrid();

    //void run();

  private:
    std::string intToBinaryString(int num, std::size_t width);
    std::string charToBinaryString(char character);
    std::string textToBinaryString(const std::string &text);

    int binaryStringToInt(const std::string &binary);
    char binaryStringToChar(const std::string &binary);
    std::string binaryStringToText(const std::string &binary);

    std::string inputStr;
    std::string bitGrid;
    const std::string bitGridSignature = "HABG";
};