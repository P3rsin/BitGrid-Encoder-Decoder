#pragma once

#include <string>
#include <vector>

class BitGridCodec
{
public:
    BitGridCodec();

    std::string reverseStr(std::string strToReverse);
    std::string charToBinary(char character);
    std::string stringToBinary(std::string str);
    int binaryToInt(std::string binary);
    char binaryToChar(std::string binary);
    std::string intToFixedBinary(int num, int bits);

    std::vector<std::string> populateBinaryVector();
    std::string binaryVectorToStr();
    std::string constructBitGrid();
    std::string decodeBitGrid(std::string qrCode);
    void downloadBitGrid();

    int checksumMaker(std::string inputStr);

    void setInputStr(std::string inputStr);

    void run();

private:
    std::string inputStr;
    std::vector<std::string> binaryVector;
    std::string bitGrid;

    const std::string bitGridSignature = "HABG";
};