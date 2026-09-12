#include "BitGridCodec.h"

#include <cmath>
#include <fstream>
#include <iostream>

using namespace std;

BitGridCodec::BitGridCodec()
{
    inputStr = "";
    populateBinaryVector();
    bitGrid = "";
}

// string BitGridCodec::reverseStr(const string &str)
// {
//     string reversedStr = "";

//     for (size_t i = str.size(); i-- > 0;)
//     {
//         reversedStr += str[i];
//     }

//     return reversedStr;
// }

string BitGridCodec::intToBinaryString(int num, size_t width)
{
    string binaryStr(width, '0');

    for (size_t i = width; i-- > 0;)
    {
        binaryStr[i] = static_cast<char>('0' + (num % 2));
        // in ASCII: '0' is 48 and '1' is 49 so ^ works out
        num /= 2;
    }

    // note, if num > 0 at this point, the size  
    // of width wasn't sufficient to convert num

    return binaryStr;
}

string BitGridCodec::charToBinaryString(char character)
{
    // we want char unsigned (0 - 255) to perform our conversion
    unsigned char value = static_cast<unsigned char>(character);
    return intToBinaryString(value, 8);
}

string BitGridCodec::textToBinaryString(const string &str)
{
    string binaryStr;
    // we know each char will produce 8 chars 
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
        if (binaryStr[i] == '1')
        {
            value += pow(2, binaryStr.size() - 1 - i);
            // would be nice to avoid the floating point here
        }
    }

    return value;
}

char BitGridCodec::binaryStringToChar(const string &binary)
{
    return static_cast<char>(binaryStringToInt(binary));
}

void BitGridCodec::populateBinaryVector()
{
    size_t inputSize = inputStr.size();
    binaryVector.resize(inputSize);

    for (size_t i = 0; i < inputSize; i++)
    {
        binaryVector.at(i) = charToBinaryString(inputStr[i]);
    }
}

string BitGridCodec::binaryVectorToStr()
{
    string binaryStr = "";

    for (size_t i = 0; i < binaryVector.size(); i++)
    {
        binaryStr += binaryVector[i];
    }

    return binaryStr;
}

