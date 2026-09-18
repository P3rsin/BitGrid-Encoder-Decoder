#include "BitGridCodec.h"
#include <cmath>

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
        checksum = (checksum + (byteValue * (characterIndex + 1))) % CHECMSUM_MODULUS;
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
            bool isBorderRow = rowIndex == 0 || rowIndex == gridDimension - 1;
            bool isBorderColumn = columnIndex == 0 || columnIndex == gridDimension - 1;

            if (isBorderRow && isBorderColumn)
            {
                bitGrid += BORDER_CORNER;
            }
            else if (isBorderRow)
            {
                bitGrid += string(BIT_BLOCK_LENGTH, BORDER_HORIZONTAL);
            }
            else if (isBorderColumn)
            {
                bitGrid += BORDER_VERTICAL;
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

CodecResult BitGridCodec::extractBinaryBits(const string &bitGrid)
{
    size_t rowNum = 0;
    size_t rowLengthCounter = 0;
    size_t expectedGridDimension = bitGrid.find('\n');
    string extractedBits;

    if (expectedGridDimension == string::npos || expectedGridDimension < BIT_BLOCK_LENGTH + 2)
    {
        return CodecError::NotInAGrid;
    }
    else if (bitGrid.size() != (expectedGridDimension * (expectedGridDimension + 1)))
    {
        return CodecError::MissingData;
    }
    else if ((expectedGridDimension - 2) % BIT_BLOCK_LENGTH != 0)
    {
        return CodecError::MissingData;
    }

    for (size_t characterIndex = 0; characterIndex < bitGrid.size(); characterIndex++)
    {
        size_t curRow = characterIndex / (expectedGridDimension + 1);
        size_t curCol = characterIndex % (expectedGridDimension + 1);

        bool isBorderRow = curRow == 0 || curRow == expectedGridDimension - 1;
        bool isBorderColumn = curCol == 0 || curCol == expectedGridDimension - 1;

        if (bitGrid[characterIndex] == '\n')
        {
            if (rowLengthCounter != expectedGridDimension)
            {
                return CodecError::NotInAGrid;
            }
            rowLengthCounter = 0;
            rowNum++;
            continue;
        }
        else if (isBorderRow || isBorderColumn)
        {
            if (isBorderRow && !isBorderColumn && bitGrid[characterIndex] != BORDER_HORIZONTAL)
            {
                return CodecError::MalformedBorder;
            }
            else if (!isBorderRow && isBorderColumn && bitGrid[characterIndex] != BORDER_VERTICAL)
            {
                return CodecError::MalformedBorder;
            }
            else if (isBorderRow && isBorderColumn && bitGrid[characterIndex] != BORDER_CORNER)
            {
                return CodecError::MalformedBorder;
            }
        }
        else if (bitGrid[characterIndex] == ZERO_BIT_CHAR || bitGrid[characterIndex] == ONE_BIT_CHAR)
        {
            string curBlock;
            if (characterIndex + BIT_BLOCK_LENGTH > bitGrid.size())
            {
                return CodecError::MissingData;
            }
            else
            {
                curBlock = bitGrid.substr(characterIndex, BIT_BLOCK_LENGTH);

                if (curBlock == ZERO_BIT_BLOCK)
                {
                    extractedBits += "0";
                    rowLengthCounter += BIT_BLOCK_LENGTH;
                    characterIndex += BIT_BLOCK_LENGTH - 1;
                    continue;
                }
                else if (curBlock == ONE_BIT_BLOCK)
                {
                    extractedBits += "1";
                    rowLengthCounter += BIT_BLOCK_LENGTH;
                    characterIndex += BIT_BLOCK_LENGTH - 1;
                    continue;
                }
                else
                {
                    return CodecError::InvalidBlock;
                }
            }
        }
        else
        {
            return CodecError::InvalidCharacter;
        }

        rowLengthCounter++;
    }

    if (rowNum != expectedGridDimension)
    {
        return CodecError::NotInAGrid;
    }

    return extractedBits;
}

CodecResult BitGridCodec::decode(const string &bitGrid)
{
    CodecResult extractionResult = extractBinaryBits(bitGrid);
    string extractedBits;

    if (holds_alternative<CodecError>(extractionResult))
    {
        return get<CodecError>(extractionResult);
    }
    else
    {
        extractedBits = get<string>(extractionResult);
    }

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