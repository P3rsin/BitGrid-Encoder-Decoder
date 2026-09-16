#include "BitGridCodec.h"
#include <iostream>

using namespace std;

int printHelp()
{
    cout << "[help text here]\n";
    return 0;
}

int printProjectInfo()
{
    cout << "[Project description]" << '\n';
    return 0;
}

int handleEncodeCommand(int argc, char *argv[])
{
    BitGridCodec bitGridCodec;

    if (argc < 3)
    {
        cout << "add some input text\n";
        return 1;
    }

    string inputText = argv[2];
    cout << bitGridCodec.encode(inputText);

    return 0;
}

int handleDecodeCommand(int argc, char *argv[])
{
    if (argc < 3)
    {
        cout << "add some input text\n";
        return 1;
    }

    BitGridCodec bitGridCodec;

    string bitGridInput;
    string inputLine;

    while (getline(cin, inputLine))
    {
        bitGridInput = bitGridInput + inputLine + '\n';
    }

    cout << bitGridCodec.decode(bitGridInput) << '\n';

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout << "use the help command for help\n";
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
    else if (command == "about")
    {
        return printProjectInfo();
    }
    else if (command == "help")
    {
        return printHelp();
    }
    else
    {
        cout << "Invalid input\n";
        return 1;
    }
}