string BitGridCodec::constructBitGrid()
{
    // ---------------------- header for the bit grid ----------------------

    string header = "";

    int inputStrSize = inputStr.size();
    string inputStrSizeBinary = intToBinaryString(inputStrSize, 16);

    int checksum = checksumMaker(inputStr);
    string checksumBinary = intToBinaryString(checksum, 16);

    // 32b sig - 16b input size - 16b checksum;
    header += textToBinaryString(bitGridSignature) + inputStrSizeBinary + checksumBinary;

    // ----------------------------------------------------------------------

    string binaryStr = header + binaryVectorToStr();
    int totalNumBits = static_cast<int>(binaryStr.size());
    int gridDimensions = 1;

    while (pow(gridDimensions, 2) < totalNumBits)
    {
        gridDimensions++;
    }

    gridDimensions += 2;

    bitGrid = "";
    size_t idx = 0;

    for (int i = 0; i < gridDimensions; i++)
    {
        for (int j = 0; j < gridDimensions; j++)
        {
            if (i == 0 && j == 0)
            {
                bitGrid += "╔";
            }
            else if (i == 0 && j == gridDimensions - 1)
            {
                bitGrid += "╗";
            }
            else if (i == gridDimensions - 1 && j == 0)
            {
                bitGrid += "╚";
            }
            else if (i == gridDimensions - 1 && j == gridDimensions - 1)
            {
                bitGrid += "╝";
            }
            else if (i == 0 || i == gridDimensions - 1)
            {
                bitGrid += "══";
            }
            else if (j == 0 || j == gridDimensions - 1)
            {
                bitGrid += "║";
            }
            else
            {
                if (idx < totalNumBits)
                {
                    if (binaryStr[idx] == '1')
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

                idx++;
            }
        }

        bitGrid += "\n";
    }

    return bitGrid;
}

string BitGridCodec::decodeBitGrid(const string &bitGrid)
{
    string binaryStr = "";
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

    string header = binaryStr.substr(0, 64);
    int inputSize = binaryStringToInt(header.substr(32, 16));

    string decodedMessage = "";

    for (int i = 64; i < 64 + (inputSize * 8); i += 8)
    {
        string charBinary = binaryStr.substr(i, 8);
        decodedMessage += binaryStringToChar(charBinary);
    }

    return decodedMessage;
}

int BitGridCodec::checksumMaker(const string &inputStr)
{
    int checksum = 0;

    for (size_t i = 0; i < inputStr.size(); i++)
    {
        int charValue = static_cast<int>(inputStr[i]);
        int signatureValue = bitGridSignature[i % bitGridSignature.size()];

        checksum += (charValue * (i + 1)) + signatureValue;
        checksum = checksum % 256;
    }

    return checksum;
}

void BitGridCodec::downloadBitGrid()
{
    ofstream bitGridFile("bitGrid.txt");

    if (bitGridFile.is_open())
    {
        bitGridFile << bitGrid;
        bitGridFile.close();

        cout << "Bit grid downloaded successfully" << endl;
    }
    else
    {
        cout << "An error occurred while downloading.\n"
             << "Please try downloading the bit grid again\n"
             << endl;
    }
}

void BitGridCodec::setInputStr(const string &inputStr)
{
    this->inputStr = inputStr;
    populateBinaryVector();
    bitGrid = constructBitGrid();
}

void BitGridCodec::run()
{
    string usrInput;

    const string title = "-------------------------\n"
                         " BitGrid Encoder/Decoder \n";

    const string menuPrompt = "-------------------------\n"
                              "[1] Generate a bit grid\n"
                              "[2] View my bit grid\n"
                              "[3] Download my bit grid\n"
                              "[4] Decode a bit grid\n"
                              "[5] About the project\n"
                              "[6] Exit\n"
                              "Choice: ";

    cout << title << menuPrompt;
    cin >> usrInput;

    while (usrInput != "6")
    {
        cout << endl;

        if (usrInput == "1")
        {
            cout << "Please enter a string to convert\nMy Input: ";

            cin.ignore();
            getline(cin, inputStr);
            setInputStr(inputStr);

            cout << "\nYour bit grid is now:\n" << bitGrid;

            string checkSumMessage = "\nChecksum explanation:\n"
                                     "This bit grid uses a position-weighted checksum.\n"
                                     "Each character's ASCII value is multiplied by its position, then\n"
                                     "mixed with the HABG signature. This helps detect if the message\n"
                                     "changes while being encoded or decoded.";

            cout << "Input checksum: " << checksumMaker(inputStr) << endl;
            cout << "Bit grid checksum: " << checksumMaker(decodeBitGrid(bitGrid)) << endl;

            if (checksumMaker(decodeBitGrid(bitGrid)) == checksumMaker(inputStr))
            {
                cout << "The checksums match, so the bit grid is valid." << endl;
            }
            else
            {
                cout << "The checksums do not match, so the bit grid may be invalid." << endl;
            }

            cout << checkSumMessage << endl;
        }
        else if (usrInput == "2")
        {
            if (bitGrid.empty())
            {
                cout << "You have not generated a bit grid yet" << endl;
            }
            else
            {
                cout << "Your current string:\n" << inputStr << endl;
                cout << "\nYour bit grid is:\n" << bitGrid;
            }
        }
        else if (usrInput == "3")
        {
            if (bitGrid.empty())
            {
                cout << "You have not generated a bit grid yet" << endl;
            }
            else
            {
                downloadBitGrid();
            }
        }
        else if (usrInput == "4")
        {
            cout << "Note, you must type \"Done\" in a NEW LINE" << endl;
            cout << "once you've pasted the bit grid you want to" << endl;
            cout << "decode. Please provide the bit grid:\n" << endl;

            string line;
            string fullBitGrid = "";

            while (true)
            {
                getline(cin, line);

                if (line == "Done")
                {
                    break;
                }

                fullBitGrid += line + "\n";
            }

            cout << "\nDecoded string: " << decodeBitGrid(fullBitGrid) << endl;
        }
        else if (usrInput == "5")
        {
            string projectInfo = "Project Info:\n"
                                 "BitGrid Encoder/Decoder is a terminal-based program that encodes text into\n"
                                 "a custom visual binary format. Each character in the input is converted\n"
                                 "into an 8-bit binary representation, and the resulting data is displayed\n"
                                 "as a two-dimensional grid using shaded and filled blocks.\n"
                                 "\n"
                                 "Each bit grid begins with a 64-bit header containing a 32-bit HABG format\n"
                                 "signature, a 16-bit message length, and a 16-bit checksum. The signature\n"
                                 "identifies the data as using the BitGrid format, while the message length\n"
                                 "tells the decoder how much encoded message data to read.\n"
                                 "\n"
                                 "The checksum provides an integrity check for the encoded message. It is\n"
                                 "calculated from the characters in the original input, their positions,\n"
                                 "and the HABG signature. When a bit grid is decoded, the stored checksum\n"
                                 "is compared with a checksum calculated from the decoded message. Matching\n"
                                 "values indicate that the message was decoded without detected corruption.\n"
                                 "\n"
                                 "Users can encode text into a bit grid, view the current grid, save it to a\n"
                                 "text file, or paste an existing BitGrid representation into the program\n"
                                 "to decode it back into text.";

            cout << projectInfo << endl;
        }
        else
        {
            cout << "INVALID INPUT" << endl;
        }

        cout << endl << menuPrompt;
        cin >> usrInput;
    }
}