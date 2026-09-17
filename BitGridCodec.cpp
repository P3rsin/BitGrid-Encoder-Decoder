#include "BitGridCodec.h"
#include <cmath>
#include <iostream>

using namespace std;

string BitGridCodec::integerToBinaryBits(int value, size_t bitWidth)
{
    string binaryBits(bitWidth, '0');

    for (size_t bitIndex = bitWidth; bitIndex-- > 0;)
    {
        binaryBits[bitIndex] = static_cast<char>('0' + (value % 2));
        // in ASCII: '0' is 48 and '1' is 49 so ^ works out
        value /= 2;
    }

    return binaryBits;
}

string BitGridCodec::characterToBinaryBits(char character)
{
    unsigned char byteValue = static_cast<unsigned char>(character);
    return integerToBinaryBits(byteValue, 8);
}

string BitGridCodec::textToBinaryBits(const string &text)
{
    string binaryBits;
    binaryBits.reserve(text.size() * 8);

    for (char character : text)
    {
        binaryBits += characterToBinaryBits(character);
    }

    return binaryBits;
}

int BitGridCodec::binaryBitsToInteger(const string &binaryBits)
{
    int integerValue = 0;

    for (size_t bitIndex = 0; bitIndex < binaryBits.size(); bitIndex++)
    {
        integerValue *= 2;

        if (binaryBits[bitIndex] == '1')
        {
            integerValue++;
        }
    }

    return integerValue;
}

char BitGridCodec::binaryBitsToCharacter(const string &binaryBits)
{
    return static_cast<char>(binaryBitsToInteger(binaryBits));
}

string BitGridCodec::binaryBitsToText(const string &binaryBits)
{
    string decodedText;

    for (size_t bitIndex = 0; bitIndex < binaryBits.size(); bitIndex += 8)
    {
        decodedText += binaryBitsToCharacter(binaryBits.substr(bitIndex, 8));
    }

    return decodedText;
}

int BitGridCodec::calculateChecksum(const string &text)
{
    int checksum = 0;

    for (size_t characterIndex = 0; characterIndex < text.size(); characterIndex++)
    {
        int byteValue = static_cast<unsigned char>(text[characterIndex]);
        checksum = (checksum + (byteValue * (characterIndex + 1))) % 65536;
        // 16 bits are allocated for the checksum 2^16 = 65536
    }

    return checksum;
}

string BitGridCodec::buildHeaderBits(const string &inputText)
{
    string payloadLengthBits = integerToBinaryBits(inputText.size(), 16);
    string checksumBits = integerToBinaryBits(calculateChecksum(inputText), 16);

    // 32 bits for signature - 16 bits for input size - 16 bits for checksum
    return textToBinaryBits(formatSignature) + payloadLengthBits + checksumBits;
}

string BitGridCodec::buildBitGrid(const string &inputText)
{
    string encodedBits = buildHeaderBits(inputText) + textToBinaryBits(inputText);
    size_t totalBitCount = encodedBits.size();
    int gridDimension = static_cast<int>(ceil(sqrt(totalBitCount))) + 2;

    string bitGrid;
    size_t bitIndex = 0;

    for (int rowIndex = 0; rowIndex < gridDimension; rowIndex++)
    {
        for (int columnIndex = 0; columnIndex < gridDimension; columnIndex++)
        {
            if (rowIndex == 0 && columnIndex == 0)
            {
                bitGrid += "╔";
            }
            else if (rowIndex == 0 && columnIndex == gridDimension - 1)
            {
                bitGrid += "╗";
            }
            else if (rowIndex == gridDimension - 1 && columnIndex == 0)
            {
                bitGrid += "╚";
            }
            else if (rowIndex == gridDimension - 1 && columnIndex == gridDimension - 1)
            {
                bitGrid += "╝";
            }
            else if (rowIndex == 0 || rowIndex == gridDimension - 1)
            {
                bitGrid += "══";
            }
            else if (columnIndex == 0 || columnIndex == gridDimension - 1)
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

string BitGridCodec::encode(const string &inputText)
{
    if (inputText.size() > 65535)
    {
        cout << "ERROR - input must be 65535 characters or fewer\n";
        return "";
    }

    return buildBitGrid(inputText);
}

string BitGridCodec::extractBinaryBits(const string &bitGrid)
{
    string extractedBits;

    for (size_t characterIndex = 0; characterIndex < bitGrid.size(); characterIndex++)
    {
        if (bitGrid.substr(characterIndex, zeroBitBlock.size()) == zeroBitBlock)
        {
            extractedBits += "0";
            characterIndex += zeroBitBlock.size() - 1;
        }
        else if (bitGrid.substr(characterIndex, oneBitBlock.size()) == oneBitBlock)
        {
            extractedBits += "1";
            characterIndex += oneBitBlock.size() - 1;
        }
    }

    return extractedBits;
}

string BitGridCodec::decode(const string &bitGrid)
{
    string extractedBits = extractBinaryBits(bitGrid);

    if (extractedBits.size() < 64)
    {
        return "ERROR - missing or incomplete header";
    }

    string headerBits = extractedBits.substr(0, 64);

    if (binaryBitsToText(headerBits.substr(0, 32)) != formatSignature)
    {
        return "ERROR - invalid BitGrid signature";
    }

    size_t payloadBitCount = static_cast<size_t>(binaryBitsToInteger(headerBits.substr(32, 16))) * 8;

    if (extractedBits.size() < 64 + payloadBitCount)
    {
        return "ERROR - missing data";
    }

    string payloadBits = extractedBits.substr(64, payloadBitCount);
    string decodedText = binaryBitsToText(payloadBits);

    int storedChecksum = binaryBitsToInteger(headerBits.substr(48, 16));
    int calculatedChecksum = calculateChecksum(decodedText);

    if (storedChecksum != calculatedChecksum)
    {
        return "ERROR - checksum mismatch";
    }

    return decodedText;
}