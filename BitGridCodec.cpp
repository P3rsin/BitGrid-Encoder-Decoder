#include "BitGridCodec.h"

#include <cmath>
#include <fstream>
#include <iostream>

using namespace std;

BitGridCodec::BitGridCodec()
{
    inputStr = "";
    binaryVector = populateBinaryVector();
    bitGrid = "";
}

string BitGridCodec::reverseStr(string strToReverse)
{
    string reversedStr = "";

    for (int i = strToReverse.size() - 1; i >= 0; i--)
    {
        reversedStr += strToReverse[i];
    }

    return reversedStr;
}

string BitGridCodec::charToBinary(char character)
{
    int asciiValue = int(character);

    string binary = "";

    while (asciiValue != 0)
    {
        int x = asciiValue % 2;
        asciiValue /= 2;
        binary += to_string(x);
    }

    while (binary.size() < 8)
    {
        binary += "0";
    }

    return reverseStr(binary);
}

string BitGridCodec::stringToBinary(string str)
{
    string binaryStr = "";

    for (int i = 0; i < str.size(); i++)
    {
        binaryStr += charToBinary(str[i]);
    }

    return binaryStr;
}

int BitGridCodec::binaryToInt(string binary)
{
    int value = 0;
    int binarySize = binary.size();
    string reversedBinary = reverseStr(binary);

    for (int i = 0; i < binarySize; i++)
    {
        if (reversedBinary[i] == '1')
        {
            value += pow(2, i);
        }
    }

    return value;
}

char BitGridCodec::binaryToChar(string binary)
{
    int asciiValue = 0;
    size_t binarySize = binary.size();
    string reversedBinary = reverseStr(binary);

    for (size_t i = 0; i < binarySize; i++)
    {
        if (reversedBinary[i] == '1')
        {
            asciiValue += static_cast<int>(pow(2, i));
        }
    }

    return char(asciiValue);
}

string BitGridCodec::intToFixedBinary(int num, int bits)
{
    string binary = "";

    while (num != 0)
    {
        int x = num % 2;
        num /= 2;
        binary += to_string(x);
    }

    while (binary.size() < bits)
    {
        binary += "0";
    }

    return reverseStr(binary);
}

vector<string> BitGridCodec::populateBinaryVector()
{
    size_t inputSize = inputStr.size();
    vector<string> binaryVector(inputSize);

    for (size_t i = 0; i < inputSize; i++)
    {
        binaryVector.at(i) = charToBinary(inputStr[i]);
    }

    return binaryVector;
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
    string inputStrSizeBinary = intToFixedBinary(inputStrSize, 16);

    int checksum = checksumMaker(inputStr);
    string checksumBinary = intToFixedBinary(checksum, 16);

    // 32b sig - 16b input size - 16b checksum;
    header += stringToBinary(bitGridSignature) + inputStrSizeBinary + checksumBinary;

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

string BitGridCodec::decodeBitGrid(string bitGrid)
{
    string binaryStr = "";
    string zeroBlock = "▒▒";
    string oneBlock = "██";

    for (int i = 0; i < bitGrid.size(); i++)
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
    int inputSize = binaryToInt(header.substr(32, 16));

    string decodedMessage = "";

    for (int i = 64; i < 64 + (inputSize * 8); i += 8)
    {
        string charBinary = binaryStr.substr(i, 8);
        decodedMessage += binaryToChar(charBinary);
    }

    return decodedMessage;
}

int BitGridCodec::checksumMaker(string inputStr)
{
    int checksum = 0;

    for (int i = 0; i < inputStr.size(); i++)
    {
        int charValue = inputStr[i];
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

void BitGridCodec::setInputStr(string inputStr)
{
    this->inputStr = inputStr;
    binaryVector = populateBinaryVector();
    bitGrid = constructBitGrid();
}

void BitGridCodec::run()
{
    string usrInput;

    const string title =
        "-----------------------\n"
        "    Bit Grid Manager   \n";

    const string menuPrompt =
        "-----------------------\n"
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

            cout << "\nYour bit grid is now:\n"
                 << bitGrid;

            string checkSumMessage =
                "\nChecksum explanation:\n"
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
                cout << "Your current string:\n"
                     << inputStr << endl;
                cout << "\nYour bit grid is:\n"
                     << bitGrid;
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
            cout << "decode. Please provide the bit grid:\n"
                 << endl;

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
            string projectInfo =
                "Project Info:\n"
                "This is a simple terminal-based QR Code Generator. The program takes a\n"
                "user's string input, converts each character into binary, and displays\n"
                "that binary data as a visual QR-style grid using shaded and filled blocks.\n"
                "\n"
                "The QR code also includes a small header before the message data. This\n"
                "header stores the HAQR project signature, the length of the original\n"
                "message, and a checksum. The signature helps identify the code as one\n"
                "made by my program, while the message length tells the decoder how many\n"
                "characters to read.\n"
                "\n"
                "The checksum is used to check whether the QR code was encoded and decoded\n"
                "correctly. The program calculates a checksum from the original input, then\n"
                "calculates another checksum from the decoded QR output. If both values\n"
                "match, the program reports that the QR code is valid.\n"
                "\n"
                "Users can generate a QR code, view the current QR code, download it to a\n"
                "text file, or paste a QR code back into the program to decode it.";

            cout << projectInfo << endl;
        }
        else
        {
            cout << "INVALID INPUT" << endl;
        }

        cout << endl
             << menuPrompt;
        cin >> usrInput;
    }
}