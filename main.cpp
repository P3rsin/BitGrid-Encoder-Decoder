#include "BitGridCodec.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

int printHelp(int argc)
{
    if (argc != 2)
    {
        cerr << "bitgrid: error: too many arguments\n";
        return 1;
    }

    cout << "BitGrid Encoder/Decoder\n\n"
         << "Usage:\n"
         << "  bitgrid encode <text>        Encode literal text into a BitGrid\n"
         << "  bitgrid encode -f <file>     Encode the contents of a file\n"
         << "  bitgrid encode -             Encode text read from standard input\n"
         << "  bitgrid decode <file>        Decode a BitGrid file\n"
         << "  bitgrid decode -             Decode a BitGrid read from standard input\n"
         << "  bitgrid help                 Display this help message\n\n"
         << "Examples:\n"
         << "  bitgrid encode \"Hello world\"\n"
         << "  bitgrid encode -f input.txt\n"
         << "  echo \"Hello world\" | bitgrid encode -\n"
         << "  bitgrid decode encoded.bgrid\n"
         << "  cat encoded.bgrid | bitgrid decode -\n";

    return 0;
}

int handleEncodeCommand(int argc, char *argv[])
{
    if (argc < 3)
    {
        cerr << "bitgrid: error: no input provided for encoding\n"
             << "Run 'bitgrid help' for usage information.\n";
        return 1;
    }

    string sourceArgument = argv[2];
    BitGridCodec codec;

    istream *streamToUse = nullptr;
    ifstream inputFile;
    istringstream strStream;

    if (sourceArgument == "-f")
    {
        if (argc < 4)
        {
            cerr << "bitgrid: error: '-f' requires a file path\n"
                 << "Usage: bitgrid encode -f <file>\n";
            return 1;
        }
        else if (argc > 4)
        {
            cerr << "too many args";
            return 1;
        }

        string inputFilePath = argv[3];
        inputFile.open(inputFilePath);

        if (!inputFile)
        {
            cerr << "bitgrid: error: could not open file '" << inputFilePath << "'\n";
            return 1;
        }

        streamToUse = &inputFile;
    }
    else if (argc != 3)
    {
        cerr << "bitgrid: error: too many arguments\n";
        return 1;
    }
    else if (sourceArgument == "-")
    {
        streamToUse = &cin;
    }
    else
    {
        strStream.str(sourceArgument);
        streamToUse = &strStream;
    }

    ostringstream sstream;
    sstream << streamToUse->rdbuf();
    CodecResult encodeResult = codec.encode(sstream.str());

    if (holds_alternative<CodecError>(encodeResult))
    {
        CodecError codecError = get<CodecError>(encodeResult);

        switch (codecError)
        {
        case CodecError::InputTooLarge:
            cerr << "bitgrid: error: input must be under max payload size\n";
            break;

        default:
            cerr << "bitgrid: error: encoding failed\n";
            break;
        }

        return 1;
    }

    cout << get<string>(encodeResult);
    return 0;
}

int handleDecodeCommand(int argc, char *argv[])
{
    if (argc < 3)
    {
        cerr << "bitgrid: error: no BitGrid input provided for decoding\n"
             << "Usage: bitgrid decode <file>\n"
             << "   or: bitgrid decode -\n";
        return 1;
    }

    string sourceArgument = argv[2];
    BitGridCodec codec;

    istream *streamToUse = nullptr;
    ifstream inputFile;

    if (argc != 3)
    {
        cerr << "bitgrid: error: too many arguments\n";
        return 1;
    }

    if (sourceArgument == "-")
    {
        streamToUse = &cin;
    }
    else
    {
        inputFile.open(sourceArgument);

        if (!inputFile)
        {
            cerr << "bitgrid: error: could not open file '" << sourceArgument << "'\n";
            return 1;
        }

        streamToUse = &inputFile;
    }

    ostringstream sstream;
    sstream << streamToUse->rdbuf();
    CodecResult decodeResult = codec.decode(sstream.str());

    if (holds_alternative<CodecError>(decodeResult))
    {
        CodecError codecError = get<CodecError>(decodeResult);

        switch (codecError)
        {
        case CodecError::InvalidCharacter:
            cerr << "bitgrid: error: invalid character found\n";
            break;

        case CodecError::IncompleteHeader:
            cerr << "bitgrid: error: missing or incomplete header\n";
            break;

        case CodecError::InvalidSignature:
            cerr << "bitgrid: error: invalid BitGrid signature\n";
            break;

        case CodecError::MissingData:
            cerr << "bitgrid: error: missing encoded data\n";
            break;

        case CodecError::ChecksumMismatch:
            cerr << "bitgrid: error: checksum mismatch\n";
            break;

        default:
            cerr << "bitgrid: error: decoding failed\n";
            break;
        }

        return 1;
    }

    cout << get<string>(decodeResult);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "bitgrid: error: no command provided\n"
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
        return printHelp(argc);
    }
    else
    {
        cerr << "bitgrid: error: unknown command '" << command << "'\n"
             << "Run 'bitgrid help' for usage information.\n";
        return 1;
    }
}