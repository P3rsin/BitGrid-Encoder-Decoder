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

    void setInputStr(const std::string &inputStr);
    void buildBitStream();
    std::string buildHeader();
    int calculateChecksum();
    std::string constructBitGrid();

    std::string decodeBitGrid(const std::string &bitGrid);
    void downloadBitGrid();


    void run();

  private:
    //user input text
    std::string inputStr;

    // vector of binaryString per char of inputStr
    // removing this
    // std::vector<std::string> binaryVector;

    // continuous binary representation of inputStr
    std::string bitStream; 

    // string for the actual grid/image
    std::string bitGrid;

    const std::string bitGridSignature = "HABG";
};