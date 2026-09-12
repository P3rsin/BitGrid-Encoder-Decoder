#pragma once

#include <string>
#include <vector>

class BitGridCodec
{
  public:
    BitGridCodec();

    //std::string reverseStr(const std::string &str);

    std::string intToBinaryString(int num, size_t width);
    std::string charToBinaryString(char character);
    std::string textToBinaryString(const std::string &text);

    int binaryStringToInt(const std::string &binary);
    char binaryStringToChar(const std::string &binary);
    
    void populateBinaryVector();
    std::string binaryVectorToStr();
    std::string constructBitGrid();
    std::string decodeBitGrid(const std::string &bitGrid);
    void downloadBitGrid();

    int checksumMaker(const std::string &inputStr);

    void setInputStr(const std::string &inputStr);

    void run();

  private:
    std::string inputStr;
    std::vector<std::string> binaryVector;
    std::string bitGrid;

    const std::string bitGridSignature = "HABG";
};