#include "BitGridCodec.h"
#include <cmath>
#include <iostream>

using namespace std;

string BitGridCodec::intToBinaryString(int num, size_t width)
{
    string binaryStr(width, '0');

    for (size_t i = width; i-- > 0;)
    {
        binaryStr[i] = static_cast<char>('0' + (num % 2));
        // in ASCII: '0' is 48 and '1' is 49 so ^ works out
        num /= 2;
    }

    return binaryStr;
}

string BitGridCodec::charToBinaryString(char character)
{
    unsigned char value = static_cast<unsigned char>(character);
    return intToBinaryString(value, 8);
}

string BitGridCodec::textToBinaryString(const string &text)
{
    string binaryStr;
    binaryStr.reserve(text.size() * 8);

    for (char character : text)
    {
        binaryStr += charToBinaryString(character);
    }

    return binaryStr;
}

int BitGridCodec::binaryStringToInt(const string &binaryStr)
{
    int value = 0;

    for (size_t i = 0; i < binaryStr.size(); i++)
    {
        value *= 2;

        if (binaryStr[i] == '1')
        {
            value++;
        }
    }

    return value;
}

char BitGridCodec::binaryStringToChar(const string &binary)
{
    return static_cast<char>(binaryStringToInt(binary));
}

string BitGridCodec::binaryStringToText(const string &binary)
{
    string text;

    for (size_t i = 0; i < binary.size(); i += 8)
    {
        text += binaryStringToChar(binary.substr(i, 8));
    }

    return text;
}

int BitGridCodec::calculateChecksum(const string &text)
{
    int checksum = 0;

    for (size_t i = 0; i < text.size(); i++)
    {
        int charValue = static_cast<unsigned char>(text[i]);
        checksum = (checksum + (charValue * (i + 1))) % 65536;
        // 16 bits are allocated for the checksum 2^16 = 65536
    }

    return checksum;
}

string BitGridCodec::buildHeader(const string &inputTxt)
{
    string inputStrSizeBinary = intToBinaryString(inputTxt.size(), 16);
    string checksumBinary = intToBinaryString(calculateChecksum(inputTxt), 16);

    // 32 bits for signature - 16 bits for input size - 16 bits for checksum
    return textToBinaryString(bitGridSignature) + inputStrSizeBinary + checksumBinary;
}

string BitGridCodec::constructBitGrid(const string &inputTxt)
{
    string encodedBits = buildHeader(inputTxt) + textToBinaryString(inputTxt);
    size_t totalBitCount = encodedBits.size();
    int gridSize = static_cast<int>(ceil(sqrt(totalBitCount))) + 2;

    string bitGrid;
    size_t bitIndex = 0;

    for (int row = 0; row < gridSize; row++)
    {
        for (int col = 0; col < gridSize; col++)
        {
            if (row == 0 && col == 0)
            {
                bitGrid += "╔";
            }
            else if (row == 0 && col == gridSize - 1)
            {
                bitGrid += "╗";
            }
            else if (row == gridSize - 1 && col == 0)
            {
                bitGrid += "╚";
            }
            else if (row == gridSize - 1 && col == gridSize - 1)
            {
                bitGrid += "╝";
            }
            else if (row == 0 || row == gridSize - 1)
            {
                bitGrid += "══";
            }
            else if (col == 0 || col == gridSize - 1)
            {
                bitGrid += "║";
            }
            else
            {
                if (bitIndex < totalBitCount)
                {
                    if (encodedBits[bitIndex] == '1')
                    {
                        bitGrid += "██";
                    }
                    else
                    {
                        bitGrid += "▒▒";
                    }
                }
                else
                {
                    bitGrid += "██";
                }

                bitIndex++;
            }
        }

        bitGrid += "\n";
    }

    return bitGrid;
}

string BitGridCodec::encode(const string &inputTxt)
{
    if (inputTxt.size() > 65535)
    {
        cout << "ERROR - input must be 65535 characters or fewer\n";
        return "";
    }

    return constructBitGrid(inputTxt);
}

string BitGridCodec::bitGridToBinaryStr(const string &bitGrid)
{
    string binaryStr;

    for (size_t i = 0; i < bitGrid.size(); i++)
    {
        if (bitGrid.substr(i, zeroBlock.size()) == zeroBlock)
        {
            binaryStr += "0";
            i += zeroBlock.size() - 1;
        }
        else if (bitGrid.substr(i, oneBlock.size()) == oneBlock)
        {
            binaryStr += "1";
            i += oneBlock.size() - 1;
        }
    }

    return binaryStr;
}

string BitGridCodec::decode(const string &bitGrid)
{
    string binaryStr = bitGridToBinaryStr(bitGrid);

    if (binaryStr.size() < 64)
    {
        return "ERROR - missing or incomplete header";
    }

    string header = binaryStr.substr(0, 64);

    if (binaryStringToText(header.substr(0, 32)) != bitGridSignature)
    {
        return "ERROR - invalid BitGrid signature";
    }

    size_t payloadBitCount = static_cast<size_t>(binaryStringToInt(header.substr(32, 16))) * 8;

    if (binaryStr.size() < 64 + payloadBitCount)
    {
        return "ERROR - missing data";
    }

    string payloadBits = binaryStr.substr(64, payloadBitCount);
    string decodedMessage = binaryStringToText(payloadBits);

    int extractedChecksum = binaryStringToInt(header.substr(48, 16));
    int recalculatedChecksum = calculateChecksum(decodedMessage);

    if (extractedChecksum != recalculatedChecksum)
    {
        return "ERROR - checksum mismatch";
    }

    return decodedMessage;
}