#include "BitGridCodec.h"
#include <fstream>
#include <iostream>

using namespace std;

int printHelp()
{
    cout << "BitGrid Encoder/Decoder\n\n"
         << "Usage:\n"
         << "  bitgrid encode <text>        Encode literal text into a BitGrid\n"
         << "  bitgrid encode -f <file>     Encode the contents of a file\n"
         << "  bitgrid encode -             Encode text read from standard input\n"
         << "  bitgrid decode <file>        Decode a BitGrid file\n"
         << "  bitgrid decode -             Decode a BitGrid read from standard input\n"
         << "  bitgrid about                Display information about the project\n"
         << "  bitgrid help                 Display this help message\n\n"
         << "Examples:\n"
         << "  bitgrid encode \"Hello world\"\n"
         << "  bitgrid encode -f input.txt\n"
         << "  echo \"Hello world\" | bitgrid encode -\n"
         << "  bitgrid decode encoded.txt\n"
         << "  cat encoded.txt | bitgrid decode -\n";

    return 0;
}

int handleEncodeCommand(int argc, char *argv[])
{
    string inputArgument;

    if (argc >= 3)
    {
        inputArgument = argv[2];
    }
    else
    {
        cerr << "Error: no input provided for encoding.\n"
             << "Run 'bitgrid help' for usage information.\n";
        return 1;
    }

    BitGridCodec bitGridCodec;
    string inputText;
    string inputLine;

    if (inputArgument == "-f")
    {
        string filePath;

        if (argc >= 4)
        {
            filePath = argv[3];
        }
        else
        {
            cerr << "Error: '-f' requires a file path.\n"
                 << "Usage: bitgrid encode -f <file>\n";
            return 1;
        }

        ifstream inputFile(filePath);

        if (!inputFile)
        {
            cerr << "Error: could not open file '" << filePath << "'.\n";
            return 1;
        }

        while (getline(inputFile, inputLine))
        {
            inputText += inputLine + '\n';
        }
    }
    else if (inputArgument == "-")
    {
        while (getline(cin, inputLine))
        {
            inputText = inputText + inputLine;
        }
    }
    else
    {
        inputText = inputArgument;
    }

    CodecResult result = bitGridCodec.encode(inputText);

    if (holds_alternative<CodecError>(result))
    {
        if (get<CodecError>(result) == CodecError::InputTooLarge)
        {
            cerr << "input must be 65535 characters or fewer\n";
        }
        return 1;
    }

    cout << get<string>(result);
    return 0;
}

int handleDecodeCommand(int argc, char *argv[])
{
    string inputArgument;

    if (argc >= 3)
    {
        inputArgument = argv[2];
    }
    else
    {
        cerr << "Error: no BitGrid input provided for decoding.\n"
             << "Usage: bitgrid decode <file>\n"
             << "   or: bitgrid decode -\n";
        return 1;
    }

    BitGridCodec bitGridCodec;
    string bitGridInput;
    string inputLine;

    if (inputArgument == "-")
    {
        while (getline(cin, inputLine))
        {
            bitGridInput = bitGridInput + inputLine;
        }
    }
    else
    {
        ifstream inputFile(inputArgument);

        if (!inputFile)
        {
            cerr << "Error: could not open file '" << inputArgument << "'.\n";
            return 1;
        }

        while (getline(inputFile, inputLine))
        {
            bitGridInput += inputLine + '\n';
        }
    }

    CodecResult result = bitGridCodec.decode(bitGridInput);

    if (holds_alternative<CodecError>(result))
    {
        if (get<CodecError>(result) == CodecError::IncompleteHeader)
        {
            cerr << "ERROR - missing or incomplete header\n";
        }
        else if (get<CodecError>(result) == CodecError::InvalidSignature)
        {
            cerr << "ERROR - invalid bitgrid signature\n";
        }
        else if (get<CodecError>(result) == CodecError::MissingData)
        {
            cerr << "ERROR - missing data\n";
        }
        else if (get<CodecError>(result) == CodecError::ChecksumMismatch)
        {
            cerr << "ERROR - checksums don't match\n";
        }

        return 1;
    }

    cout << get<string>(result);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "Error: no command provided.\n"
             << "Run 'bitgrid help' for usage information.\n";
        return 1;
    }

    string command = argv[1];

    if (command == "encode")
    {
        return handleEncodeCommand(argc, argv);
    }
    else if (command == "decode")
    {
        return handleDecodeCommand(argc, argv);
    }
    else if (command == "help")
    {
        return printHelp();
    }
    else
    {
        cerr << "Error: unknown command '" << command << "'.\n"
             << "Run 'bitgrid help' for usage information.\n";
        return 1;
    }
}