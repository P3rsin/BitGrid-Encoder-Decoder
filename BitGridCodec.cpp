#include "BitGridCodec.h"
#include <cmath>
#include <sstream>
#include <vector>

using namespace std;

namespace
{
constexpr size_t BITS_PER_BYTE = 8;

const string FORMAT_SIGNATURE = "HABG";

constexpr char BORDER_CORNER = '+';
constexpr char BORDER_HORIZONTAL = '-';
constexpr char BORDER_VERTICAL = '|';
constexpr char ZERO_BIT_CHAR = '.';
constexpr char ONE_BIT_CHAR = '#';

constexpr size_t BIT_BLOCK_LENGTH = 2;
const string ZERO_BIT_BLOCK(BIT_BLOCK_LENGTH, ZERO_BIT_CHAR);
const string ONE_BIT_BLOCK(BIT_BLOCK_LENGTH, ONE_BIT_CHAR);

constexpr size_t PAYLOAD_LENGTH_BITS = 16;
constexpr size_t CHECKSUM_BITS = 16;
constexpr size_t SIGNATURE_BITS = 32;

constexpr size_t MAX_PAYLOAD_SIZE = (size_t{1} << PAYLOAD_LENGTH_BITS) - 1;
constexpr size_t CHECKSUM_MODULUS = (size_t{1} << CHECKSUM_BITS);
constexpr size_t HEADER_BITS = SIGNATURE_BITS + PAYLOAD_LENGTH_BITS + CHECKSUM_BITS;

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
    return integerToBinaryBits(static_cast<unsigned char>(character), BITS_PER_BYTE);
}

string textToBinaryBits(const string &text)
{
    string binaryBits;
    binaryBits.reserve(text.size() * BITS_PER_BYTE);

    for (const char character : text)
    {
        binaryBits += characterToBinaryBits(character);
    }

    return binaryBits;
}

int binaryBitsToInteger(const string &binaryBits)
{
    int integerValue = 0;

    for (const char bit : binaryBits)
    {
        integerValue *= 2;

        if (bit == '1')
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
    decodedText.reserve(binaryBits.size() / BITS_PER_BYTE);

    for (size_t bitIndex = 0; bitIndex < binaryBits.size(); bitIndex += BITS_PER_BYTE)
    {
        decodedText += binaryBitsToCharacter(binaryBits.substr(bitIndex, BITS_PER_BYTE));
    }

    return decodedText;
}
} // namespace

