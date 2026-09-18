#include "BitGridCodec.h"

#include <cmath>
#include <iostream>

using namespace std;

namespace
{
constexpr size_t BITS_PER_BYTE = 8;

string integerToBinaryBits(int value, size_t bitWidth)
{
    string binaryBits(bitWidth, '0');

    for (size_t bitIndex = bitWidth; bitIndex-- > 0;)
    {
        binaryBits[bitIndex] = static_cast<char>('0' + (value % 2));
        value /= 2;
    }

    return binaryBits;
}

string characterToBinaryBits(char character)
{
    unsigned char byteValue = static_cast<unsigned char>(character);
    return integerToBinaryBits(byteValue, BITS_PER_BYTE);
}

string textToBinaryBits(const string &text)
{
    string binaryBits;
    binaryBits.reserve(text.size() * BITS_PER_BYTE);

    for (char character : text)
    {
        binaryBits += characterToBinaryBits(character);
    }

    return binaryBits;
}

int binaryBitsToInteger(const string &binaryBits)
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

char binaryBitsToCharacter(const string &binaryBits)
{
    return static_cast<char>(binaryBitsToInteger(binaryBits));
}

string binaryBitsToText(const string &binaryBits)
{
    string decodedText;

    for (size_t bitIndex = 0; bitIndex < binaryBits.size(); bitIndex += BITS_PER_BYTE)
    {
        decodedText += binaryBitsToCharacter(binaryBits.substr(bitIndex, BITS_PER_BYTE));
    }

    return decodedText;
}
} // namespace

int BitGridCodec::calculateChecksum(const string &text)
{
    int checksum = 0;

    for (size_t characterIndex = 0; characterIndex < text.size(); characterIndex++)
    {
        int byteValue = static_cast<unsigned char>(text[characterIndex]);
        checksum = (checksum + (byteValue * (characterIndex + 1))) % 65536;
    }

    return checksum;
}

string BitGridCodec::buildHeaderBits(const string &inputText)
{
    string payloadLengthBits = integerToBinaryBits(inputText.size(), PAYLOAD_LENGTH_BITS);
    string checksumBits = integerToBinaryBits(calculateChecksum(inputText), CHECKSUM_BITS);
    return textToBinaryBits(FORMAT_SIGNATURE) + payloadLengthBits + checksumBits;
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
                        bitGrid += ONE_BIT_BLOCK;
                    }
                    else
                    {
                        bitGrid += ZERO_BIT_BLOCK;
                    }
                }
                else
                {
                    bitGrid += ONE_BIT_BLOCK;
                }

                bitIndex++;
            }
        }

        bitGrid += "\n";
    }

    return bitGrid;
}

CodecResult BitGridCodec::encode(const string &inputText)
{
    if (inputText.size() > MAX_PAYLOAD_SIZE)
    {
        return CodecError::InputTooLarge;
    }

    return buildBitGrid(inputText);
}

string BitGridCodec::extractBinaryBits(const string &bitGrid)
{
    string extractedBits;

    for (size_t characterIndex = 0; characterIndex < bitGrid.size(); characterIndex++)
    {
        if (bitGrid.substr(characterIndex, ZERO_BIT_BLOCK.size()) == ZERO_BIT_BLOCK)
        {
            extractedBits += "0";
            characterIndex += ZERO_BIT_BLOCK.size() - 1;
        }
        else if (bitGrid.substr(characterIndex, ONE_BIT_BLOCK.size()) == ONE_BIT_BLOCK)
        {
            extractedBits += "1";
            characterIndex += ONE_BIT_BLOCK.size() - 1;
        }
    }

    return extractedBits;
}

CodecResult BitGridCodec::decode(const string &bitGrid)
{
    string extractedBits = extractBinaryBits(bitGrid);

    if (extractedBits.size() < HEADER_BITS)
    {
        return CodecError::IncompleteHeader;
    }

    string headerBits = extractedBits.substr(0, HEADER_BITS);

    if (binaryBitsToText(headerBits.substr(0, SIGNATURE_BITS)) != FORMAT_SIGNATURE)
    {
        return CodecError::InvalidSignature;
    }

    int payloadLength = binaryBitsToInteger(headerBits.substr(SIGNATURE_BITS, PAYLOAD_LENGTH_BITS));
    size_t payloadBitCount = static_cast<size_t>(payloadLength) * BITS_PER_BYTE;

    if (extractedBits.size() < HEADER_BITS + payloadBitCount)
    {
        return CodecError::MissingData;
    }

    string payloadBits = extractedBits.substr(HEADER_BITS, payloadBitCount);
    string decodedText = binaryBitsToText(payloadBits);
    int storedChecksum = binaryBitsToInteger(headerBits.substr(SIGNATURE_BITS + PAYLOAD_LENGTH_BITS, CHECKSUM_BITS));
    int calculatedChecksum = calculateChecksum(decodedText);

    if (storedChecksum != calculatedChecksum)
    {
        return CodecError::ChecksumMismatch;
    }

    return decodedText;
}