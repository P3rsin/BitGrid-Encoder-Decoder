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

string BitGridCodec::textToBinaryString(const string &str)
{
    string binaryStr;
    binaryStr.reserve(str.size() * 8);

    for (char character : str)
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

string BitGridCodec::buildHeader()
{
    string inputStrSizeBinary = intToBinaryString(inputStr.size(), 16);
    string checksumBinary = intToBinaryString(calculateChecksum(inputStr), 16);

    // 32 bits for signature - 16 bits for input size - 16 bits for checksum
    return textToBinaryString(bitGridSignature) + inputStrSizeBinary + checksumBinary;
}

string BitGridCodec::constructBitGrid()
{
    string encodedBits = buildHeader() + textToBinaryString(inputStr);
    size_t totalBitCount = encodedBits.size();
    int gridSize = static_cast<int>(ceil(sqrt(totalBitCount))) + 2;

    bitGrid.clear();
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

    this->inputStr = inputTxt;
    bitGrid = constructBitGrid();
    return bitGrid;
}

string BitGridCodec::bitGridToBinaryStr(const string &bitGrid)
{
    string binaryStr;
    string zeroBlock = "▒▒";
    string oneBlock = "██";

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

// void BitGridCodec::run()
// {
//     string usrInput;

//     const string title = "-------------------------\n"
//                          " BitGrid Encoder/Decoder \n";

//     const string menuPrompt = "-------------------------\n"
//                               "[1] Generate a bit grid\n"
//                               "[2] View my bit grid\n"
//                               "[3] Save my bit grid\n"
//                               "[4] Decode a bit grid\n"
//                               "[5] About the project\n"
//                               "[6] Exit\n"
//                               "Choice: ";

//     cout << title << menuPrompt;
//     cin >> usrInput;

//     while (usrInput != "6")
//     {
//         cout << endl;

//         if (usrInput == "1")
//         {
//             cout << "Please enter a string to convert\nMy Input: ";

//             cin.ignore();
//             getline(cin, inputStr);
//             encode(inputStr);

//             cout << "\nYour bit grid is now:\n" << bitGrid;

//             string gridBinaryStr = bitGridToBinaryStr(bitGrid);
//             int extractedChecksum = binaryStringToInt(gridBinaryStr.substr(48, 16));
//             int recalculatedChecksum = calculateChecksum(decodeBitGrid(bitGrid));

//             cout << "The checksum stored in the bit grid: " << extractedChecksum << endl;
//             cout << "The recalculated checksum from the grid data: " << recalculatedChecksum << endl;
//         }
//         else if (usrInput == "2")
//         {
//             if (bitGrid.empty())
//             {
//                 cout << "You have not generated a bit grid yet" << endl;
//             }
//             else
//             {
//                 cout << "Your current string:\n" << inputStr << endl;
//                 cout << "\nYour bit grid is:\n" << bitGrid;
//             }
//         }
//         else if (usrInput == "3")
//         {
//             if (bitGrid.empty())
//             {
//                 cout << "You have not generated a bit grid yet" << endl;
//             }
//             else
//             {
//                 saveBitGrid();
//             }
//         }
//         else if (usrInput == "4")
//         {
//             cout << "Note, you must type \"Done\" in a NEW LINE" << endl;
//             cout << "once you've pasted the bit grid you want to" << endl;
//             cout << "decode. Please provide the bit grid:\n" << endl;

//             string line;
//             string fullBitGrid;

//             while (true)
//             {
//                 getline(cin, line);

//                 if (line == "Done")
//                 {
//                     break;
//                 }

//                 fullBitGrid += line + "\n";
//             }

//             cout << "\nDecoded string: " << decodeBitGrid(fullBitGrid) << endl;
//         }
//         else if (usrInput == "5")
//         {
//             string projectInfo = "Project Info:\n"
//                                  "BitGrid Encoder/Decoder is a terminal-based program that encodes text into\n"
//                                  "a custom visual binary format. Each character in the input is converted\n"
//                                  "into an 8-bit binary representation, and the resulting data is displayed\n"
//                                  "as a two-dimensional grid using shaded and filled blocks.\n"
//                                  "\n"
//                                  "Each bit grid begins with a 64-bit header containing a 32-bit HABG format\n"
//                                  "signature, a 16-bit message length, and a 16-bit checksum. The signature\n"
//                                  "identifies the data as using the BitGrid format, while the message length\n"
//                                  "tells the decoder exactly how much encoded message data to read.\n"
//                                  "\n"
//                                  "The checksum provides a basic integrity check for the encoded message. It\n"
//                                  "is calculated from each character's numeric value and position in the\n"
//                                  "original input, then stored in the BitGrid header. During decoding, the\n"
//                                  "program validates the HABG signature, verifies that the expected message\n"
//                                  "data is present, and compares the stored checksum with one recalculated\n"
//                                  "from the decoded message. A matching checksum indicates that no corruption\n"
//                                  "was detected.\n"
//                                  "\n"
//                                  "Users can encode text into a bit grid, view the current grid, save it to a\n"
//                                  "text file, or paste an existing BitGrid representation into the program\n"
//                                  "to validate and decode it back into text.";

//             cout << projectInfo << endl;
//         }
//         else
//         {
//             cout << "INVALID INPUT" << endl;
//         }

//         cout << endl << menuPrompt;
//         cin >> usrInput;
//     }
// }