namespace BitGridCodec
{
int calculateChecksum(const string &text)
{
    int checksum = 0;

    for (size_t characterIndex = 0; characterIndex < text.size(); characterIndex++)
    {
        const int byteValue = static_cast<unsigned char>(text[characterIndex]);
        checksum = (checksum + (byteValue * (characterIndex + 1))) % CHECKSUM_MODULUS;
    }

    return checksum;
}

string buildHeaderBits(const string &inputText)
{
    const string payloadLengthBits = integerToBinaryBits(inputText.size(), PAYLOAD_LENGTH_BITS);
    const string checksumBits = integerToBinaryBits(calculateChecksum(inputText), CHECKSUM_BITS);
    return textToBinaryBits(FORMAT_SIGNATURE) + payloadLengthBits + checksumBits;
}

string buildBitGrid(const string &inputText)
{
    const string encodedBits = buildHeaderBits(inputText) + textToBinaryBits(inputText);
    const size_t totalBitCount = encodedBits.size();
    const size_t gridDimension = static_cast<size_t>(ceil(sqrt(totalBitCount))) + 2;

    string bitGrid;
    size_t bitIndex = 0;

    for (size_t rowIndex = 0; rowIndex < gridDimension; rowIndex++)
    {
        for (size_t columnIndex = 0; columnIndex < gridDimension; columnIndex++)
        {
            const bool isBorderRow = rowIndex == 0 || rowIndex == gridDimension - 1;
            const bool isBorderColumn = columnIndex == 0 || columnIndex == gridDimension - 1;

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

CodecResult encode(const string &inputText)
{
    if (inputText.size() > MAX_PAYLOAD_SIZE)
    {
        return CodecError::InputTooLarge;
    }

    return buildBitGrid(inputText);
}

CodecResult extractBinaryBits(const string &bitGrid)
{
    vector<string> rows;
    istringstream stream(bitGrid);
    string readRow;

    while (getline(stream, readRow, '\n'))
    {
        rows.push_back(readRow);
    }

    if (rows.size() < 2)
    {
        return CodecError::MissingData;
    }

    const size_t rowLength = rows[0].size();

    if (rowLength < 2)
    {
        return CodecError::MissingData;
    }

    if ((rowLength - 2) % BIT_BLOCK_LENGTH != 0)
    {
        return CodecError::InvalidDimensions;
    }

    const size_t logicalGridDimension = ((rowLength - 2) / BIT_BLOCK_LENGTH) + 2;

    if (rows.size() != logicalGridDimension)
    {
        return CodecError::InvalidDimensions;
    }

    string extractedBits;

    for (size_t rowIndex = 0; rowIndex < rows.size(); rowIndex++)
    {
        if (rows[rowIndex].size() != rowLength)
        {
            return CodecError::InvalidDimensions;
        }

        if (rowIndex == 0 || rowIndex == rows.size() - 1)
        {
            const string expectedBorderRow = BORDER_CORNER + string(rowLength - 2, BORDER_HORIZONTAL) + BORDER_CORNER;

            if (rows[rowIndex] != expectedBorderRow)
            {
                return CodecError::MalformedBorder;
            }
        }
        else
        {
            if (rows[rowIndex][0] != BORDER_VERTICAL || rows[rowIndex][rowLength - 1] != BORDER_VERTICAL)
            {
                return CodecError::MalformedBorder;
            }

            for (size_t colIndex = 1; colIndex < rowLength - 1; colIndex += BIT_BLOCK_LENGTH)
            {
                const string currentBitBlock = rows[rowIndex].substr(colIndex, BIT_BLOCK_LENGTH);

                if (currentBitBlock == ZERO_BIT_BLOCK)
                {
                    extractedBits += '0';
                }
                else if (currentBitBlock == ONE_BIT_BLOCK)
                {
                    extractedBits += '1';
                }
                else
                {
                    return CodecError::InvalidBlock;
                }
            }
        }
    }

    return extractedBits;
}

CodecResult decode(const string &bitGrid)
{
    const CodecResult extractionResult = extractBinaryBits(bitGrid);

    if (holds_alternative<CodecError>(extractionResult))
    {
        return get<CodecError>(extractionResult);
    }

    const string extractedBits = get<string>(extractionResult);

    if (extractedBits.size() < HEADER_BITS)
    {
        return CodecError::IncompleteHeader;
    }

    const string headerBits = extractedBits.substr(0, HEADER_BITS);

    if (binaryBitsToText(headerBits.substr(0, SIGNATURE_BITS)) != FORMAT_SIGNATURE)
    {
        return CodecError::InvalidSignature;
    }

    const size_t payloadLength = binaryBitsToInteger(headerBits.substr(SIGNATURE_BITS, PAYLOAD_LENGTH_BITS));
    const size_t payloadBitCount = payloadLength * BITS_PER_BYTE;

    if (extractedBits.size() < HEADER_BITS + payloadBitCount)
    {
        return CodecError::MissingData;
    }

    const string payloadBits = extractedBits.substr(HEADER_BITS, payloadBitCount);
    const string decodedText = binaryBitsToText(payloadBits);
    const int storedChecksum =
        binaryBitsToInteger(headerBits.substr(SIGNATURE_BITS + PAYLOAD_LENGTH_BITS, CHECKSUM_BITS));
    const int calculatedChecksum = calculateChecksum(decodedText);

    if (storedChecksum != calculatedChecksum)
    {
        return CodecError::ChecksumMismatch;
    }

    return decodedText;
}
} // namespace BitGridCodec