#pragma once

#include <string>
#include <vector>

class BitGridCodec
{
  public:
    std::string intToBinaryString(int num, size_t width);
    std::string charToBinaryString(char character);
    std::string textToBinaryString(const std::string &text);

    int binaryStringToInt(const std::string &binary);
    char binaryStringToChar(const std::string &binary);
    std::string binaryStringToText(const std::string &binary);

    void setInputStr(const std::string &inputStr);
    std::string buildHeader();
    int calculateChecksum();
    std::string constructBitGrid();

    std::string bitGridToBinaryStr(const std::string &bitGrid);
    std::string decodeBitGrid(const std::string &bitGrid);
    void saveBitGrid();

    void run();

  private:
    std::string inputStr;    // user input text
    std::string bitStream;   // continuous binary representation of inputStr
    std::string bitGrid;     // string for the actual grid/image

    const std::string bitGridSignature = "HABG";
};