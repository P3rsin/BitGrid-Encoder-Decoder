#pragma once

#include <string>
#include <vector>

class BitGridCodec
{
  public:
    BitGridCodec();

    //std::string reverseStr(const std::string &str);
    
    std::string intToFixedBinary(int num, size_t bits);
    std::string charToBinary(char character);
    std::string stringToBinary(const std::string &str);

    int binaryToInt(const std::string &binary);
    char binaryToChar(const std::string &binary);
    
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